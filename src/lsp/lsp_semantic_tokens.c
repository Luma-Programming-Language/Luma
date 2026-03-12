/**
 * @file lsp_semantic_tokens.c
 * @brief LSP textDocument/semanticTokens/full implementation for Luma.
 *
 * We classify every token the lexer produced and encode them in the LSP
 * delta-encoded wire format (5 integers per token).
 *
 * Token type legend (indices MUST match the "tokenTypes" array sent in the
 * capabilities response — see lsp_message.c):
 *
 *   Index  Name
 *   -----  ----
 *     0    namespace
 *     1    type
 *     2    typeParameter
 *     3    function
 *     4    method
 *     5    property
 *     6    variable
 *     7    parameter
 *     8    keyword
 *     9    modifier
 *    10    comment
 *    11    string
 *    12    number
 *    13    operator
 *    14    struct
 *    15    enum
 *    16    enumMember
 *
 * Token modifier bitmask (indices MUST match "tokenModifiers"):
 *   Bit 0  declaration
 *   Bit 1  definition
 *   Bit 2  readonly
 *   Bit 3  static
 *   Bit 4  defaultLibrary
 */

#include "lsp.h"
#include <string.h>

enum SemanticTokenType {
  ST_NAMESPACE = 0,
  ST_TYPE = 1,
  ST_TYPE_PARAMETER = 2,
  ST_FUNCTION = 3,
  ST_METHOD = 4,
  ST_PROPERTY = 5,
  ST_VARIABLE = 6,
  ST_PARAMETER = 7,
  ST_KEYWORD = 8,
  ST_MODIFIER = 9,
  ST_COMMENT = 10,
  ST_STRING = 11,
  ST_NUMBER = 12,
  ST_OPERATOR = 13,
  ST_STRUCT = 14,
  ST_ENUM = 15,
  ST_ENUM_MEMBER = 16,
  ST_COUNT = 17 /* sentinel — keep last */
};

enum SemanticTokenMod {
  SM_DECLARATION = (1 << 0),
  SM_DEFINITION = (1 << 1),
  SM_READONLY = (1 << 2),
  SM_STATIC = (1 << 3),
  SM_DEFAULT_LIB = (1 << 4),
};

typedef struct {
  int type;
  int mods;
} TokenClass;

static Symbol *find_symbol_anywhere(Scope *scope, const char *name) {
  if (!scope || !name)
    return NULL;

  if (scope->symbols.data) {
    for (size_t i = 0; i < scope->symbols.count; i++) {
      Symbol *sym =
          (Symbol *)((char *)scope->symbols.data + i * sizeof(Symbol));
      if (sym && sym->name && strcmp(sym->name, name) == 0)
        return sym;
    }
  }

  if (scope->children.data) {
    for (size_t i = 0; i < scope->children.count; i++) {
      Scope **child_ptr =
          (Scope **)((char *)scope->children.data + i * sizeof(Scope *));
      if (*child_ptr) {
        Symbol *found = find_symbol_anywhere(*child_ptr, name);
        if (found)
          return found;
      }
    }
  }

  return NULL;
}

static bool is_module_alias(LSPDocument *doc, const char *word) {
  if (!doc->imports)
    return false;
  for (size_t i = 0; i < doc->import_count; i++) {
    const char *alias = doc->imports[i].alias ? doc->imports[i].alias
                                              : doc->imports[i].module_path;
    if (alias && strcmp(alias, word) == 0)
      return true;
  }
  return false;
}

