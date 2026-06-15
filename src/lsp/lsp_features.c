#include "lsp.h"
#include <string.h>

static const char *json_escape_hover(const char *src, ArenaAllocator *arena) {
  if (!src) return "";
  size_t len = strlen(src);
  // Worst case: every char doubles
  char *dst = arena_alloc(arena, len * 2 + 1, 1);
  if (!dst) return src;
  char *out = dst;
  while (*src) {
    switch (*src) {
    case '"':  *out++ = '\\'; *out++ = '"';  break;
    case '\\': *out++ = '\\'; *out++ = '\\'; break;
    case '\n': *out++ = '\\'; *out++ = 'n';  break;
    case '\r': *out++ = '\\'; *out++ = 'r';  break;
    case '\t': *out++ = '\\'; *out++ = 't';  break;
    default:   *out++ = *src; break;
    }
    src++;
  }
  *out = '\0';
  return dst;
}

static Token *find_token_at(LSPDocument *doc, LSPPosition pos) {
  if (!doc || !doc->tokens) return NULL;
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    int tok_line = (int)tok->line - 1;
    int tok_col  = (int)tok->col - (int)tok->length + 1;  // 0-based start
    int tok_end  = tok_col + (int)tok->length;             // unchanged
    if (tok_line == pos.line &&
        tok_col <= pos.character && pos.character < tok_end) {
      return tok;
    }
  }
  return NULL;
}

static bool token_is_name_like(Token *tok) {
  if (!tok || !tok->value || tok->length == 0) return false;
  if (tok->type_ == TOK_IDENTIFIER) return true;
  char c = tok->value[0];
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static const char *hover_for_symbol(Symbol *sym, const char *module_alias,
                                     ArenaAllocator *arena) {
  if (!sym || !sym->name) return NULL;

  // Normalize '.' enum separators to '::' (e.g. "Foo.Bar" -> "Foo::Bar")
  const char *_raw_type = sym->type ? type_to_string(sym->type, arena) : "unknown";
  size_t _tlen = strlen(_raw_type);
  char *_type_buf = arena_alloc(arena, _tlen * 2 + 1, 1);
  if (_type_buf) {
    char *_p = _type_buf; const char *_s = _raw_type;
    while (*_s) { if (*_s == '.') { *_p++ = ':'; *_p++ = ':'; _s++; } else { *_p++ = *_s++; } }
    *_p = '\0';
  }
  const char *type_str = _type_buf ? _type_buf : _raw_type;

  // Determine declaration keyword
  const char *kw = "let";
  if (sym->type) {
    switch (sym->type->type) {
    case AST_TYPE_FUNCTION: kw = "fn";     break;
    case AST_TYPE_STRUCT:   kw = "struct"; break;
    case AST_TYPE_ENUM:     kw = "enum";   break;
    default:
      kw = sym->is_mutable ? "let" : "const";
      break;
    }
  }

  // Build the code line shown in the hover
  size_t code_size = (module_alias ? strlen(module_alias) : 0) + strlen(sym->name) + strlen(type_str) + 16;
  char *code = arena_alloc(arena, code_size, 1);
  if (!code) return NULL;
  if (module_alias) {
    snprintf(code, code_size, "%s::%s: %s", module_alias, sym->name, type_str);
  } else {
    snprintf(code, code_size, "%s %s: %s", kw, sym->name, type_str);
  }

  // Build visibility/mutability tags
  size_t tags_size = 256;
  char *tags = arena_alloc(arena, tags_size, 1);
  if (!tags) return NULL;
  tags[0] = '\0';
  if (sym->is_public)   strncat(tags, "public ", tags_size - strlen(tags) - 1);
  if (sym->is_mutable)  strncat(tags, "mutable ", tags_size - strlen(tags) - 1);

  // Build extra details for structs
  char extra[2048];
  extra[0] = '\0';

  if (sym->type && sym->type->type == AST_TYPE_STRUCT) {
    size_t mc = sym->type->type_data.struct_type.member_count;
    if (mc > 0 && sym->type->type_data.struct_type.member_names) {
      strncat(extra, "\n\n**Fields:**\n", sizeof(extra) - strlen(extra) - 1);
      for (size_t i = 0; i < mc; i++) {
        char field_line[256];
        const char *field_type_str = sym->type->type_data.struct_type.member_types[i]
          ? type_to_string(sym->type->type_data.struct_type.member_types[i], arena)
          : "?";
        snprintf(field_line, sizeof(field_line), "- `%s`: %s\n",
                 sym->type->type_data.struct_type.member_names[i],
                 field_type_str);
        strncat(extra, field_line, sizeof(extra) - strlen(extra) - 1);
      }
    }
  }

  if (sym->type && sym->type->type == AST_TYPE_FUNCTION &&
      sym->type->type_data.function.param_types) {
    strncat(extra, "\n\n**Parameters:**\n", sizeof(extra) - strlen(extra) - 1);
    AstNode *func_type = sym->type;
    for (size_t i = 0; i < func_type->type_data.function.param_count; i++) {
      char param_line[256];
      const char *ptype = type_to_string(func_type->type_data.function.param_types[i], arena);
      snprintf(param_line, sizeof(param_line), "- `%s`\n", ptype);
      strncat(extra, param_line, sizeof(extra) - strlen(extra) - 1);
    }
    const char *ret = func_type->type_data.function.return_type
      ? type_to_string(func_type->type_data.function.return_type, arena) : "void";
    char ret_line[128];
    snprintf(ret_line, sizeof(ret_line), "\n**Returns:** `%s`\n", ret);
    strncat(extra, ret_line, sizeof(extra) - strlen(extra) - 1);
  }

  // Final markdown
  size_t buf_size = strlen(code) + strlen(tags) + strlen(extra) + 128;
  char *result = arena_alloc(arena, buf_size, 1);
  if (!result) return NULL;

  if (strlen(tags) > 0 && strlen(extra) > 0) {
    snprintf(result, buf_size, "```luma\n%s\n```\n*%s*\n%s", code, tags, extra);
  } else if (strlen(tags) > 0) {
    snprintf(result, buf_size, "```luma\n%s\n```\n*%s*", code, tags);
  } else if (strlen(extra) > 0) {
    snprintf(result, buf_size, "```luma\n%s\n```\n%s", code, extra);
  } else {
    snprintf(result, buf_size, "```luma\n%s\n```", code);
  }

  return result;  // caller applies json_escape_hover()
}

typedef struct { Symbol *sym; const char *scope_name; bool is_module_scope; } ScopeSymbol;

// Recursively collect symbols, flagging whether they come from a module scope
// (i.e. an imported dependency) vs the current document's own scope tree.
static void collect_all_symbols(Scope *scope, ScopeSymbol *buf,
                                 size_t *count, size_t capacity,
                                 bool is_imported) {
  if (!scope || *count >= capacity) return;

  if (scope->symbols.data) {
    for (size_t i = 0; i < scope->symbols.count && *count < capacity; i++) {
      Symbol *sym = (Symbol *)((char *)scope->symbols.data + i * sizeof(Symbol));
      if (sym && sym->name) {
        buf[*count].sym = sym;
        buf[*count].scope_name = scope->scope_name ? scope->scope_name : "?";
        buf[*count].is_module_scope = is_imported;
        (*count)++;
      }
    }
  }

  if (scope->children.data) {
    for (size_t i = 0; i < scope->children.count; i++) {
      Scope **child_ptr = (Scope **)((char *)scope->children.data + i * sizeof(Scope *));
      if (*child_ptr) {
        // Children of an already-imported scope stay imported
        collect_all_symbols(*child_ptr, buf, count, capacity, is_imported);
      }
    }
  }
}

// Find the name of the function that contains the given line by scanning
// backwards through the token array for the nearest function-name token.
// Returns NULL if not determinable.
static const char *enclosing_function_name(LSPDocument *doc, int line_0based) {
  if (!doc || !doc->tokens) return NULL;

  static char fn_name_buf[256];
  const char *best_fn = NULL;

  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    int tok_line = (int)tok->line - 1;

    if (tok_line > line_0based) break;

    if (tok->type_ == TOK_IDENTIFIER && tok->length > 0) {
      if (i + 1 < doc->token_count) {
        Token *next = &doc->tokens[i + 1];
        if (next->type_ == TOK_RIGHT_ARROW) {
          int copy_len = tok->length < (int)sizeof(fn_name_buf) - 1 ? tok->length : (int)sizeof(fn_name_buf) - 1;
          memcpy(fn_name_buf, tok->value, copy_len);
          fn_name_buf[copy_len] = '\0';
          best_fn = fn_name_buf;
        }
      }
    }
  }

  return best_fn;
}

