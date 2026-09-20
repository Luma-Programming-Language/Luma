# Module: lsp_document

*Source: `src/lsp/lsp_document.lx`*

Document tracking + single-file analysis for the Luma language server.

Each open document is analyzed as its own scratch file, run through the
normal parse/typecheck pipeline exactly as the CLI compiler would — but
its `@use` imports are resolved transitively against a workspace/std
registry first (see the Module Resolution section), so this isn't
limited to single-file programs in practice.

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)
- [Variables](#variables)
- [OS-Specific](#os-specific)

---

## Structures

### `LSPDocument`

An open document tracked by the server.


| Field | Type | Description |
|-------|------|-------------|
| `uri` | *byte |  |
| `text` | *byte |  |
| `version` | i64 |  |
| `ast` | *AST::AstNode |  |

### `ModuleRegistryEntry`

One resolved module in the registry.


| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `path` | *byte |  |

### `StrBuf`

A growable string buffer used to assemble signature text.


| Field | Type | Description |
|-------|------|-------------|
| `data` | *byte |  |
| `len` | i64 |  |
| `cap` | i64 |  |

### `LocalThing`

Result of resolving a name inside a function body.


| Field | Type | Description |
|-------|------|-------------|
| `found` | bool |  |
| `is_param` | bool |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `name_len` | i64 |  |
| `type_node` | *AST::AstNode |  |

### `TypeParamHit`

A generic declaration that owns a type parameter.


| Field | Type | Description |
|-------|------|-------------|
| `found` | bool |  |
| `node` | *AST::AstNode |  |
| `idx` | i64 |  |

### `ParamPos`

Source position of a located type parameter.


| Field | Type | Description |
|-------|------|-------------|
| `line` | i64 |  |
| `col` | i64 |  |
| `len` | i64 |  |

### `DefHit`

A definition-site position found for go-to-definition.


| Field | Type | Description |
|-------|------|-------------|
| `found` | bool |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `name_len` | i64 |  |

### `SnippetDef`

A snippet completion template.


| Field | Type | Description |
|-------|------|-------------|
| `label` | *byte |  |
| `snippet` | *byte |  |
| `detail` | *byte |  |
| `filter` | *byte |  |

### `SemToken`

One semantic token: position, length, legend type index, and modifier bits.

| Field | Type | Description |
|-------|------|-------------|
| `line` | i64 |  |
| `col` | i64 |  |
| `length` | i64 |  |
| `type_idx` | i64 |  |
| `mods` | i64 |  |

### `MemberPos`

Recovered source position of a member name following `::` or `.`.


| Field | Type | Description |
|-------|------|-------------|
| `found` | bool |  |
| `line` | i64 |  |
| `col` | i64 |  |

### `TokenClass`

Classification of a raw token for the semantic-token fallback pass.


| Field | Type | Description |
|-------|------|-------------|
| `type_idx` | i64 |  |
| `mods` | i64 |  |


## Functions

### `lsp_document_init`

Initializes the global document table; call once before any other
lsp_document_* function.

```luma
pub lsp_document_init -> fn(
) void
```

### `find_index`

Returns the index of the document whose uri matches, or -1 if not open.

```luma
      find_index -> fn(
    uri: *byte
) i64
```

### `dup_str`

Returns a fresh heap copy of a NUL-terminated string.

```luma
      #returns_ownership
dup_str -> fn(
    s: *byte
) *byte
```

### `lsp_document_find`

Returns the open LSPDocument for uri, or null if not open.

```luma
pub lsp_document_find -> fn(
    uri: *byte
) *LSPDocument
```

### `lsp_document_open`

Registers uri with the given text and version, creating or replacing the document.

```luma
pub lsp_document_open -> fn(
    uri: *byte,
    text: *byte,
    version: i64
) void
```

### `lsp_document_update`

Replaces the text and version of an open document, opening it first if absent.

```luma
pub lsp_document_update -> fn(
    uri: *byte,
    text: *byte,
    version: i64
) void
```

### `lsp_document_close`

Unregisters uri and frees its URI, text, and cached AST.

```luma
pub lsp_document_close -> fn(
    uri: *byte
) void
```

### `lsp_set_workspace_root`

Stores the workspace root, stripping a leading `file://` prefix.

```luma
pub lsp_set_workspace_root -> fn(
    uri: *byte
) void
```

### `find_substr`

Returns the index of the first occurrence of needle in haystack, or -1.

```luma
      find_substr -> fn(
    haystack: *byte,
    needle: *byte
) i64
```

### `extract_module_name`

Extracts the `@module "name"` from file contents, or null if none.

```luma
      #returns_ownership
extract_module_name -> fn(
    content: *byte
) *byte
```

### `registry_lookup`

Returns the registered path for name, or null if unknown.

```luma
      registry_lookup -> fn(
    name: *byte
) *byte
```

### `register_file`

Parses a file's `@module` name and registers it in the registry.

```luma
      register_file -> fn(
    path: *byte
) void
```

### `append_dir_listing`

Appends every `*.lx` path under dir to the registry listing file.

```luma
      append_dir_listing -> fn(
    dir: *byte
) void
```

### `build_registry`

Scans std and the workspace root and fills g_registry; only runs once.

```luma
      build_registry -> fn(
) void
```

### `name_seen`

True if name already appears in the seen list.

```luma
      name_seen -> fn(
    seen: *VEC::Vector,
    name: *byte
) bool
```

### `collect_modules`

Returns the transitive closure of modules imported by main_module.

```luma
      collect_modules -> fn(
    main_module: *AST::AstNode
) VEC::Vector
```

### `write_scratch_file`

Creates path if needed, then writes content to it.

```luma
      write_scratch_file -> fn(
    path: *byte,
    content: *byte
) void
```

### `lsp_analyze`

Parses and typechecks a document's text as a scratch file, caching the program.

```luma
pub lsp_analyze -> fn(
    doc: *LSPDocument
) void
```

### `build_message_with_help`

Appends the compiler's printed help text to a diagnostic message.

```luma
      #returns_ownership
build_message_with_help -> fn(
    message: *byte,
    help: *byte
) *byte
```

### `diagnostics_array`

Builds the JSON diagnostics array for the current analysis errors.

```luma
      #returns_ownership
diagnostics_array -> fn(
) *J::JsonValue
```

### `lsp_publish_diagnostics`

Publishes a textDocument/publishDiagnostics notification for uri.

```luma
pub lsp_publish_diagnostics -> fn(
    uri: *byte
) void
```

### `lsp_analyze_and_publish`

Re-analyzes uri and publishes fresh diagnostics for it.

```luma
pub lsp_analyze_and_publish -> fn(
    uri: *byte
) void
```

### `doc_module`

Unwraps the cached program into a ModuleNode, or null.

```luma
      doc_module -> fn(
    doc: *LSPDocument
) *AST::ModuleNode
```

### `make_position`

Builds a JSON `{line, character}` position object.

```luma
      #returns_ownership
make_position -> fn(
    line: i64,
    character: i64
) *J::JsonValue
```

### `make_symbol`

Builds a document-symbol node with a name, kind, range, and children.

```luma
      #returns_ownership
make_symbol -> fn(
    name: *byte,
    kind: i64,
    line: i64,
    col: i64,
    children: *J::JsonValue
) *J::JsonValue
```

### `make_struct_symbol_children`

Builds child symbols for a struct's fields and methods.

```luma
      #returns_ownership
make_struct_symbol_children -> fn(
    sd: *AST::StructDeclNode
) *J::JsonValue
```

### `make_enum_symbol_children`

Builds child symbols for an enum's variants.

```luma
      #returns_ownership
make_enum_symbol_children -> fn(
    ed: *AST::EnumDeclNode
) *J::JsonValue
```

### `lsp_document_symbols`

Returns the document-symbol array for uri, or null.

```luma
pub #returns_ownership
lsp_document_symbols -> fn(
    uri: *byte
) *J::JsonValue
```

### `sb_init`

Returns an empty StrBuf with a preallocated buffer.

```luma
      sb_init -> fn(
) StrBuf
```

### `sb_ensure`

Grows the buffer so `extra` more bytes fit.

```luma
      sb_ensure -> fn(
    b: *StrBuf,
    extra: i64
) void
```

### `sb_str`

Appends the NUL-terminated string s.

```luma
      sb_str -> fn(
    b: *StrBuf,
    s: *byte
) void
```

### `sb_finish`

NUL-terminates and returns the buffer as an owned string.

```luma
      #returns_ownership
sb_finish -> fn(
    b: *StrBuf
) *byte
```

### `is_ident_char`

True if c is a valid identifier character.

```luma
      is_ident_char -> fn(
    c: byte
) bool
```

### `word_at_position`

Returns the identifier word touching (line, character) in text, or null.

```luma
      #returns_ownership
word_at_position -> fn(
    text: *byte,
    line: i64,
    character: i64
) *byte
```

### `format_func_signature`

Formats a function header as `fn name<T>(args) Ret` plus its doc comment.

```luma
      #returns_ownership
format_func_signature -> fn(
    fd: *AST::FuncDeclNode
) *byte
```

### `format_struct_member`

Formats one struct member (typed field or method) for a signature.

```luma
      #returns_ownership
format_struct_member -> fn(
    member: *AST::AstNode
) *byte
```

### `format_struct_signature`

Formats a struct header as `struct Name<T> { ... }`.

```luma
      #returns_ownership
format_struct_signature -> fn(
    sd: *AST::StructDeclNode
) *byte
```

### `format_enum_signature`

Formats an enum header as `enum Name<T> { ... }`.

```luma
      #returns_ownership
format_enum_signature -> fn(
    ed: *AST::EnumDeclNode
) *byte
```

### `format_var_signature`

Formats a variable header as `let name: Type`.

```luma
      #returns_ownership
format_var_signature -> fn(
    vd: *AST::VarDeclNode
) *byte
```

### `append_doc_comment`

Appends doc_comment (if any) to sig, returning a fresh owned string.

```luma
      #returns_ownership
append_doc_comment -> fn(
    sig: *byte,
    doc_comment: *byte
) *byte
```

### `find_enclosing_function`

Returns the function whose body spans one_indexed_line, or null.

```luma
      find_enclosing_function -> fn(
    mod: *AST::ModuleNode,
    one_indexed_line: i64
) *AST::FuncDeclNode
```

### `search_stmt_for_local_decl`

Searches a statement recursively for a local or const named name.

```luma
      search_stmt_for_local_decl -> fn(
    stmt: *AST::AstNode,
    name: *byte
) *AST::AstNode
```

### `find_local_thing`

Resolves name to a LocalThing inside the function enclosing the line.

```luma
      find_local_thing -> fn(
    mod: *AST::ModuleNode,
    one_indexed_line: i64,
    name: *byte
) LocalThing
```

### `format_local_hover`

Formats hover text for a resolved local or parameter.

```luma
      #returns_ownership
format_local_hover -> fn(
    name: *byte,
    thing: LocalThing
) *byte
```

### `find_in_module`

Returns a formatted signature for the top-level declaration named name, or null.

```luma
      #returns_ownership
find_in_module -> fn(
    mod: *AST::ModuleNode,
    name: *byte,
    public_only: bool
) *byte
```

### `max_line_in_stmt`

Returns the deepest line spanned by any node in stmt.

```luma
      max_line_in_stmt -> fn(
    stmt: *AST::AstNode
) i64
```

### `func_end_line`

Returns the last line of a function body, or its header line if bodyless.

```luma
      func_end_line -> fn(
    fd: *AST::FuncDeclNode
) i64
```

### `field_end_line`

Returns the last line spanned by a struct field, including a method body.

```luma
      field_end_line -> fn(
    f: *AST::AstNode
) i64
```

### `struct_end_line`

Returns the deepest line spanned by any member of a struct.

```luma
      struct_end_line -> fn(
    sd: *AST::StructDeclNode
) i64
```

### `struct_method_hit`

Searches a struct's methods (and self type params) for one that owns word.

```luma
      struct_method_hit -> fn(
    sd: *AST::StructDeclNode,
    one_indexed_line: i64,
    word: *byte
) TypeParamHit
```

### `resolve_type_param`

Resolves word to the generic declaration that owns it at that line.

```luma
      resolve_type_param -> fn(
    mod: *AST::ModuleNode,
    one_indexed_line: i64,
    word: *byte
) TypeParamHit
```

### `format_type_param_hover`

Formats hover text for a type parameter under a generic header.

```luma
      #returns_ownership
format_type_param_hover -> fn(
    name: *byte,
    sig: *byte
) *byte
```

### `line_start_offset`

Returns the byte offset of the start of zero-indexed line_index.

```luma
      line_start_offset -> fn(
    text: *byte,
    line_index: i64
) i64
```

### `locate_type_param`

Finds where the type parameter appears inside its generic header.

```luma
      locate_type_param -> fn(
    text: *byte,
    hit: TypeParamHit
) ParamPos
```

### `resolve_symbol_signature`

Resolves name at a 0-based line to a signature string, or null.

```luma
      #returns_ownership
resolve_symbol_signature -> fn(
    doc: *LSPDocument,
    name: *byte,
    zero_indexed_line: i64
) *byte
```

### `lsp_hover`

Returns hover text for the word at (line, character) in uri, or null.

```luma
pub #returns_ownership
lsp_hover -> fn(
    uri: *byte,
    line: i64,
    character: i64
) *byte
```

### `find_definition_in_module`

Finds the top-level declaration named name in a module, optionally public-only.

```luma
      find_definition_in_module -> fn(
    mod: *AST::ModuleNode,
    name: *byte,
    public_only: bool
) DefHit
```

### `path_to_uri`

Prefixes a filesystem path with `file://`.

```luma
      #returns_ownership
path_to_uri -> fn(
    path: *byte
) *byte
```

### `make_location_json`

Builds a JSON Location object for a definition sitting at (line, col).

```luma
      #returns_ownership
make_location_json -> fn(
    uri: *byte,
    line: i64,
    col: i64,
    name_len: i64
) *J::JsonValue
```

### `lsp_definition`

Returns the definition Location for the word at (line, character) in uri, or null.

```luma
pub #returns_ownership
lsp_definition -> fn(
    uri: *byte,
    line: i64,
    character: i64
) *J::JsonValue
```

### `make_completion_item`

Builds a completion item with a label, kind, and detail.

```luma
      #returns_ownership
make_completion_item -> fn(
    label: *byte,
    kind: i64,
    detail: *byte
) *J::JsonValue
```

### `make_snippet_item`

Builds a snippet completion item from a SnippetDef.

```luma
      #returns_ownership
make_snippet_item -> fn(
    def: SnippetDef
) *J::JsonValue
```

### `make_prefixed_completion_item`

Builds an `alias::name` completion item for an alias-prefixed access.

```luma
      #returns_ownership
make_prefixed_completion_item -> fn(
    alias: *byte,
    name: *byte,
    kind: i64
) *J::JsonValue
```

### `collect_prefixed_import_completions`

Adds `alias::name` completions for every dependency module.

```luma
      collect_prefixed_import_completions -> fn(
    arr: *J::JsonValue,
    mod: *AST::ModuleNode,
    prog: *AST::ProgramNode
) void
```

### `collect_module_completions`

Adds completions for a module's top-level declarations.

```luma
      collect_module_completions -> fn(
    arr: *J::JsonValue,
    mod: *AST::ModuleNode,
    public_only: bool
) void
```

### `collect_locals_completions`

Adds completions for every local declaration inside stmt, recursively.

```luma
      collect_locals_completions -> fn(
    arr: *J::JsonValue,
    stmt: *AST::AstNode
) void
```

### `lsp_completions`

Returns the completion list for the position in uri, or null.

```luma
pub #returns_ownership
lsp_completions -> fn(
    uri: *byte,
    line: i64,
    character: i64
) *J::JsonValue
```

### `push_token`

Appends a token to the vector, skipping zero-length tokens.

```luma
      push_token -> fn(
    tokens: *VEC::Vector,
    line: i64,
    col: i64,
    length: i64,
    type_idx: i64,
    mods: i64
) void
```

### `sort_tokens`

Sorts tokens by (line, column) so they are emitted in position order.

```luma
      sort_tokens -> fn(
    tokens: *VEC::Vector
) void
```

### `lookup_public_kind`

Returns the legend index for a matching public declaration, or -1.

```luma
      lookup_public_kind -> fn(
    mod: *AST::ModuleNode,
    name: *byte
) i64
```

### `line_col_to_offset`

Converts a 1-based line and 0-based column to a byte offset into text.

```luma
      line_col_to_offset -> fn(
    text: *byte,
    one_indexed_line: i64,
    col: i64
) i64
```

### `find_member_position`

Locates a member name in the live text just past a `::` or `.` access.

```luma
      find_member_position -> fn(
    text: *byte,
    dot_line: i64,
    dot_col: i64,
    is_compiletime: i64,
    member: *byte
) MemberPos
```

### `resolve_use_alias`

Resolves an `@use` alias to its ModuleNode, or null.

```luma
      resolve_use_alias -> fn(
    mod: *AST::ModuleNode,
    prog: *AST::ProgramNode,
    alias: *byte
) *AST::ModuleNode
```

### `resolve_type_name`

Unwraps pointer/array wrappers to the named type node, or null.

```luma
      resolve_type_name -> fn(
    t: *AST::AstNode
) *byte
```

### `find_struct_decl`

Finds a struct declaration by name in a module or its public dependencies.

```luma
      find_struct_decl -> fn(
    mod: *AST::ModuleNode,
    prog: *AST::ProgramNode,
    name: *byte
) *AST::StructDeclNode
```

### `find_field_in_struct`

Looks up a name among a struct's fields and methods.

```luma
      find_field_in_struct -> fn(
    sd: *AST::StructDeclNode,
    name: *byte
) *AST::FieldDeclNode
```

### `emit_member_token`

Emits a semantic token for the member name of a `::` or `.` access when classifiable.

```luma
      emit_member_token -> fn(
    tokens: *VEC::Vector,
    mod: *AST::ModuleNode,
    prog: *AST::ProgramNode,
    member_node: *AST::AstNode,
    n: *AST::MemberNode
) void
```

### `emit_ident_token`

Emits semantic tokens for a single identifier expression.

```luma
      emit_ident_token -> fn(
    tokens: *VEC::Vector,
    mod: *AST::ModuleNode,
    fd: *AST::FuncDeclNode,
    id: *AST::IdentifierNode,
    prog: *AST::ProgramNode
) void
```

### `walk_type_for_tokens`

Emits tokens for everything in a type annotation.

```luma
      walk_type_for_tokens -> fn(
    tokens: *VEC::Vector,
    mod: *AST::ModuleNode,
    prog: *AST::ProgramNode,
    t: *AST::AstNode
) void
```

### `walk_expr_for_tokens`

Walks an expression, emitting tokens for identifiers, members, and types.

```luma
      walk_expr_for_tokens -> fn(
    tokens: *VEC::Vector,
    mod: *AST::ModuleNode,
    fd: *AST::FuncDeclNode,
    prog: *AST::ProgramNode,
    expr: *AST::AstNode
) void
```

### `walk_field_for_tokens`

Walks a struct field's type, or a method's params and body.

```luma
      walk_field_for_tokens -> fn(
    tokens: *VEC::Vector,
    mod: *AST::ModuleNode,
    prog: *AST::ProgramNode,
    fld: *AST::AstNode
) void
```

### `resolve_os_body_for_tokens`

Selects the `@os` arm matching the current OS, or the default arm; null if none.

```luma
      resolve_os_body_for_tokens -> fn(
    n: *AST::OsNode
) *AST::AstNode
```

### `walk_stmt_for_tokens`

Walks a statement, emitting tokens for its expressions and nested statements.

```luma
      walk_stmt_for_tokens -> fn(
    tokens: *VEC::Vector,
    mod: *AST::ModuleNode,
    fd: *AST::FuncDeclNode,
    prog: *AST::ProgramNode,
    stmt: *AST::AstNode
) void
```

### `walk_module_decl_for_tokens`

Emits tokens for one top-level declaration in a module.

```luma
      walk_module_decl_for_tokens -> fn(
    tokens: *VEC::Vector,
    mod: *AST::ModuleNode,
    prog: *AST::ProgramNode,
    bn: *AST::AstNode
) void
```

### `token_text_equals`

True if a token's text equals s.

```luma
      token_text_equals -> fn(
    tok: TOK::Token,
    s: *byte
) bool
```

### `is_use_alias`

True if a token's text matches one of the module's `@use` aliases.

```luma
      is_use_alias -> fn(
    mod: *AST::ModuleNode,
    tok: TOK::Token
) bool
```

### `classify_raw_identifier`

Classifies an identifier that the AST pass did not already resolve.

```luma
      classify_raw_identifier -> fn(
    mod: *AST::ModuleNode,
    toks: *TOK::Token,
    count: i64,
    idx: i64
) TokenClass
```

### `classify_raw_token`

Classifies a raw keyword or punctuation token, or -1 to skip it.

```luma
      classify_raw_token -> fn(
    mod: *AST::ModuleNode,
    toks: *TOK::Token,
    count: i64,
    idx: i64
) TokenClass
```

### `lsp_semantic_tokens`

Returns delta-encoded semantic tokens for uri, merging both passes.

```luma
pub #returns_ownership
lsp_semantic_tokens -> fn(
    uri: *byte
) *J::JsonValue
```


## Variables

- **`g_documents`** : VEC::Vector *(let)* — Vector of every open LSPDocument.
- **`g_registry`** : VEC::Vector *(let)* — Registry of known modules, keyed by `@module` name.
- **`g_registry_built`** : bool *(let)* — True once the registry scan has run (the scan runs lazily, once).
- **`g_workspace_root`** : *byte *(let)* — Owned workspace root URI, set from the client's initialize request.
- **`REGISTRY_LISTING_PATH`** : *byte *(const)* — Scratch file that collects the recursive registry scan output.
- **`SCRATCH_PATH`** : *byte *(const)* — Scratch file each document is written to for analysis.
- **`SK_FUNCTION`** : i64 *(const)* — SymbolKind code for functions.
- **`SK_STRUCT`** : i64 *(const)* — SymbolKind code for structs.
- **`SK_ENUM`** : i64 *(const)* — SymbolKind code for enums.
- **`SK_VARIABLE`** : i64 *(const)* — SymbolKind code for variables.
- **`SK_CONSTANT`** : i64 *(const)* — SymbolKind code for constants.
- **`SK_METHOD`** : i64 *(const)* — SymbolKind code for methods.
- **`SK_FIELD`** : i64 *(const)* — SymbolKind code for fields.
- **`SK_ENUM_MEMBER`** : i64 *(const)* — SymbolKind code for enum members.
- **`CK_FUNCTION`** : i64 *(const)* — CompletionItemKind code for functions.
- **`CK_VARIABLE`** : i64 *(const)* — CompletionItemKind code for variables.
- **`CK_ENUM`** : i64 *(const)* — CompletionItemKind code for enums.
- **`CK_KEYWORD`** : i64 *(const)* — CompletionItemKind code for keywords.
- **`CK_CONSTANT`** : i64 *(const)* — CompletionItemKind code for constants.
- **`CK_STRUCT`** : i64 *(const)* — CompletionItemKind code for structs.
- **`CK_ENUM_MEMBER`** : i64 *(const)* — CompletionItemKind code for enum members.
- **`KEYWORD_COUNT`** : i64 *(const)* — Number of keyword completions.
- **`KEYWORDS`** : [*byte; 43] *(const)* — The language keywords offered as completions.
- **`CK_SNIPPET`** : i64 *(const)* — CompletionItemKind code for snippet completions.
- **`SNIPPET_COUNT`** : i64 *(const)* — Number of snippet templates.
- **`SNIPPETS`** : [SnippetDef; 47] *(const)* — The snippet templates offered as completions.
- **`TT_NAMESPACE`** : i64 *(const)* — Legend index for namespace tokens.
- **`TT_TYPE`** : i64 *(const)* — Legend index for type tokens.
- **`TT_TYPE_PARAMETER`** : i64 *(const)* — Legend index for type-parameter tokens.
- **`TT_FUNCTION`** : i64 *(const)* — Legend index for function tokens.
- **`TT_METHOD`** : i64 *(const)* — Legend index for method tokens.
- **`TT_PROPERTY`** : i64 *(const)* — Legend index for property tokens.
- **`TT_VARIABLE`** : i64 *(const)* — Legend index for variable tokens.
- **`TT_PARAMETER`** : i64 *(const)* — Legend index for parameter tokens.
- **`TT_KEYWORD`** : i64 *(const)* — Legend index for keyword tokens.
- **`TT_MODIFIER`** : i64 *(const)* — Legend index for modifier tokens.
- **`TT_COMMENT`** : i64 *(const)* — Legend index for comment tokens.
- **`TT_STRING`** : i64 *(const)* — Legend index for string tokens.
- **`TT_NUMBER`** : i64 *(const)* — Legend index for number tokens.
- **`TT_OPERATOR`** : i64 *(const)* — Legend index for operator tokens.
- **`TT_STRUCT`** : i64 *(const)* — Legend index for struct tokens.
- **`TT_ENUM`** : i64 *(const)* — Legend index for enum tokens.
- **`TT_ENUM_MEMBER`** : i64 *(const)* — Legend index for enum-member tokens.
- **`MOD_DECLARATION`** : i64 *(const)* — Modifier bit for a declaration.
- **`MOD_DEFINITION`** : i64 *(const)* — Modifier bit for a definition.
- **`MOD_READONLY`** : i64 *(const)* — Modifier bit for a read-only value.
- **`MOD_STATIC`** : i64 *(const)* — Modifier bit for a static member.
- **`MOD_DEFAULT_LIB`** : i64 *(const)* — Modifier bit for a std-library symbol.
- **`g_sem_text`** : *byte *(let)* — Live document text kept while emitting tokens so member names can be recovered.
- **`g_sem_in_method`** : bool *(let)* — True while emitting tokens inside a struct method body, so self reads as a variable.
- **`g_sem_in_template`** : bool *(let)* — True while emitting tokens inside an uninstantiated generic body.

## OS-Specific

### `"linux"`

- **`STD_DIR`** : *byte *(const)*

### `"macos"`

- **`STD_DIR`** : *byte *(const)*

### `"windows64"`

- **`STD_DIR`** : *byte *(const)*

### Default `_`

- **`STD_DIR`** : *byte *(const)*

### `"linux"`

- **`CURRENT_OS`** : *byte *(const)*

### `"macos"`

- **`CURRENT_OS`** : *byte *(const)*

### `"windows64"`

- **`CURRENT_OS`** : *byte *(const)*

### Default `_`

- **`CURRENT_OS`** : *byte *(const)*

