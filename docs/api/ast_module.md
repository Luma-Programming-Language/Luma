# Module: ast_module

*Source: `src/ast/module.lx`*

Constructors for the preprocessor AST nodes: `@module`, `@use`, `@os`, and
`@link`. Each returns a freshly allocated node boxed as an `*AST::AstNode`.

## Table of Contents

- [Functions](#functions)


## Functions

### `make_module`

Builds a `PREPROCESSOR_MODULE` node for a `@module` section: name, doc
comment, file path, body statement list, and the module's full source text
and token stream (retained for diagnostics).

```luma
pub #returns_ownership
make_module -> fn(
    name: *byte,
    doc_comment: *byte,
    file_path: *byte,
    body: **AST::AstNode,
    body_count: i64,
    line: i64,
    col: i64,
    source: *byte,
    tokens: *TOK::Token,
    token_count: i64
) *AST::AstNode
```

### `make_use`

Builds a `PREPROCESSOR_USE` node for an `@use "name" as alias` import.

```luma
pub #returns_ownership
make_use -> fn(
    module_name: *byte,
    alias: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_os`

Builds a `PREPROCESSOR_OS` node for an `@os` conditional section: one body
per platform plus an optional default body.

```luma
pub #returns_ownership
make_os -> fn(
    platforms: **byte,
    bodies: **AST::AstNode,
    has_default: i64,
    default_body: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_link`

Builds a `PREPROCESSOR_LINK` node naming a library to link against.

```luma
pub #returns_ownership
make_link -> fn(
    lib_name: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