const char *lsp_hover(LSPDocument *doc, LSPPosition position,
                      ArenaAllocator *arena) {
  if (!doc) return NULL;

  // 1. Find the token at the cursor
  Token *tok = find_token_at(doc, position);
  if (!tok || !token_is_name_like(tok) || !tok->value || tok->length == 0) return NULL;

  size_t name_len = tok->length;
  char *name = arena_alloc(arena, name_len + 1, 1);
  if (!name) return NULL;
  memcpy(name, tok->value, name_len);
  name[name_len] = '\0';

  fprintf(stderr, "[LSP] lsp_hover: token='%s' type=%d\n", name, tok->type_);

  // 1b. Module alias hover — hovering "io" in "io::print" shows module info.
  if (doc->imports) {
    for (size_t i = 0; i < doc->import_count; i++) {
      ImportedModule *imp = &doc->imports[i];
      const char *alias = imp->alias ? imp->alias : imp->module_path;
      if (alias && strcmp(alias, name) == 0) {
        // Build a hover: "module io = @use \"std_io\""
        size_t buf_size = strlen(imp->module_path) + strlen(alias) + 64;
        char *plain = arena_alloc(arena, buf_size, 1);
        if (plain) {
          snprintf(plain, buf_size, "```luma\n@use \"%s\" as %s\n```\nImported module",
                   imp->module_path, alias);
          fprintf(stderr, "[LSP] lsp_hover: module alias '%s'\n", name);
          return json_escape_hover(plain, arena);
        }
      }
    }
  }

  #define MAX_SYMS 4096
  static ScopeSymbol all_syms[MAX_SYMS];
  size_t sym_count = 0;

  if (doc->scope) {
    // Separate the current file's scope from imported module scopes.
    // The current file's module scope is a direct child of global that is
    // a module scope but NOT in doc->imports.
    // Imported dependency scopes ARE in doc->imports (they have aliases).

    // Build a quick set of imported module scope pointers for O(1) exclusion.
    Scope *imported_scopes[128];
    size_t imported_count = 0;
    if (doc->imports) {
      for (size_t i = 0; i < doc->import_count && imported_count < 128; i++) {
        if (doc->imports[i].scope) {
          imported_scopes[imported_count++] = doc->imports[i].scope;
        }
      }
    }

    // Pass A: current file's own scopes (global + file module + all children
    // that are NOT imported dependency roots)
    if (doc->scope->symbols.data) {
      for (size_t i = 0; i < doc->scope->symbols.count && sym_count < MAX_SYMS; i++) {
        Symbol *sym = (Symbol *)((char *)doc->scope->symbols.data + i * sizeof(Symbol));
        if (sym && sym->name) {
          all_syms[sym_count].sym = sym;
          all_syms[sym_count].scope_name = doc->scope->scope_name ? doc->scope->scope_name : "global";
          all_syms[sym_count].is_module_scope = false;
          sym_count++;
        }
      }
    }
    if (doc->scope->children.data) {
      for (size_t i = 0; i < doc->scope->children.count; i++) {
        Scope **child_ptr = (Scope **)((char *)doc->scope->children.data + i * sizeof(Scope *));
        Scope *child = *child_ptr;
        if (!child) continue;

        // Check if this child is an imported dependency root
        bool is_imported = false;
        for (size_t j = 0; j < imported_count; j++) {
          if (imported_scopes[j] == child) { is_imported = true; break; }
        }
        collect_all_symbols(child, all_syms, &sym_count, MAX_SYMS, is_imported);
      }
    }

    fprintf(stderr, "[LSP] lsp_hover: searching %zu total symbols\n", sym_count);

    // Determine enclosing function name for tie-breaking
    const char *enc_fn = enclosing_function_name(doc, position.line);
    fprintf(stderr, "[LSP] lsp_hover: enclosing fn='%s'\n", enc_fn ? enc_fn : "?");

    // Pass A search: current-file symbols only, last match wins,
    // but an exact scope_name match beats positional order.
    Symbol *best_local = NULL;
    const char *best_local_scope = NULL;
    Symbol *best_fn_match = NULL; // matches enclosing function name exactly

    for (size_t i = 0; i < sym_count; i++) {
      if (all_syms[i].is_module_scope) continue;
      Symbol *sym = all_syms[i].sym;
      if (!sym->name || strcmp(sym->name, name) != 0) continue;

      best_local = sym;
      best_local_scope = all_syms[i].scope_name;

      // If this scope matches the enclosing function, it's the best possible
      if (enc_fn && all_syms[i].scope_name &&
          strcmp(all_syms[i].scope_name, enc_fn) == 0) {
        best_fn_match = sym;
        fprintf(stderr, "[LSP] lsp_hover: fn-match '%s' in scope '%s'\n",
                name, all_syms[i].scope_name);
      } else {
        fprintf(stderr, "[LSP] lsp_hover: local candidate '%s' in scope '%s'\n",
                name, all_syms[i].scope_name);
      }
    }

    Symbol *chosen_local = best_fn_match ? best_fn_match : best_local;
    if (chosen_local) {
      fprintf(stderr, "[LSP] lsp_hover: using local '%s' from scope '%s'\n",
              name, best_local_scope);
      const char *plain = hover_for_symbol(chosen_local, NULL, arena);
      return plain ? json_escape_hover(plain, arena) : NULL;
    }

    // Pass B search: imported module symbols
    Symbol *best_imported = NULL;
    const char *best_import_alias = NULL;
    for (size_t i = 0; i < sym_count; i++) {
      if (!all_syms[i].is_module_scope) continue;
      Symbol *sym = all_syms[i].sym;
      if (!sym->name || strcmp(sym->name, name) != 0) continue;
      if (!sym->is_public) continue; // only show public imported symbols
      best_imported = sym;
      best_import_alias = all_syms[i].scope_name;
      fprintf(stderr, "[LSP] lsp_hover: import candidate '%s' from '%s'\n",
              name, all_syms[i].scope_name);
    }
    if (best_imported) {
      fprintf(stderr, "[LSP] lsp_hover: using imported '%s' from '%s'\n",
              name, best_import_alias);
      const char *plain = hover_for_symbol(best_imported, best_import_alias, arena);
      return plain ? json_escape_hover(plain, arena) : NULL;
    }
  }

  static const struct { const char *kw; const char *desc; } keywords[] = {
    {"if",       "```luma\nif (condition) { ... }\n```\nConditional branch"},
    {"elif",     "```luma\nelif (condition) { ... }\n```\nAdditional branch"},
    {"else",     "Else branch of an if statement"},
    {"loop",     "```luma\nloop { ... }\n```\nInfinite or conditional loop"},
    {"switch",   "```luma\nswitch (value) { case -> result; }\n```\nPattern match"},
    {"return",   "Return a value from a function"},
    {"break",    "Exit the current loop"},
    {"continue", "Skip to the next loop iteration"},
    {"let",      "```luma\nlet name: Type = value;\n```\nMutable variable binding"},
    {"const",    "```luma\nconst name -> fn (...) Type { ... }\n```\nImmutable binding"},
    {"pub",      "Mark a declaration as publicly exported"},
    {"fn",       "Function type or declaration"},
    {"struct",   "Struct type declaration"},
    {"enum",     "Enum type declaration"},
    {"cast",     "```luma\ncast<Type>(value)\n```\nType cast"},
    {"sizeof",   "```luma\nsizeof<Type>\n```\nSize of a type in bytes"},
    {"alloc",    "```luma\nalloc(size)\n```\nAllocate heap memory (returns *void)"},
    {"free",     "```luma\nfree(ptr)\n```\nFree heap memory"},
    {"output",   "```luma\noutput(value)\n```\nPrint without newline"},
    {"outputln", "```luma\noutputln(value)\n```\nPrint with newline"},
    {"input",    "```luma\ninput<Type>(\"prompt\")\n```\nRead typed input"},
    {"defer",    "```luma\ndefer { ... }\n```\nRun block when scope exits"},
    {"system",   "```luma\nsystem(\"command\")\n```\nExecute a shell command"},
  };
  for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
    if (strcmp(name, keywords[i].kw) == 0) {
      fprintf(stderr, "[LSP] lsp_hover: builtin/keyword '%s'\n", name);
      return json_escape_hover(keywords[i].desc, arena);
    }
  }

  fprintf(stderr, "[LSP] lsp_hover: no hover for '%s'\n", name);
  return NULL;
}

