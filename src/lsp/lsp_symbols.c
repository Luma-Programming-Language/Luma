#include "lsp.h"
#include <string.h>

AstNode *lsp_node_at_position(LSPDocument *doc, LSPPosition position) {
  (void)doc;
  (void)position;
  return NULL;
}

// Map a Symbol's type to the closest LSPSymbolKind.
static LSPSymbolKind symbol_kind_for(Symbol *sym) {
  if (!sym || !sym->type)
    return LSP_SYMBOL_VARIABLE;

  switch (sym->type->type) {
  case AST_TYPE_FUNCTION:
    return LSP_SYMBOL_FUNCTION;
  case AST_TYPE_STRUCT:
    return LSP_SYMBOL_STRUCT;
  default:
    return sym->is_mutable ? LSP_SYMBOL_VARIABLE : LSP_SYMBOL_CONSTANT;
  }
}

// Find the first token whose value matches `name` exactly.
// Returns the token index, or -1 if not found.
static int find_token_for_name(LSPDocument *doc, const char *name) {
  if (!doc || !doc->tokens || !name)
    return -1;

  size_t name_len = strlen(name);
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    if (tok->type_ == TOK_IDENTIFIER && tok->length == name_len &&
        tok->value && strncmp(tok->value, name, name_len) == 0) {
      return (int)i;
    }
  }
  return -1;
}

LSPDocumentSymbol **lsp_document_symbols(LSPDocument *doc,
                                         size_t *symbol_count,
                                         ArenaAllocator *arena) {
  if (!doc || !symbol_count) {
    if (symbol_count)
      *symbol_count = 0;
    return NULL;
  }

  *symbol_count = 0;

  if (!doc->scope || !doc->scope->symbols.data ||
      doc->scope->symbols.count == 0) {
    return NULL;
  }

  size_t count = doc->scope->symbols.count;

  // Allocate an array of pointers
  LSPDocumentSymbol **symbols =
      arena_alloc(arena, count * sizeof(LSPDocumentSymbol *),
                  alignof(LSPDocumentSymbol *));
  if (!symbols)
    return NULL;

  size_t out = 0;

  for (size_t i = 0; i < count; i++) {
    Symbol *sym = (Symbol *)((char *)doc->scope->symbols.data +
                             i * sizeof(Symbol));

    if (!sym || !sym->name)
      continue;

    // Skip internal/compiler-generated symbols
    if (strncmp(sym->name, "__", 2) == 0)
      continue;

    LSPDocumentSymbol *dsym =
        arena_alloc(arena, sizeof(LSPDocumentSymbol),
                    alignof(LSPDocumentSymbol));
    if (!dsym)
      continue;

    dsym->name        = arena_strdup(arena, sym->name);
    dsym->kind        = symbol_kind_for(sym);
    dsym->children    = NULL;
    dsym->child_count = 0;

    // Default range: line 0 (fallback when token lookup fails)
    dsym->range.start.line      = 0;
    dsym->range.start.character = 0;
    dsym->range.end.line        = 0;
    dsym->range.end.character   = 0;
    dsym->selection_range       = dsym->range;

    // Try to locate the defining token for precise line/col
    int tok_idx = find_token_for_name(doc, sym->name);
    if (tok_idx >= 0) {
      Token *tok = &doc->tokens[tok_idx];

      // LSP lines/characters are 0-based; Token lines/cols may be 1-based
      int line = (int)tok->line > 0 ? (int)tok->line - 1 : 0;
      int col  = (int)tok->col  > 0 ? (int)tok->col  - 1 : 0;
      int end_col = col + (int)tok->length;

      dsym->range.start.line      = line;
      dsym->range.start.character = col;
      dsym->range.end.line        = line;
      dsym->range.end.character   = end_col;
      dsym->selection_range       = dsym->range;
    }

    symbols[out++] = dsym;
  }

  *symbol_count = out;
  return out > 0 ? symbols : NULL;
}