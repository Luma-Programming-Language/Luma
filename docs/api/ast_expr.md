# Module: ast_expr

*Source: `src/ast/expr.lx`*

Expression AST node constructors.

Factory functions that allocate each kind of expression node used by the
parser and return it as a generic `*AST::AstNode`.

## Table of Contents

- [Functions](#functions)


## Functions

### `make_literal_int`

Creates an integer literal expression node.

```luma
pub #returns_ownership
make_literal_int -> fn(
    int_val: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_float`

Creates a float literal expression node.

```luma
pub #returns_ownership
make_literal_float -> fn(
    float_val: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_string`

Creates a string literal expression node.

```luma
pub #returns_ownership
make_literal_string -> fn(
    string_val: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_char`

Creates a character literal expression node.

```luma
pub #returns_ownership
make_literal_char -> fn(
    char_val: byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_bool`

Creates a boolean literal expression node.

```luma
pub #returns_ownership
make_literal_bool -> fn(
    bool_val: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_literal_null`

Creates a null literal expression node.

```luma
pub #returns_ownership
make_literal_null -> fn(
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_identifier`

Creates an identifier expression node.

```luma
pub #returns_ownership
make_identifier -> fn(
    name: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_binary`

Creates a binary expression node from an operator and two operands.

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

Creates a unary expression node from an operator and one operand.

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

Creates a function-call expression node.

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

Creates an assignment expression node.

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

Creates a ternary (`condition ? then_expr : else_expr`) expression node.

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

Creates a member-access expression node (`object.member`).

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

Creates an index expression node (`object[index]`).

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

Creates an array-literal expression node.

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

Creates a cast expression node.

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

Creates a memory-allocation (`alloc`) expression node.

```luma
pub #returns_ownership
make_alloc -> fn(
    size: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_free`

Creates a memory-free (`free`) expression node.

```luma
pub #returns_ownership
make_free -> fn(
    ptr: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_memcpy`

Creates a `memcpy` expression node (`to`, `from`, `size`).

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

Creates a `sizeof` expression node; `is_type` selects a type operand
over a value operand.

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

Creates a `syscall` expression node.

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

Creates a struct-literal expression node.

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

Creates a pointer-dereference expression node.

```luma
pub #returns_ownership
make_deref -> fn(
    operand: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_addr`

Creates an address-of expression node.

```luma
pub #returns_ownership
make_addr -> fn(
    operand: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_grouping`

Creates a parenthesized grouping expression node.

```luma
pub #returns_ownership
make_grouping -> fn(
    expr: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_input`

Creates an input expression node that reads a value of `type_node`,
optionally prompting with `msg`.

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

Creates a `system` command-execution expression node.

```luma
pub #returns_ownership
make_system -> fn(
    command: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