LSPLocation *lsp_definition(LSPDocument *doc, LSPPosition position,
                            ArenaAllocator *arena) {
  if (!doc) return NULL;

  Symbol *symbol = lsp_symbol_at_position(doc, position);
  if (!symbol) return NULL;

  // Try to find the token for this symbol in the document's tokens
  // to provide an accurate definition location
  LSPLocation *loc = arena_alloc(arena, sizeof(LSPLocation), alignof(LSPLocation));
  loc->uri = doc->uri;
  loc->range.start.line = 0;
  loc->range.start.character = 0;
  loc->range.end.line = 0;
  loc->range.end.character = 0;

  if (!symbol->name || !doc->tokens) return loc;

  size_t name_len = strlen(symbol->name);

  // Scan all tokens to find where this symbol is DECLARED (first occurrence)
  // We look for the first token that matches the symbol name and is in a
  // declaration context (followed by ':' for variable, '->' for function/type)
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    if (tok->type_ == TOK_IDENTIFIER &&
        tok->length == (int)name_len &&
        strncmp(tok->value, symbol->name, name_len) == 0) {

      // Check if next significant token indicates a declaration
      for (size_t j = i + 1; j < doc->token_count && j < i + 5; j++) {
        Token *next = &doc->tokens[j];
        if (next->type_ == TOK_COLON || next->type_ == TOK_RIGHT_ARROW || next->type_ == TOK_EQUAL) {
          loc->range.start.line = (int)tok->line - 1;
          loc->range.start.character = (int)tok->col - 1;
          loc->range.end.line = (int)tok->line - 1;
          loc->range.end.character = (int)tok->col + (int)tok->length - 1;
          return loc;
        }
      }
    }
  }

  // Fallback: first occurrence of the name
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    if (tok->type_ == TOK_IDENTIFIER &&
        tok->length == (int)name_len &&
        strncmp(tok->value, symbol->name, name_len) == 0) {
      loc->range.start.line = (int)tok->line - 1;
      loc->range.start.character = (int)tok->col - 1;
      loc->range.end.line = (int)tok->line - 1;
      loc->range.end.character = (int)tok->col + (int)tok->length - 1;
      return loc;
    }
  }

  return loc;
}

