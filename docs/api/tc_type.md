# Module: tc_type

*Source: `src/typechecker/type.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `type_is_kind`

```luma
pub type_is_kind -> fn(
    t: *AST::AstNode,
    kind: i64
) bool
```

### `basic_name`

```luma
pub basic_name -> fn(
    t: *AST::AstNode
) *byte
```

### `is_builtin_name`

```luma
pub is_builtin_name -> fn(
    name: *byte
) bool
```

### `is_basic_named`

```luma
pub is_basic_named -> fn(
    t: *AST::AstNode,
    name: *byte
) bool
```

### `is_numeric_type`

```luma
pub is_numeric_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_integer_kind_name`

```luma
pub is_integer_kind_name -> fn(
    n: *byte
) bool
```

### `is_integer_type`

```luma
pub is_integer_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_float_type`

```luma
pub is_float_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_pointer_type`

```luma
pub is_pointer_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_array_type`

```luma
pub is_array_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_function_type`

```luma
pub is_function_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_void_type`

```luma
pub is_void_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_bool_type`

```luma
pub is_bool_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_nominal_type`

```luma
pub is_nominal_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_pointer_to_function_type`

```luma
pub is_pointer_to_function_type -> fn(
    t: *AST::AstNode
) bool
```

### `make_int_type`

```luma
pub #returns_ownership
make_int_type -> fn(
) *AST::AstNode
```

### `make_i8_type`

```luma
pub #returns_ownership
make_i8_type -> fn(
) *AST::AstNode
```

### `make_i16_type`

```luma
pub #returns_ownership
make_i16_type -> fn(
) *AST::AstNode
```

### `make_i32_type`

```luma
pub #returns_ownership
make_i32_type -> fn(
) *AST::AstNode
```

### `make_i64_type`

```luma
pub #returns_ownership
make_i64_type -> fn(
) *AST::AstNode
```

### `make_u8_type`

```luma
pub #returns_ownership
make_u8_type -> fn(
) *AST::AstNode
```

### `make_u16_type`

```luma
pub #returns_ownership
make_u16_type -> fn(
) *AST::AstNode
```

### `make_u32_type`

```luma
pub #returns_ownership
make_u32_type -> fn(
) *AST::AstNode
```

### `make_u64_type`

```luma
pub #returns_ownership
make_u64_type -> fn(
) *AST::AstNode
```

### `make_f32_type`

```luma
pub #returns_ownership
make_f32_type -> fn(
) *AST::AstNode
```

### `make_f64_type`

```luma
pub #returns_ownership
make_f64_type -> fn(
) *AST::AstNode
```

### `make_bool_type`

```luma
pub #returns_ownership
make_bool_type -> fn(
) *AST::AstNode
```

### `make_void_type`

```luma
pub #returns_ownership
make_void_type -> fn(
) *AST::AstNode
```

### `make_byte_type`

```luma
pub #returns_ownership
make_byte_type -> fn(
) *AST::AstNode
```

### `make_named_type`

```luma
pub #returns_ownership
make_named_type -> fn(
    name: *byte
) *AST::AstNode
```

### `make_string_type`

```luma
pub #returns_ownership
make_string_type -> fn(
) *AST::AstNode
```

### `make_voidptr_type`

```luma
pub #returns_ownership
make_voidptr_type -> fn(
) *AST::AstNode
```

### `make_pointer_to`

```luma
pub #returns_ownership
make_pointer_to -> fn(
    pointee: *AST::AstNode
) *AST::AstNode
```

### `numeric_rank`

```luma
pub numeric_rank -> fn(
    t: *AST::AstNode
) i64
```

### `types_match`

```luma
pub types_match -> fn(
    a: *AST::AstNode,
    b: *AST::AstNode
) i64
```

### `types_match`

```luma
pub types_match -> fn(
    a: *AST::AstNode,
    b: *AST::AstNode
) i64
```

### `is_cast_valid`

```luma
pub is_cast_valid -> fn(
    from: *AST::AstNode,
    to: *AST::AstNode
) bool
```

### `type_to_string`

```luma
pub #returns_ownership
type_to_string -> fn(
    t: *AST::AstNode
) *byte
```

### `type_to_string`

```luma
pub #returns_ownership
type_to_string -> fn(
    t: *AST::AstNode
) *byte
```

### `nominal_known_in_module`

```luma
pub nominal_known_in_module -> fn(
    mscope: *CORE::Scope,
    name: *byte
) bool
```

### `check_nominal_bare`

```luma
pub check_nominal_bare -> fn(
    name: *byte,
    scope: *CORE::Scope,
    err_node: *AST::AstNode
) void
```

### `resolve_type_ref`

```luma
pub #returns_ownership
resolve_type_ref -> fn(
    t: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

