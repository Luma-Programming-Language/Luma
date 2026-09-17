# Module: ast_expr

*Source: `src/ast/expr.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `make_literal_int`

```luma
pub #returns_ownership
make_literal_int -> fn(
    int_val: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_float`

```luma
pub #returns_ownership
make_literal_float -> fn(
    float_val: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_string`

```luma
pub #returns_ownership
make_literal_string -> fn(
    string_val: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_char`

```luma
pub #returns_ownership
make_literal_char -> fn(
    char_val: byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_bool`

```luma
pub #returns_ownership
make_literal_bool -> fn(
    bool_val: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_null`

```luma
pub #returns_ownership
make_literal_null -> fn(
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_identifier`

```luma
pub #returns_ownership
make_identifier -> fn(
    name: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_binary`

```luma
pub #returns_ownership
make_binary -> fn(
    op: i64,
    left: *AST::AstNode,
    right: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_unary`

```luma
pub #returns_ownership
make_unary -> fn(
    op: i64,
    operand: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_call`

```luma
pub #returns_ownership
make_call -> fn(
    callee: *AST::AstNode,
    args: **AST::AstNode,
    arg_count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_assign`

```luma
pub #returns_ownership
make_assign -> fn(
    target: *AST::AstNode,
    value: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_ternary`

```luma
pub #returns_ownership
make_ternary -> fn(
    condition: *AST::AstNode,
    then_expr: *AST::AstNode,
    else_expr: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_member`

```luma
pub #returns_ownership
make_member -> fn(
    object: *AST::AstNode,
    member: *byte,
    is_compiletime: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_index`

```luma
pub #returns_ownership
make_index -> fn(
    object: *AST::AstNode,
    index: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_array`

```luma
pub #returns_ownership
make_array -> fn(
    elements: **AST::AstNode,
    element_count: i64,
    target_size: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_cast`

```luma
pub #returns_ownership
make_cast -> fn(
    type_node: *AST::AstNode,
    castee: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_alloc`

```luma
pub #returns_ownership
make_alloc -> fn(
    size: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_free`

```luma
pub #returns_ownership
make_free -> fn(
    ptr: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_memcpy`

```luma
pub #returns_ownership
make_memcpy -> fn(
    to: *AST::AstNode,
    from: *AST::AstNode,
    size: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_sizeof`

```luma
pub #returns_ownership
make_sizeof -> fn(
    object: *AST::AstNode,
    is_type: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_syscall`

```luma
pub #returns_ownership
make_syscall -> fn(
    args: **AST::AstNode,
    count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_struct_expr`

```luma
pub #returns_ownership
make_struct_expr -> fn(
    name: *byte,
    alias: *byte,
    field_names: **byte,
    field_vals: **AST::AstNode,
    field_count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_deref`

```luma
pub #returns_ownership
make_deref -> fn(
    operand: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_addr`

```luma
pub #returns_ownership
make_addr -> fn(
    operand: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_grouping`

```luma
pub #returns_ownership
make_grouping -> fn(
    expr: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_input`

```luma
pub #returns_ownership
make_input -> fn(
    type_node: *AST::AstNode,
    msg: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_system`

```luma
pub #returns_ownership
make_system -> fn(
    command: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