LSPCompletionItem *lsp_completion(LSPDocument *doc, LSPPosition position,
                                  size_t *completion_count,
                                  ArenaAllocator *arena) {
  (void)position;
  if (!doc || !completion_count) {
    return NULL;
  }

  GrowableArray completions;
  growable_array_init(&completions, arena, 32, sizeof(LSPCompletionItem));

  const struct {
    const char *label;
    const char *snippet;
    const char *detail;
    const char *filter;
  } keywords[] = {
      {"const fn", "const ${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Function declaration", "const fn"},
      {"pub const fn",
       "pub const ${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Public function", "pub const fn"},
      {"const fn<T>",
       "const ${1:name} = fn<${2:T}>(${3:params}) ${4:Type} {\n\t$0\n}",
       "Generic function", "const fn <"},
      {"pub const fn<T>",
       "pub const ${1:name} = fn<${2:T}>(${3:params}) ${4:Type} {\n\t$0\n}",
       "Public generic function", "pub const fn <"},
      {"const struct",
       "const ${1:Name} -> struct {\n\t${2:field}: ${3:Type}$0,\n};",
       "Struct definition", "const struct"},
      {"const struct<T>",
       "const ${1:Name} -> struct<${2:T}> {\n\t${3:field}: ${4:Type}$0,\n};",
       "Generic struct", "const struct <"},
      {"const enum", "const ${1:Name} -> enum {\n\t${2:Variant}$0,\n};",
       "Enum definition", "const enum"},
      {"const var", "const ${1:name}: ${2:Type} = ${3:value};$0",
       "Top-level constant", "const"},
      {"if", "if (${1:condition}) {\n\t$0\n}", "If statement", "if"},
      {"if else", "if (${1:condition}) {\n\t${2}\n} else {\n\t$0\n}",
       "If-else statement", "if else"},
      {"elif", "elif (${1:condition}) {\n\t$0\n}", "Elif clause", "elif"},
      {"switch", "switch (${1:value}) {\n\t${2:case} -> ${3:result};$0\n}",
       "Switch statement", "switch"},
      {"switch default",
       "switch (${1:value}) {\n\t${2:case} -> ${3:result};\n\t_ -> "
       "${4:default};$0\n}",
       "Switch with default case", "switch default"},
      {"loop", "loop {\n\t$0\n}", "Infinite loop", "loop"},
      {"loop while", "loop (${1:condition}) {\n\t$0\n}", "While-style loop", "loop"},
      {"loop for",
       "loop [${1:i}: int = 0](${1:i} < ${2:10}) : (++${1:i}) {\n\t$0\n}",
       "For-style loop", "loop for"},
      {"loop for multi",
       "loop [${1:i}: int = 0, ${2:j}: int = 0](${1:i} < ${3:10}) : (++${1:i}) "
       "{\n\t$0\n}",
       "Multi-variable for loop", "loop for"},
      {"defer", "defer ${1:statement};$0",
       "Defer statement", "defer"},
      {"defer block", "defer {\n\t${1:cleanup()};$0\n}", "Defer block", "defer"},
      {"let", "let ${1:name}: ${2:Type} = ${3:value};$0",
       "Variable declaration", "let"},
      {"return", "return ${1:value};$0", "Return statement", "return"},
      {"break", "break;$0", "Break statement", "break"},
      {"continue", "continue;$0", "Continue statement", "continue"},
      {"main", "const main -> fn () int {\n\t$0\n\treturn 0;\n};",
       "Main function", "main"},
      {"outputln", "outputln(${1:message});$0", "Output with newline", "outputln"},
      {"output", "output(${1:message});$0", "Output without newline", "output"},
      {"input", "input<${1:Type}>(\"${2:prompt}\")$0", "Read typed input", "input"},
      {"system", "system(\"${1:command}\");$0", "Execute system command", "system"},
      {"cast", "cast<${1:Type}>(${2:value})$0", "Type cast", "cast"},
      {"sizeof", "sizeof<${1:Type}>$0", "Size of type", "sizeof"},
      {"alloc", "cast<${1:*Type}>(alloc(${2:size}))$0",
       "Allocate memory", "alloc"},
      {"alloc defer",
       "let ${1:ptr}: ${2:*Type} = cast<${2:*Type}>(alloc(${3:size}));\n"
       "defer free(${1:ptr});$0",
       "Allocate with defer cleanup", "alloc"},
      {"@module", "@module \"${1:name}\"$0", "Module declaration", "@module"},
      {"@use", "@use \"${1:module}\" as ${2:alias}$0", "Import module", "@use"},
      {"@os", "@os {\n\t\"${1:linux}\" -> {\n\t\t$0\n\t}\n}", "Platform directive", "@os"},
      {"@link", "@link(\"${1:lib.so}\")$0", "Link shared library", "@link"},
      {"struct method", "${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Struct method", "fn"},
      {"struct pub", "pub:\n\t${1:field}: ${2:Type},$0",
       "Public struct fields", "pub"},
      {"struct priv", "priv:\n\t${1:field}: ${2:Type},$0",
       "Private struct fields", "priv"},
      {"array", "[${1:Type}; ${2:size}]$0", "Array type", "["},
      {"array init", "let ${1:arr}: [${2:Type}; ${3:size}] = [${4:values}];$0",
       "Array initialization", "["},
      {"pointer", "*${1:Type}$0", "Pointer type", "*"},
      {"address of", "&${1:variable}$0", "Address-of operator", "&"},
      {"dereference", "*${1:pointer}$0", "Dereference pointer", "*"},
      {"#returns_ownership",
       "#returns_ownership\nconst ${1:name} -> fn (${2:params}) ${3:*Type} "
       "{\n\t$0\n}",
       "Function returns owned pointer", "#returns_ownership"},
      {"#takes_ownership",
       "#takes_ownership\nconst ${1:name} -> fn (${2:ptr}: ${3:*Type}) void "
       "{\n\t$0\n}",
       "Function takes ownership", "#takes_ownership"},
      {"#lib_import",
       "#lib_import(\"${1:lib}.so\")\npub const ${2:name} -> fn (${3:params}) ${4:Type};",
       "Per-function library import (POSIX)", "#lib_import"},
      {"#dll_import",
       "#dll_import(\"${1:dll}.dll\", callconv: \"stdcall\")\npub const ${2:name} -> fn (${3:params}) ${4:Type};",
       "Per-function DLL import (Windows)", "#dll_import"},
  };

  for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
    LSPCompletionItem *item =
        (LSPCompletionItem *)growable_array_push(&completions);
    if (item) {
      item->label = arena_strdup(arena, keywords[i].label);
      item->kind = LSP_COMPLETION_SNIPPET;
      item->insert_text = arena_strdup(arena, keywords[i].snippet);
      item->format = LSP_INSERT_FORMAT_SNIPPET;
      item->detail = arena_strdup(arena, keywords[i].detail);
      item->documentation = NULL;
      // Snippets get low sort priority — show after symbols
      item->sort_text = arena_strdup(arena, "z");
      // Set filter text so "fn" matches "const fn", "struct fn" etc.
      item->filter_text = keywords[i].filter ? arena_strdup(arena, keywords[i].filter) : NULL;
    }
  }

  // Keyword completions (true/false/null) as keyword type
  static const char *keyword_items[][3] = {
    {"true", "true", "Boolean literal"},
    {"false", "false", "Boolean literal"},
    {"null", "null", "Null pointer literal"},
  };
  for (size_t i = 0; i < 3; i++) {
    LSPCompletionItem *item =
        (LSPCompletionItem *)growable_array_push(&completions);
    if (item) {
      item->label = arena_strdup(arena, keyword_items[i][0]);
      item->kind = LSP_COMPLETION_KEYWORD;
      item->insert_text = arena_strdup(arena, keyword_items[i][1]);
      item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
      item->detail = arena_strdup(arena, keyword_items[i][2]);
      item->documentation = NULL;
      item->sort_text = arena_strdup(arena, "a");
      item->filter_text = arena_strdup(arena, keyword_items[i][0]);
    }
  }

  if (doc->scope) {
    // Build a set of imported module scopes so we can filter their
    // private symbols during the BFS below.  Those symbols are shown
    // separately (with a "module::" prefix) in the imports section,
    // so we only want public ones here.
    Scope *imported_scopes[128];
    size_t imported_scope_count = 0;
    if (doc->imports) {
      for (size_t i = 0; i < doc->import_count && imported_scope_count < 128; i++) {
        if (doc->imports[i].scope)
          imported_scopes[imported_scope_count++] = doc->imports[i].scope;
      }
    }

    // BFS over the full scope tree so variables at every depth appear.
    Scope *to_process[256];
    int to_process_count = 0;
    int depth[256];

    to_process[0] = doc->scope;
    depth[0] = 0;
    to_process_count = 1;

    for (int i = 0; i < to_process_count; i++) {
      Scope *sc = to_process[i];
      int sc_depth = depth[i];

      // Is this scope an imported module root?
      bool is_imported_scope = false;
      for (size_t k = 0; k < imported_scope_count; k++) {
        if (imported_scopes[k] == sc) { is_imported_scope = true; break; }
      }

      if (sc->symbols.data && sc->symbols.count > 0) {
        for (size_t j = 0; j < sc->symbols.count; j++) {
          Symbol *sym = (Symbol *)((char *)sc->symbols.data +
                                   j * sizeof(Symbol));
          if (!sym || !sym->name || !sym->type) continue;
          // Only show public symbols from imported module scopes
          if (is_imported_scope && !sym->is_public) continue;

          LSPCompletionItem *item =
              (LSPCompletionItem *)growable_array_push(&completions);
          if (!item) continue;

          item->label = arena_strdup(arena, sym->name);

          if (sym->type->type == AST_TYPE_FUNCTION) {
            item->kind = LSP_COMPLETION_FUNCTION;
            size_t snippet_size = strlen(sym->name) + 16;
            char *snippet = arena_alloc(arena, snippet_size, 1);
            if (snippet) {
              snprintf(snippet, snippet_size, "%s($0)", sym->name);
              item->insert_text = snippet;
              item->format = LSP_INSERT_FORMAT_SNIPPET;
            } else {
              item->insert_text = arena_strdup(arena, sym->name);
              item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
            }
          } else if (sym->type->type == AST_TYPE_STRUCT) {
            item->kind = LSP_COMPLETION_STRUCT;
            item->insert_text = arena_strdup(arena, sym->name);
            item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
          } else {
            item->kind = LSP_COMPLETION_VARIABLE;
            item->insert_text = arena_strdup(arena, sym->name);
            item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
          }

          item->detail = type_to_string(sym->type, arena);
          char sort[16];
          snprintf(sort, sizeof(sort), "%d", sc_depth);
          item->sort_text = arena_strdup(arena, sort);
          item->documentation = NULL;
          item->filter_text = arena_strdup(arena, sym->name);
        }
      }

      // Walk parent chain (outer scopes are "higher" depth)
      if (sc->parent && to_process_count < 256) {
        bool already = false;
        for (int k = 0; k < to_process_count; k++) {
          if (to_process[k] == sc->parent) { already = true; break; }
        }
        if (!already) {
          to_process[to_process_count] = sc->parent;
          depth[to_process_count] = sc_depth + 1;
          to_process_count++;
        }
      }

      // Walk children (inner scopes are "lower" depth)
      if (sc->children.data && sc->children.count > 0) {
        for (size_t k = 0; k < sc->children.count && to_process_count < 256; k++) {
          Scope *child = *(Scope **)((char *)sc->children.data + k * sizeof(Scope *));
          if (!child) continue;
          bool already = false;
          for (int m = 0; m < to_process_count; m++) {
            if (to_process[m] == child) { already = true; break; }
          }
          if (!already) {
            to_process[to_process_count] = child;
            depth[to_process_count] = sc_depth + 1;
            to_process_count++;
          }
        }
      }
    }
  }

  if (doc->imports && doc->import_count > 0) {
    for (size_t i = 0; i < doc->import_count; i++) {
      ImportedModule *import = &doc->imports[i];

      if (!import->scope || !import->scope->symbols.data) continue;

      const char *prefix = import->alias ? import->alias : "module";

      for (size_t j = 0; j < import->scope->symbols.count; j++) {
        Symbol *sym = (Symbol *)((char *)import->scope->symbols.data +
                                 j * sizeof(Symbol));

        if (!sym || !sym->name || !sym->type || !sym->is_public) continue;
        if (strncmp(sym->name, "__", 2) == 0) continue;

        bool is_struct = (sym->type->type == AST_TYPE_STRUCT);
        bool is_enum   = (sym->type->type == AST_TYPE_ENUM);
        bool needs_prefix = !is_struct && !is_enum;

        LSPCompletionItem *item =
            (LSPCompletionItem *)growable_array_push(&completions);
        if (item) {
          if (needs_prefix) {
            size_t label_len = strlen(prefix) + strlen(sym->name) + 3;
            char *label = arena_alloc(arena, label_len, 1);
            snprintf(label, label_len, "%s::%s", prefix, sym->name);
            item->label = label;
            item->insert_text = arena_strdup(arena, label);
          } else {
            item->label = arena_strdup(arena, sym->name);
            item->insert_text = arena_strdup(arena, sym->name);
          }

          if (sym->type->type == AST_TYPE_FUNCTION) {
            item->kind = LSP_COMPLETION_FUNCTION;
          } else if (is_struct) {
            item->kind = LSP_COMPLETION_STRUCT;
          } else {
            item->kind = LSP_COMPLETION_VARIABLE;
          }

          item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
          item->detail = type_to_string(sym->type, arena);
          item->documentation = NULL;
          item->sort_text = arena_strdup(arena, "9");
          item->filter_text = arena_strdup(arena, sym->name);
        }
      }
    }
  }

  *completion_count = completions.count;
  return (LSPCompletionItem *)completions.data;
}

