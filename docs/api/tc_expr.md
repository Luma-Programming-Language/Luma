# Module: tc_expr

*Source: `src/typechecker/expr.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `typecheck_expression`

```luma
pub #returns_ownership
typecheck_expression -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `track_pointer_source`

```luma
pub track_pointer_source -> fn(
    name: *byte,
    value_expr: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `qualify`

```luma
pub #returns_ownership
qualify -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `track_pointer_source`

```luma
pub track_pointer_source -> fn(
    name: *byte,
    value_expr: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `typecheck_expression`

```luma
pub #returns_ownership
typecheck_expression -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

