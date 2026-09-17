# Module: ast_module

*Source: `src/ast/module.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `make_module`

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

```luma
pub #returns_ownership
make_link -> fn(
    lib_name: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