LSPCompletionItem *lsp_completion_resolve(LSPCompletionItem *item,
                                          ArenaAllocator *arena) {
  (void)arena;
  if (!item) return NULL;
  return item;
}

LSPSignatureInfo *lsp_signature_help(LSPDocument *doc, LSPPosition position,
                                     size_t *signature_count,
                                     ArenaAllocator *arena) {
  if (!doc || !signature_count) return NULL;

  // Find the token at the cursor
  Token *tok = find_token_at(doc, position);
  if (!tok) return NULL;

  // Scan backwards for the function call opening paren
  int target_line = position.line;
  int target_col = position.character;

  // Look for an identifier before '(' — simplest approach:
  // find the nearest '(' before cursor on the same line,
  // then check if there's an identifier before it
  const char *content = doc->content;
  if (!content) return NULL;

  // Convert position to offset in content
  int line = 0, col = 0;
  const char *p = content;
  while (*p && line < target_line) {
    if (*p == '\n') { line++; col = 0; }
    else { col++; }
    p++;
  }
  // p now points to the start of the target line
  const char *line_start = p;
  // advance to target column
  while (*p && col < target_col) {
    if (*p == '\n') break;
    col++;
    p++;
  }

  // Scan backwards from cursor to find '(' and then function name
  const char *scan = p;
  while (scan > line_start) {
    scan--;
    if (*scan == '(') {
      // Found '(' — now look for identifier before it
      const char *name_scan = scan - 1;
      // Skip whitespace
      while (name_scan >= line_start && (*name_scan == ' ' || *name_scan == '\t'))
        name_scan--;
      // Find end of identifier
      const char *id_end = name_scan + 1;
      while (name_scan >= line_start &&
             ((*name_scan >= 'a' && *name_scan <= 'z') ||
              (*name_scan >= 'A' && *name_scan <= 'Z') ||
              *name_scan == '_' ||
              (*name_scan >= '0' && *name_scan <= '9')))
        name_scan--;
      const char *id_start = name_scan + 1;
      size_t id_len = (size_t)(id_end - id_start);

      if (id_len > 0 && id_len < 256) {
        char fn_name[256];
        memcpy(fn_name, id_start, id_len);
        fn_name[id_len] = '\0';

        // Count how many commas are inside the parens (= active parameter)
        size_t active_param = 0;
        const char *cp = scan + 1;
        int depth = 1;
        while (*cp && depth > 0 && cp < p) {
          if (*cp == '(') depth++;
          else if (*cp == ')') depth--;
          else if (*cp == ',' && depth == 1) active_param++;
          cp++;
        }

        // Look up the function in the scope
        if (doc->scope) {
          Symbol *fn_sym = scope_lookup(doc->scope, fn_name);
          if (fn_sym && fn_sym->type && fn_sym->type->type == AST_TYPE_FUNCTION) {
            AstNode *func = fn_sym->type;
            LSPSignatureInfo *sig = arena_alloc(arena, sizeof(LSPSignatureInfo), alignof(LSPSignatureInfo));
            if (!sig) return NULL;

            // Build the label: fn_name(param1, param2, ...) -> return_type
            size_t label_size = 1024;
            char *label = arena_alloc(arena, label_size, 1);
            if (!label) return NULL;
            snprintf(label, label_size, "%s(", fn_name);

            size_t param_count = func->type_data.function.param_count;
            for (size_t i = 0; i < param_count; i++) {
              if (i > 0) strncat(label, ", ", label_size - strlen(label) - 1);
              const char *ptype = func->type_data.function.param_types[i]
                ? type_to_string(func->type_data.function.param_types[i], arena)
                : "?";
              char param_str[64];
              snprintf(param_str, sizeof(param_str), "p%zu: %s", i + 1, ptype);
              strncat(label, param_str, label_size - strlen(label) - 1);
            }

            const char *ret = func->type_data.function.return_type
              ? type_to_string(func->type_data.function.return_type, arena)
              : "void";
            char ret_str[64];
            snprintf(ret_str, sizeof(ret_str), ") %s", ret);
            strncat(label, ret_str, label_size - strlen(label) - 1);

            sig->label = label;
            sig->documentation = NULL;
            sig->active_parameter = active_param < param_count ? active_param : 0;

            // Build parameter info
            if (param_count > 0) {
              sig->parameters = arena_alloc(arena, param_count * sizeof(LSPParameterInfo), alignof(LSPParameterInfo));
              sig->parameter_count = param_count;
              for (size_t i = 0; i < param_count; i++) {
                const char *ptype = func->type_data.function.param_types[i]
                  ? type_to_string(func->type_data.function.param_types[i], arena)
                  : "?";
                char param_label[64];
                snprintf(param_label, sizeof(param_label), "p%zu: %s", i + 1, ptype);
                char param_doc[128];
                snprintf(param_doc, sizeof(param_doc), "Parameter %zu: %s", i + 1, ptype);
                sig->parameters[i].label = arena_strdup(arena, param_label);
                sig->parameters[i].documentation = arena_strdup(arena, param_doc);
              }
            } else {
              sig->parameters = NULL;
              sig->parameter_count = 0;
            }

            *signature_count = 1;
            return sig;
          }
        }
      }
      break;
    }
    if (*scan == '\n' || *scan == ';' || *scan == '{' || *scan == '}') break;
  }

  *signature_count = 0;
  return NULL;
}

