# Module: tc_stmt

*Source: `src/typechecker/stmt.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `typecheck_statement`

```luma
pub typecheck_statement -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `register_declaration`

```luma
pub register_declaration -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `resolve_os_body`

```luma
pub resolve_os_body -> fn(
    node: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `typecheck_statement`

```luma
pub typecheck_statement -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `resolve_os_body`

```luma
pub resolve_os_body -> fn(
    node: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `register_declaration`

```luma
pub register_declaration -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

