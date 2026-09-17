# Module: ast_type

*Source: `src/ast/type.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `make_basic_type`

```luma
pub #returns_ownership
make_basic_type -> fn(
    name: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_pointer_type`

```luma
pub #returns_ownership
make_pointer_type -> fn(
    pointee_type: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_array_type`

```luma
pub #returns_ownership
make_array_type -> fn(
    element_type: *AST::AstNode,
    size: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_func_type`

```luma
pub #returns_ownership
make_func_type -> fn(
    param_types: **AST::AstNode,
    param_count: i64,
    return_type: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_resolution_type`

```luma
pub #returns_ownership
make_resolution_type -> fn(
    parts: **byte,
    part_count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

