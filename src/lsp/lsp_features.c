#include "lsp.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Internal: JSON-escape a string into a static arena buffer.
// Double-backslash and double-quote so the result is safe inside a JSON string.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Internal: find token at a 0-based LSP position.
// Tokens are 1-based (confirmed by log analysis); LSP positions are 0-based.
// ---------------------------------------------------------------------------
static Token *find_token_at(LSPDocument *doc, LSPPosition pos) {
  if (!doc || !doc->tokens) return NULL;
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    int tok_line = (int)tok->line - 1;
    int tok_col  = (int)tok->col  - 1;
    int tok_end  = tok_col + (int)tok->length;
    if (tok_line == pos.line &&
        tok_col <= pos.character && pos.character < tok_end) {
      return tok;
    }
  }
  return NULL;
}

// Returns true for tokens that carry a word-like name: identifiers AND
// keyword/builtin tokens (alloc, outputln, let, if, ...) whose text starts
// with a letter or underscore.
static bool token_is_name_like(Token *tok) {
  if (!tok || !tok->value || tok->length == 0) return false;
  if (tok->type_ == TOK_IDENTIFIER) return true;
  char c = tok->value[0];
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

// ---------------------------------------------------------------------------
// Internal: build a markdown hover string for a symbol.
// ---------------------------------------------------------------------------
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
    default:
      kw = sym->is_mutable ? "let" : "const";
      break;
    }
  }

  // Build the code line shown in the hover
  char code[512];
  if (module_alias) {
    snprintf(code, sizeof(code), "%s::%s: %s", module_alias, sym->name, type_str);
  } else {
    snprintf(code, sizeof(code), "%s %s: %s", kw, sym->name, type_str);
  }

  // Build visibility/mutability tags
  char tags[128] = "";
  if (sym->is_public)   strncat(tags, "public ", sizeof(tags) - strlen(tags) - 1);
  if (sym->is_mutable)  strncat(tags, "mutable ", sizeof(tags) - strlen(tags) - 1);

  // Final markdown
  size_t buf_size = strlen(code) + strlen(tags) + 128;
  char *result = arena_alloc(arena, buf_size, 1);
  if (!result) return NULL;

  if (strlen(tags) > 0) {
    snprintf(result, buf_size, "```luma\n%s\n```\n*%s*", code, tags);
  } else {
    snprintf(result, buf_size, "```luma\n%s\n```", code);
  }

  return result;  // caller applies json_escape_hover()
}

// ---------------------------------------------------------------------------
// lsp_hover helpers
// ---------------------------------------------------------------------------

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
  // Walk backwards from the cursor looking for a token on an earlier line
  // that is an identifier immediately preceded by '->' or 'fn' pattern.
  // Simpler heuristic: find the last function-scope name in the scope tree
  // whose associated_node->line is <= cursor line+1 (1-based).
  // We don't have that info reliably, so just scan tokens for the pattern:
  //   IDENTIFIER  '->'  'fn'  '('   ... which is a Luma function declaration.
  // Walk backwards from cursor line.
  const char *best_fn = NULL;
  for (size_t i = 0; i < doc->token_count; i++) {
    Token *tok = &doc->tokens[i];
    int tok_line = (int)tok->line - 1; // convert to 0-based
    if (tok_line > line_0based) break;
    // Look for pattern: identifier followed by '->' on same or next token
    // In Luma: "const NAME -> fn ..."
    // We detect by finding identifiers whose next non-ws token is '->'
    if (tok->type_ == TOK_IDENTIFIER && tok->length > 0 && tok->length < 64) {
      // Peek ahead for '->'
      if (i + 1 < doc->token_count) {
        Token *next = &doc->tokens[i + 1];
        // token type for '->' — check value
        if (next->length == 2 && next->value &&
            next->value[0] == '-' && next->value[1] == '>') {
          // This looks like a declaration — record as potential function name
          char tmp[65];
          memcpy(tmp, tok->value, tok->length);
          tmp[tok->length] = '\0';
          best_fn = tok->value; // points into token array (stable)
        }
      }
    }
  }
  return best_fn;
}

