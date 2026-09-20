# Module: ast_type

*Source: `src/ast/type.lx`*

Constructors for the type AST nodes: basic, pointer, array, function, and
qualified (resolved) types. Each returns a freshly allocated node boxed as
an `*AST::AstNode`.

## Table of Contents

- [Functions](#functions)


## Functions

### `make_basic_type`

Builds a `TYPE_BASIC` node for a named type (`i64`, user structs, ...).
The generic `type_args` list is left empty.

```luma
pub #returns_ownership
make_basic_type -> fn(
    name: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_pointer_type`

Builds a `TYPE_POINTER` node for a pointer to `pointee_type`.

```luma
pub #returns_ownership
make_pointer_type -> fn(
    pointee_type: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_array_type`

Builds a `TYPE_ARRAY` node for an array of `element_type` sized by `size`.

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

Builds a `TYPE_FUNCTION` node from the parameter type list `param_types`
(`param_count` entries) and `return_type`.

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

Builds a `TYPE_RESOLUTION` node for a qualified name assembled from `parts`
(e.g. `A::B`). The generic `type_args` list is left empty.

```luma
pub #returns_ownership
make_resolution_type -> fn(
    parts: **byte,
    part_count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

