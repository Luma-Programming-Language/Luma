#include "lsp.h"

const char *lsp_hover(LSPDocument *doc, LSPPosition position,
                      ArenaAllocator *arena) {
  if (!doc)
    return NULL;

  Symbol *symbol = lsp_symbol_at_position(doc, position);
  if (!symbol)
    return NULL;

  const char *type_str = type_to_string(symbol->type, arena);
  size_t len = strlen(symbol->name) + strlen(type_str) + 50;
  char *hover = arena_alloc(arena, len, 1);

  snprintf(hover, len, "```\\n%s: %s\\n```\\n%s%s", symbol->name, type_str,
           symbol->is_public ? "public " : "",
           symbol->is_mutable ? "mutable" : "immutable");

  return hover;
}

LSPLocation *lsp_definition(LSPDocument *doc, LSPPosition position,
                            ArenaAllocator *arena) {
  if (!doc)
    return NULL;

  Symbol *symbol = lsp_symbol_at_position(doc, position);
  if (!symbol)
    return NULL;

  LSPLocation *loc =
      arena_alloc(arena, sizeof(LSPLocation), alignof(LSPLocation));
  loc->uri = doc->uri;
  loc->range.start.line = position.line;
  loc->range.start.character = 0;
  loc->range.end.line = position.line;
  loc->range.end.character = 100;

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
  } keywords[] = {
      // Function declarations
      {"const fn", "const ${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Function declaration"},
      {"pub const fn",
       "pub const ${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Public function"},

      // Generic functions
      {"const fn<T>",
       "const ${1:name} = fn<${2:T}>(${3:params}) ${4:Type} {\n\t$0\n}",
       "Generic function"},
      {"pub const fn<T>",
       "pub const ${1:name} = fn<${2:T}>(${3:params}) ${4:Type} {\n\t$0\n}",
       "Public generic function"},

      // Type declarations
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

      // Control flow - if/elif/else
      {"if", "if (${1:condition}) {\n\t$0\n}", "If statement"},
      {"if else", "if (${1:condition}) {\n\t${2}\n} else {\n\t$0\n}",
       "If-else statement"},
      {"elif", "elif (${1:condition}) {\n\t$0\n}", "Elif clause"},

      // Loop patterns
      {"loop", "loop {\n\t$0\n}", "Infinite loop"},
      {"loop while", "loop (${1:condition}) {\n\t$0\n}", "While-style loop"},
      {"loop for",
       "loop [${1:i}: int = 0](${1:i} < ${2:10}) : (++${1:i}) {\n\t$0\n}",
       "For-style loop"},
      {"loop for multi",
       "loop [${1:i}: int = 0, ${2:j}: int = 0](${1:i} < ${3:10}) : (++${1:i}) "
       "{\n\t$0\n}",
       "Multi-variable for loop"},

      // Switch patterns
      {"switch", "switch (${1:value}) {\n\t${2:case} -> ${3:result};$0\n}",
       "Switch statement"},
      {"switch default",
       "switch (${1:value}) {\n\t${2:case} -> ${3:result};\n\t_ -> "
       "${4:default};$0\n}",
       "Switch with default case"},

      // Variable declaration
      {"let", "let ${1:name}: ${2:Type} = ${3:value};$0",
       "Variable declaration"},

      // Defer patterns
      {"defer block", "defer {\n\t${1:cleanup()};$0\n}", "Defer block"},

      // Module system
      {"@module", "@module \"${1:name}\"$0", "Module declaration"},
      {"@use", "@use \"${1:module}\" as ${2:alias}$0", "Import module"},

      // Flow control
      {"return", "return ${1:value};$0", "Return statement"},
      {"break", "break;$0", "Break statement"},
      {"continue", "continue;$0", "Continue statement"},

      // Common functions
      {"main", "const main -> fn () int {\n\t$0\n\treturn 0;\n};",
       "Main function"},
      {"outputln", "outputln(${1:message});$0", "Output with newline"},
      {"output", "output(${1:message});$0", "Output without newline"},

      // Built-in functions
      {"input", "input<${1:Type}>(\"${2:prompt}\")$0", "Read typed input"},
      {"system", "system(\"${1:command}\");$0", "Execute system command"},

      // Type operations
      {"cast", "cast<${1:Type}>(${2:value})$0", "Type cast"},
      {"sizeof", "sizeof<${1:Type}>$0", "Size of type"},

      // Memory operations
      {"alloc", "cast<${1:*Type}>(alloc(${2:size} * sizeof<${3:Type}>))$0",
       "Allocate memory"},
      {"alloc defer",
       "let ${1:ptr}: ${2:*Type} = cast<${2:*Type}>(alloc(${3:size} * "
       "sizeof<${4:Type}>));\ndefer free(${1:ptr});$0",
       "Allocate with defer cleanup"},

      // Struct patterns
      {"struct method", "${1:name} -> fn (${2:params}) ${3:Type} {\n\t$0\n}",
       "Struct method"},
      {"struct pub", "pub:\n\t${1:field}: ${2:Type},$0",
       "Public struct fields"},
      {"struct priv", "priv:\n\t${1:field}: ${2:Type},$0",
       "Private struct fields"},

      // Array patterns
      {"array", "[${1:Type}; ${2:size}]$0", "Array type"},
      {"array init", "let ${1:arr}: [${2:Type}; ${3:size}] = [${4:values}];$0",
       "Array initialization"},

      // Pointer patterns
      {"pointer", "*${1:Type}$0", "Pointer type"},
      {"address of", "&${1:variable}$0", "Address-of operator"},
      {"dereference", "*${1:pointer}$0", "Dereference pointer"},

      // Function attributes
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

  // Find the most specific scope at the cursor position
  Scope *local_scope = doc->scope;
  // if (doc->ast) {
  //   local_scope = find_scope_at_position(doc->ast, position, doc->scope);
  // }

  // Add symbols from local scope and all parent scopes
  if (local_scope) {
    Scope *current_scope = local_scope;
    int scope_depth = 0;

    while (current_scope) {
      if (current_scope->symbols.data && current_scope->symbols.count > 0) {
        for (size_t i = 0; i < current_scope->symbols.count; i++) {
          Symbol *sym = (Symbol *)((char *)current_scope->symbols.data +
                                   i * sizeof(Symbol));

          if (!sym || !sym->name || !sym->type) {
            continue;
          }

          LSPCompletionItem *item =
              (LSPCompletionItem *)growable_array_push(&completions);
          if (item) {
            item->label = arena_strdup(arena, sym->name);

            if (sym->type->type == AST_TYPE_FUNCTION) {
              item->kind = LSP_COMPLETION_FUNCTION;

              // Create function call snippet with placeholders for params
              char snippet[512];
              snprintf(snippet, sizeof(snippet), "%s()$0", sym->name);
              item->insert_text = arena_strdup(arena, snippet);
              item->format = LSP_INSERT_FORMAT_SNIPPET;

              // Show function signature in detail (type_to_string handles this
              // now)
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

            // Add scope depth info for sorting (prefer local variables)
            if (scope_depth == 0) {
              item->sort_text = arena_strdup(arena, "0");
            } else {
              char sort[8];
              snprintf(sort, sizeof(sort), "%d", scope_depth);
              item->sort_text = arena_strdup(arena, sort);
            }

            item->documentation = NULL;
            item->filter_text = NULL;
          }
        }
      }

      current_scope = current_scope->parent;
      scope_depth++;
    }
  }

  // Add imported module symbols
  if (doc->imports && doc->import_count > 0) {
    for (size_t i = 0; i < doc->import_count; i++) {
      ImportedModule *import = &doc->imports[i];

      if (!import->scope || !import->scope->symbols.data) {
        continue;
      }

      const char *prefix = import->alias ? import->alias : "module";

      for (size_t j = 0; j < import->scope->symbols.count; j++) {
        Symbol *sym = (Symbol *)((char *)import->scope->symbols.data +
                                 j * sizeof(Symbol));

        if (!sym || !sym->name || !sym->type || !sym->is_public) {
          continue;
        }

        // FILTER OUT INTERNAL SYMBOLS (those starting with __)
        if (strncmp(sym->name, "__", 2) == 0) {
          continue;
        }

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

          // Show function signature or type
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