static TokenClass classify_token(LSPDocument *doc, size_t tok_idx) {
  Token *tok = &doc->tokens[tok_idx];
  TokenClass none = {-1, 0};

  if (!tok->value || tok->length == 0)
    return none;

  switch (tok->type_) {

  /* --- Punctuation / structural tokens: not highlighted --- */
  case TOK_EOF:
  case TOK_ERROR:
  case TOK_WHITESPACE:
  case TOK_LPAREN:
  case TOK_RPAREN:
  case TOK_LBRACE:
  case TOK_RBRACE:
  case TOK_LBRACKET:
  case TOK_RBRACKET:
  case TOK_SEMICOLON:
  case TOK_COMMA:
  case TOK_AT:
  case TOK_COLON:
  case TOK_QUESTION:
  case TOK_BANG:
  case TOK_SYMBOL:
    return none;

  /* --- Comments --- */
  case TOK_COMMENT:
  case TOK_DOC_COMMENT:
  case TOK_MODULE_DOC:
  case TOK_DOCUMENT:
    return (TokenClass){ST_COMMENT, 0};

  /* --- String / char literals --- */
  case TOK_STRING:
  case TOK_CHAR_LITERAL:
    return (TokenClass){ST_STRING, 0};

  /* --- Numeric literals --- */
  case TOK_NUMBER:
  case TOK_NUM_FLOAT:
    return (TokenClass){ST_NUMBER, 0};

  /* --- Built-in type keywords (int, bool, void, …) --- */
  case TOK_INT:
  case TOK_DOUBLE:
  case TOK_UINT:
  case TOK_FLOAT:
  case TOK_BOOL:
  case TOK_STRINGT:
  case TOK_VOID:
  case TOK_CHAR:
    return (TokenClass){ST_TYPE, SM_DEFAULT_LIB};

  /* --- Control-flow keywords --- */
  case TOK_IF:
  case TOK_ELIF:
  case TOK_ELSE:
  case TOK_LOOP:
  case TOK_RETURN:
  case TOK_BREAK:
  case TOK_CONTINUE:
  case TOK_SWITCH:
  case TOK_DEFER:
  case TOK_IN:
    return (TokenClass){ST_KEYWORD, 0};

  /* --- Declaration / visibility keywords --- */
  case TOK_VAR:
  case TOK_CONST:
  case TOK_PUBLIC:
  case TOK_PRIVATE:
  case TOK_IMPL:
    return (TokenClass){ST_KEYWORD, SM_STATIC};

  /* --- Type-declaration keywords --- */
  case TOK_FN:
  case TOK_STRUCT:
  case TOK_ENUM:
    return (TokenClass){ST_KEYWORD, 0};

  /* --- Module / import keywords --- */
  case TOK_MODULE:
  case TOK_USE:
  case TOK_IMPORT:
  case TOK_AS:
    return (TokenClass){ST_KEYWORD, SM_STATIC};

  /* --- Built-in functions --- */
  case TOK_PRINT:
  case TOK_PRINTLN:
  case TOK_INPUT:
  case TOK_ALLOC:
  case TOK_FREE:
  case TOK_SYSTEM:
  case TOK_SYSCALL:
  case TOK_CAST:
  case TOK_SIZE_OF:
    return (TokenClass){ST_FUNCTION, SM_DEFAULT_LIB};

  /* --- Boolean literals --- */
  case TOK_TRUE:
  case TOK_FALSE:
    return (TokenClass){ST_ENUM_MEMBER, SM_READONLY | SM_DEFAULT_LIB};

  /* --- Ownership attributes --- */
  case TOK_RETURNES_OWNERSHIP:
  case TOK_TAKES_OWNERSHIP:
    return (TokenClass){ST_MODIFIER, SM_DEFAULT_LIB};

  /* --- Operators --- */
  case TOK_EQUAL:
  case TOK_PLUS:
  case TOK_MINUS:
  case TOK_STAR:
  case TOK_SLASH:
  case TOK_MODL:
  case TOK_LT:
  case TOK_GT:
  case TOK_LE:
  case TOK_GE:
  case TOK_EQEQ:
  case TOK_NEQ:
  case TOK_AMP:
  case TOK_PIPE:
  case TOK_CARET:
  case TOK_TILDE:
  case TOK_AND:
  case TOK_OR:
  case TOK_PLUSPLUS:
  case TOK_MINUSMINUS:
  case TOK_SHIFT_LEFT:
  case TOK_SHIFT_RIGHT:
  case TOK_RANGE:
  case TOK_RIGHT_ARROW:
  case TOK_LEFT_ARROW:
  case TOK_DOT:
  case TOK_RESOLVE:
    return (TokenClass){ST_OPERATOR, 0};

  case TOK_KEYWORD:
    return (TokenClass){ST_KEYWORD, 0};

  /* --- Identifiers: scope-aware + heuristic fallback --- */
  case TOK_IDENTIFIER: {
    char word[256];
    size_t len = (size_t)tok->length < 255 ? (size_t)tok->length : 255;
    memcpy(word, tok->value, len);
    word[len] = '\0';

    if (is_module_alias(doc, word))
      return (TokenClass){ST_NAMESPACE, 0};

    if (doc->scope) {
      Symbol *sym = find_symbol_anywhere(doc->scope, word);
      if (sym && sym->type) {
        switch (sym->type->type) {
        case AST_TYPE_FUNCTION:
          if (tok_idx + 1 < doc->token_count &&
              doc->tokens[tok_idx + 1].type_ == TOK_LPAREN)
            return (TokenClass){ST_FUNCTION,
                                sym->is_public ? SM_DECLARATION : 0};
          return (TokenClass){ST_FUNCTION, 0};

        case AST_TYPE_STRUCT:
          return (TokenClass){
              ST_STRUCT, SM_DEFINITION | (sym->is_public ? SM_DECLARATION : 0)};

        default:
          if (word[0] >= 'A' && word[0] <= 'Z')
            return (TokenClass){ST_ENUM_MEMBER, SM_READONLY};
          return (TokenClass){ST_VARIABLE, sym->is_mutable ? 0 : SM_READONLY};
        }
      }
    }

    if (word[0] >= 'A' && word[0] <= 'Z')
      return (TokenClass){ST_TYPE, 0};

    if (tok_idx + 1 < doc->token_count &&
        doc->tokens[tok_idx + 1].type_ == TOK_LPAREN)
      return (TokenClass){ST_FUNCTION, 0};

    if (tok_idx + 1 < doc->token_count &&
        doc->tokens[tok_idx + 1].type_ == TOK_RESOLVE)
      return (TokenClass){ST_NAMESPACE, 0};

    return (TokenClass){ST_VARIABLE, 0};
  }

  default:
    return none;
  }
}

