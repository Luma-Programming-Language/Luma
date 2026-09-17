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

| Field | Type | Description |
|-------|------|-------------|
| `uri` | *byte |  |
| `text` | *byte |  |
| `version` | i64 |  |
| `ast` | *AST::AstNode |  |


## Functions

### `lsp_document_init`

```luma
pub lsp_document_init -> fn(
) void
```

### `lsp_document_find`

```luma
pub lsp_document_find -> fn(
    uri: *byte
) *LSPDocument
```

### `lsp_document_open`

```luma
pub lsp_document_open -> fn(
    uri: *byte,
    text: *byte,
    version: i64
) void
```

### `lsp_document_update`

```luma
pub lsp_document_update -> fn(
    uri: *byte,
    text: *byte,
    version: i64
) void
```

### `lsp_document_close`

```luma
pub lsp_document_close -> fn(
    uri: *byte
) void
```

### `lsp_set_workspace_root`

```luma
pub lsp_set_workspace_root -> fn(
    uri: *byte
) void
```

### `lsp_analyze`

```luma
pub lsp_analyze -> fn(
    doc: *LSPDocument
) void
```

### `lsp_publish_diagnostics`

```luma
pub lsp_publish_diagnostics -> fn(
    uri: *byte
) void
```

### `lsp_analyze_and_publish`

```luma
pub lsp_analyze_and_publish -> fn(
    uri: *byte
) void
```

### `lsp_document_symbols`

```luma
pub #returns_ownership
lsp_document_symbols -> fn(
    uri: *byte
) *J::JsonValue
```

### `lsp_hover`

```luma
pub #returns_ownership
lsp_hover -> fn(
    uri: *byte,
    line: i64,
    character: i64
) *byte
```

### `lsp_definition`

```luma
pub #returns_ownership
lsp_definition -> fn(
    uri: *byte,
    line: i64,
    character: i64
) *J::JsonValue
```

### `lsp_completions`

```luma
pub #returns_ownership
lsp_completions -> fn(
    uri: *byte,
    line: i64,
    character: i64
) *J::JsonValue
```

### `lsp_semantic_tokens`

```luma
pub #returns_ownership
lsp_semantic_tokens -> fn(
    uri: *byte
) *J::JsonValue
```


## Variables

- **`g_documents`** : VEC::Vector *(let)*

## OS-Specific

### `"linux"`


### `"macos"`


### `"windows64"`


### Default `_`


### `"linux"`


### `"macos"`


### `"windows64"`


### Default `_`