LSPCodeAction *lsp_code_action(LSPDocument *doc, LSPPosition position,
                               size_t *action_count, ArenaAllocator *arena) {
  if (!doc || !action_count) return NULL;

  // Provide basic code actions based on diagnostics at the position
  Token *tok = find_token_at(doc, position);
  if (!tok || !token_is_name_like(tok)) {
    *action_count = 0;
    return NULL;
  }

  // Check for common issues at the cursor position
  GrowableArray actions;
  growable_array_init(&actions, arena, 4, sizeof(LSPCodeAction));

  size_t name_len = tok->length;
  char *name = arena_alloc(arena, name_len + 1, 1);
  if (name) {
    memcpy(name, tok->value, name_len);
    name[name_len] = '\0';

    // Check if user typed something that looks like it should be public
    if (doc->scope) {
      Symbol *sym = scope_lookup(doc->scope, name);
      if (sym && !sym->is_public && doc->import_count > 0) {
        LSPCodeAction *action =
            (LSPCodeAction *)growable_array_push(&actions);
        if (action) {
          action->title = arena_strdup(arena, "Make 'pub'");
          action->kind = "quickfix";
          action->command = NULL;
          action->edit_title = NULL;
          action->edit_text = NULL;
          action->edit_uri = NULL;
        }
      }
    }
  }

  *action_count = actions.count;
  return (LSPCodeAction *)actions.data;
}