// TODO: Look into this some more currently strings and the default condition seem to be working fun
// Comments and chars seem to have issues not sure as to way yet
static void token_display_col_and_len(Token *tok, int *out_col, int *out_len) {
  int raw_len = (int)tok->length;

  switch (tok->type_) {
  case TOK_STRING:
    /* value excludes both " delimiters */
    *out_len = raw_len + 2;
    *out_col = (int)tok->col - raw_len - 2 + 1;
    break;

  case TOK_CHAR_LITERAL:
    /* value excludes both ' delimiters */
    *out_len = raw_len + 2;
    *out_col = (int)tok->col - raw_len + 1;
    break;

  case TOK_COMMENT:
    /* value excludes the leading // (2 chars) */
    *out_len = raw_len + 2;
    *out_col = (int)tok->col - raw_len;
    break;

  case TOK_DOC_COMMENT:
    /* value excludes the leading /// (3 chars) */
    *out_len = raw_len + 3;
    *out_col = (int)tok->col - raw_len;
    break;

  case TOK_MODULE_DOC:
    /* value excludes the leading //! (3 chars) */
    *out_len = raw_len + 3;
    *out_col = (int)tok->col - raw_len;
    break;

  default:
    /* All other tokens: col is 0-based end, length is exact */
    *out_len = raw_len;
    *out_col = (int)tok->col - raw_len + 1;
    break;
  }
}