// lsp_hover
// ---------------------------------------------------------------------------
const char *lsp_hover(LSPDocument *doc, LSPPosition position,
                      ArenaAllocator *arena) {
  if (!doc) return NULL;

  // 1. Find the token at the cursor
  Token *tok = find_token_at(doc, position);
  if (!tok || !token_is_name_like(tok)) return NULL;

  char name[256];
  size_t len = tok->length < 255 ? tok->length : 255;
  memcpy(name, tok->value, len);
  name[len] = '\0';

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

  // 2. Collect symbols from the document's OWN scope tree.
  //    doc->scope is the global scope; its children include:
  //      - the current file's module scope (and its nested function/block scopes)
  //      - imported module scopes (registered as children of global)
  //    We want to search the current file FIRST before imported modules so that
  //    a local `buf` beats a stdlib `buf`.
  //
  //    Strategy: two passes.
  //      Pass A — only non-module-scope children of doc->scope (the current file).
  //      Pass B — module-scope children (imported dependencies).
  //    Within each pass, prefer the LAST match (deepest/most-local scope wins).
  //    Additionally, among same-named candidates, prefer the one whose
  //    scope_name matches the enclosing function at the cursor.

  #define MAX_SYMS 4096
  static ScopeSymbol all_syms[MAX_SYMS]; // static to avoid 64KB stack frame
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

  // 3. Keywords and built-ins
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
// ---------------------------------------------------------------------------
// lsp_definition
// ---------------------------------------------------------------------------
LSPLocation *lsp_definition(LSPDocument *doc, LSPPosition position,
                            ArenaAllocator *arena) {
  if (!doc) return NULL;

  Symbol *symbol = lsp_symbol_at_position(doc, position);
  if (!symbol) return NULL;

  LSPLocation *loc = arena_alloc(arena, sizeof(LSPLocation), alignof(LSPLocation));
  loc->uri = doc->uri;
  loc->range.start.line = position.line;
  loc->range.start.character = 0;
  loc->range.end.line = position.line;
  loc->range.end.character = 100;

  return loc;
}

// ---------------------------------------------------------------------------
// lsp_completion
// ---------------------------------------------------------------------------
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
  } keywords[] = {
      {"const fn", "const ${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Function declaration"},
      {"pub const fn",
       "pub const ${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Public function"},
      {"const fn<T>",
       "const ${1:name} = fn<${2:T}>(${3:params}) ${4:Type} {\n\t$0\n}",
       "Generic function"},
      {"pub const fn<T>",
       "pub const ${1:name} = fn<${2:T}>(${3:params}) ${4:Type} {\n\t$0\n}",
       "Public generic function"},
      {"const struct",
       "const ${1:Name} -> struct {\n\t${2:field}: ${3:Type}$0,\n};",
       "Struct definition"},
      {"const struct<T>",
       "const ${1:Name} -> struct<${2:T}> {\n\t${3:field}: ${4:Type}$0,\n};",
       "Generic struct"},
      {"const enum", "const ${1:Name} -> enum {\n\t${2:Variant}$0,\n};",
       "Enum definition"},
      {"const var", "const ${1:name}: ${2:Type} = ${3:value};$0",
       "Top-level constant"},
      {"if", "if (${1:condition}) {\n\t$0\n}", "If statement"},
      {"if else", "if (${1:condition}) {\n\t${2}\n} else {\n\t$0\n}",
       "If-else statement"},
      {"elif", "elif (${1:condition}) {\n\t$0\n}", "Elif clause"},
      {"loop", "loop {\n\t$0\n}", "Infinite loop"},
      {"loop while", "loop (${1:condition}) {\n\t$0\n}", "While-style loop"},
      {"loop for",
       "loop [${1:i}: int = 0](${1:i} < ${2:10}) : (++${1:i}) {\n\t$0\n}",
       "For-style loop"},
      {"loop for multi",
       "loop [${1:i}: int = 0, ${2:j}: int = 0](${1:i} < ${3:10}) : (++${1:i}) "
       "{\n\t$0\n}",
       "Multi-variable for loop"},
      {"switch", "switch (${1:value}) {\n\t${2:case} -> ${3:result};$0\n}",
       "Switch statement"},
      {"switch default",
       "switch (${1:value}) {\n\t${2:case} -> ${3:result};\n\t_ -> "
       "${4:default};$0\n}",
       "Switch with default case"},
      {"let", "let ${1:name}: ${2:Type} = ${3:value};$0",
       "Variable declaration"},
      {"defer block", "defer {\n\t${1:cleanup()};$0\n}", "Defer block"},
      {"@module", "@module \"${1:name}\"$0", "Module declaration"},
      {"@use", "@use \"${1:module}\" as ${2:alias}$0", "Import module"},
      {"return", "return ${1:value};$0", "Return statement"},
      {"break", "break;$0", "Break statement"},
      {"continue", "continue;$0", "Continue statement"},
      {"main", "const main -> fn () int {\n\t$0\n\treturn 0;\n};",
       "Main function"},
      {"outputln", "outputln(${1:message});$0", "Output with newline"},
      {"output", "output(${1:message});$0", "Output without newline"},
      {"input", "input<${1:Type}>(\"${2:prompt}\")$0", "Read typed input"},
      {"system", "system(\"${1:command}\");$0", "Execute system command"},
      {"cast", "cast<${1:Type}>(${2:value})$0", "Type cast"},
      {"sizeof", "sizeof<${1:Type}>$0", "Size of type"},
      {"alloc", "cast<${1:*Type}>(alloc(${2:size} * sizeof<${3:Type}>))$0",
       "Allocate memory"},
      {"alloc defer",
       "let ${1:ptr}: ${2:*Type} = cast<${2:*Type}>(alloc(${3:size} * "
       "sizeof<${4:Type}>));\ndefer free(${1:ptr});$0",
       "Allocate with defer cleanup"},
      {"struct method", "${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Struct method"},
      {"struct pub", "pub:\n\t${1:field}: ${2:Type},$0",
       "Public struct fields"},
      {"struct priv", "priv:\n\t${1:field}: ${2:Type},$0",
       "Private struct fields"},
      {"array", "[${1:Type}; ${2:size}]$0", "Array type"},
      {"array init", "let ${1:arr}: [${2:Type}; ${3:size}] = [${4:values}];$0",
       "Array initialization"},
      {"pointer", "*${1:Type}$0", "Pointer type"},
      {"address of", "&${1:variable}$0", "Address-of operator"},
      {"dereference", "*${1:pointer}$0", "Dereference pointer"},
      {"#returns_ownership",
       "#returns_ownership\nconst ${1:name} -> fn (${2:params}) ${3:*Type} "
       "{\n\t$0\n}",
       "Function returns owned pointer"},
      {"#takes_ownership",
       "#takes_ownership\nconst ${1:name} -> fn (${2:ptr}: ${3:*Type}) void "
       "{\n\t$0\n}",
       "Function takes ownership"},
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
      item->sort_text = NULL;
      item->filter_text = NULL;
    }
  }

  Scope *local_scope = doc->scope;

  if (local_scope) {
    Scope *current_scope = local_scope;
    int scope_depth = 0;

    while (current_scope) {
      if (current_scope->symbols.data && current_scope->symbols.count > 0) {
        for (size_t i = 0; i < current_scope->symbols.count; i++) {
          Symbol *sym = (Symbol *)((char *)current_scope->symbols.data +
                                   i * sizeof(Symbol));

          if (!sym || !sym->name || !sym->type) continue;

          LSPCompletionItem *item =
              (LSPCompletionItem *)growable_array_push(&completions);
          if (item) {
            item->label = arena_strdup(arena, sym->name);

            if (sym->type->type == AST_TYPE_FUNCTION) {
              item->kind = LSP_COMPLETION_FUNCTION;
              char snippet[512];
              snprintf(snippet, sizeof(snippet), "%s()$0", sym->name);
              item->insert_text = arena_strdup(arena, snippet);
              item->format = LSP_INSERT_FORMAT_SNIPPET;
              item->detail = type_to_string(sym->type, arena);
            } else if (sym->type->type == AST_TYPE_STRUCT) {
              item->kind = LSP_COMPLETION_STRUCT;
              item->insert_text = arena_strdup(arena, sym->name);
              item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
              item->detail = type_to_string(sym->type, arena);
            } else {
              item->kind = LSP_COMPLETION_VARIABLE;
              item->insert_text = arena_strdup(arena, sym->name);
              item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
              item->detail = type_to_string(sym->type, arena);
            }

            char sort[8];
            snprintf(sort, sizeof(sort), "%d", scope_depth);
            item->sort_text = arena_strdup(arena, sort);
            item->documentation = NULL;
            item->filter_text = NULL;
          }
        }
      }

      current_scope = current_scope->parent;
      scope_depth++;
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

        LSPCompletionItem *item =
            (LSPCompletionItem *)growable_array_push(&completions);
        if (item) {
          size_t label_len = strlen(prefix) + strlen(sym->name) + 3;
          char *label = arena_alloc(arena, label_len, 1);
          snprintf(label, label_len, "%s::%s", prefix, sym->name);

          item->label = label;
          item->kind = (sym->type->type == AST_TYPE_FUNCTION)
                           ? LSP_COMPLETION_FUNCTION
                           : LSP_COMPLETION_VARIABLE;
          item->insert_text = arena_strdup(arena, label);
          item->format = LSP_INSERT_FORMAT_PLAIN_TEXT;
          item->detail = type_to_string(sym->type, arena);
          item->documentation = NULL;
          item->sort_text = arena_strdup(arena, "9");
          item->filter_text = NULL;
        }
      }
    }
  }

  *completion_count = completions.count;
  return (LSPCompletionItem *)completions.data;
}