LSPDocumentHighlight *lsp_document_highlight(LSPDocument *doc,
                                             LSPPosition position,
                                             size_t *highlight_count,
                                             ArenaAllocator *arena) {
  if (!doc || !highlight_count || !doc->tokens) return NULL;

  // Find the token at the cursor
  Token *tok = find_token_at(doc, position);
  if (!tok || !token_is_name_like(tok) || !tok->value || tok->length == 0) {
    *highlight_count = 0;
    return NULL;
  }

  // Extract the name
  size_t name_len = tok->length;
  char *name = arena_alloc(arena, name_len + 1, 1);
  if (!name) { *highlight_count = 0; return NULL; }
  memcpy(name, tok->value, name_len);
  name[name_len] = '\0';

  // Count all matching identifiers
  size_t count = 0;
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *t = &doc->tokens[i];
    if (t->type_ == TOK_IDENTIFIER && t->length == (int)name_len &&
        strncmp(t->value, name, name_len) == 0) {
      count++;
    }
  }

  if (count == 0) { *highlight_count = 0; return NULL; }

  LSPDocumentHighlight *highlights =
      arena_alloc(arena, count * sizeof(LSPDocumentHighlight),
                  alignof(LSPDocumentHighlight));
  if (!highlights) { *highlight_count = 0; return NULL; }

  size_t idx = 0;
  for (size_t i = 0; i < doc->token_count && idx < count; i++) {
    Token *t = &doc->tokens[i];
    if (t->type_ == TOK_IDENTIFIER && t->length == (int)name_len &&
        strncmp(t->value, name, name_len) == 0) {
      highlights[idx].range.start.line = (int)t->line - 1;
      highlights[idx].range.start.character = (int)t->col - 1;
      highlights[idx].range.end.line = (int)t->line - 1;
      highlights[idx].range.end.character = (int)t->col + (int)t->length - 1;
      highlights[idx].kind = LSP_HIGHLIGHT_TEXT;
      idx++;
    }
  }

  *highlight_count = idx;
  return highlights;
}