char *lsp_semantic_tokens_full(LSPDocument *doc, ArenaAllocator *arena) {
  if (!doc || !doc->tokens || doc->token_count == 0)
    return NULL;

  fprintf(stderr, "[LSP] semanticTokens: encoding %zu tokens\n",
          doc->token_count);

  /* Heap-allocate the data buffer so we never blow a 64KB temp arena. */
  size_t data_buf_size = doc->token_count * 64 + 64;
  char *data_buf = (char *)malloc(data_buf_size);
  if (!data_buf)
    return NULL;

  size_t offset = 0;
  int prev_line = 0;
  int prev_char = 0;
  size_t token_count = 0;

  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    TokenClass cls = classify_token(doc, i);

    if (cls.type < 0)
      continue; /* skip punctuation, whitespace, etc. */

    /* Grow buffer defensively. */
    if (offset + 80 > data_buf_size) {
      data_buf_size *= 2;
      char *tmp = (char *)realloc(data_buf, data_buf_size);
      if (!tmp) {
        free(data_buf);
        return NULL;
      }
      data_buf = tmp;
    }

    if ((int)tok->line < 1 || (int)tok->col < 1) {
      fprintf(stderr,
              "[LSP] semanticTokens: skipping token '%.*s' with "
              "invalid position line=%d col=%d\n",
              (int)tok->length, tok->value, (int)tok->line, (int)tok->col);
      continue;
    }

    int line = (int)tok->line - 1; /* convert to 0-based */
    int col, tok_len;
    token_display_col_and_len(tok, &col, &tok_len);

    if (col < 0) {
      fprintf(stderr,
              "[LSP] semanticTokens: skipping token '%.*s' with "
              "negative computed col=%d (raw col=%d len=%d)\n",
              (int)tok->length, tok->value, col, (int)tok->col,
              (int)tok->length);
      continue;
    }

    int delta_line = line - prev_line;
    int delta_char = (delta_line == 0) ? (col - prev_char) : col;

    if (delta_line < 0 || (delta_line == 0 && delta_char < 0)) {
      fprintf(stderr,
              "[LSP] semanticTokens: skipping out-of-order token "
              "'%.*s' at %d:%d (prev emitted %d:%d)\n",
              (int)tok->length, tok->value, line, col, prev_line, prev_char);
      continue;
    }

    if (token_count > 0)
      data_buf[offset++] = ',';

    offset += (size_t)snprintf(data_buf + offset, data_buf_size - offset,
                               "%d,%d,%d,%d,%d", delta_line, delta_char,
                               tok_len, cls.type, cls.mods);
    prev_line = line;
    prev_char = col;
    token_count++;
  }

  fprintf(stderr, "[LSP] semanticTokens: emitting %zu classified tokens\n",
          token_count);

  size_t result_size = offset + 32;
  char *result = arena_alloc(arena, result_size, 1);
  if (result) {
    snprintf(result, result_size, "{\"data\":[%.*s]}", (int)offset, data_buf);
  }
  free(data_buf);
  return result;
}

const char *lsp_semantic_tokens_capabilities(void) {
  return "\"semanticTokensProvider\":{"
         "\"legend\":{"
         "\"tokenTypes\":["
         "\"namespace\","     /* 0  */
         "\"type\","          /* 1  */
         "\"typeParameter\"," /* 2  */
         "\"function\","      /* 3  */
         "\"method\","        /* 4  */
         "\"property\","      /* 5  */
         "\"variable\","      /* 6  */
         "\"parameter\","     /* 7  */
         "\"keyword\","       /* 8  */
         "\"modifier\","      /* 9  */
         "\"comment\","       /* 10 */
         "\"string\","        /* 11 */
         "\"number\","        /* 12 */
         "\"operator\","      /* 13 */
         "\"struct\","        /* 14 */
         "\"enum\","          /* 15 */
         "\"enumMember\""     /* 16 */
         "],"
         "\"tokenModifiers\":["
         "\"declaration\","   /* bit 0 */
         "\"definition\","    /* bit 1 */
         "\"readonly\","      /* bit 2 */
         "\"static\","        /* bit 3 */
         "\"defaultLibrary\"" /* bit 4 */
         "]"
         "},"
         "\"full\":true"
         "}";
}