const char *lsp_rename(LSPDocument *doc, LSPPosition position,
                       const char *new_name, ArenaAllocator *arena) {
  if (!doc || !new_name || !doc->tokens) return NULL;

  Token *tok = find_token_at(doc, position);
  if (!tok || !token_is_name_like(tok) || !tok->value || tok->length == 0)
    return NULL;

  // Extract old name
  size_t name_len = tok->length;
  char *old_name = arena_alloc(arena, name_len + 1, 1);
  if (!old_name) return NULL;
  memcpy(old_name, tok->value, name_len);
  old_name[name_len] = '\0';

  // Build edit entries for each occurrence of the old name
  // Output format: "\"uri\":[{edit1},{edit2},...]"
  size_t buf_size = 65536;
  char *result = arena_alloc(arena, buf_size, 1);
  if (!result) return NULL;
  result[0] = '\0';

  // Count matches first
  size_t match_count = 0;
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *t = &doc->tokens[i];
    if (t->type_ == TOK_IDENTIFIER && t->length == (int)name_len &&
        strncmp(t->value, old_name, name_len) == 0) {
      match_count++;
    }
  }

  if (match_count == 0) return NULL;

  // Build edits for this document
  char edits_section[65536];
  size_t off = 0;
  off += snprintf(edits_section + off, sizeof(edits_section) - off,
                  "\"%s\":[", doc->uri);

  bool first = true;
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *t = &doc->tokens[i];
    if (t->type_ == TOK_IDENTIFIER && t->length == (int)name_len &&
        strncmp(t->value, old_name, name_len) == 0) {
      if (!first)
        off += snprintf(edits_section + off, sizeof(edits_section) - off, ",");
      first = false;

      char escaped_new[256];
      size_t esc_idx = 0;
      for (const char *s = new_name; *s && esc_idx < sizeof(escaped_new) - 1; s++) {
        if (*s == '"' || *s == '\\') {
          escaped_new[esc_idx++] = '\\';
        }
        escaped_new[esc_idx++] = *s;
      }
      escaped_new[esc_idx] = '\0';

      off += snprintf(edits_section + off, sizeof(edits_section) - off,
                      "{\"range\":{"
                      "\"start\":{\"line\":%d,\"character\":%d},"
                      "\"end\":{\"line\":%d,\"character\":%d}"
                      "},\"newText\":\"%s\"}",
                      (int)t->line - 1, (int)t->col - 1,
                      (int)t->line - 1, (int)t->col + (int)t->length - 1,
                      escaped_new);
    }
  }
  off += snprintf(edits_section + off, sizeof(edits_section) - off, "]");

  return arena_strdup(arena, edits_section);
}