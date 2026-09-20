# Module: tc_type

*Source: `src/typechecker/type.lx`*

Type-related helpers for the type checker.

Provides predicates over type AST nodes, constructors for builtin basic
types, type-compatibility (`types_match`, `is_cast_valid`), type-to-string
rendering, and bare/qualified nominal type name resolution.

## Table of Contents

- [Functions](#functions)


## Functions

### `dup_str`

Returns a heap-allocated copy of `s`.

```luma
      #returns_ownership
dup_str -> fn(
    s: *byte
) *byte
```

### `type_is_kind`

Returns true if `t` is non-null and its node kind is `kind`.

```luma
pub type_is_kind -> fn(
    t: *AST::AstNode,
    kind: i64
) bool
```

### `basic_name`

Returns the name of a basic type node.

```luma
pub basic_name -> fn(
    t: *AST::AstNode
) *byte
```

### `is_builtin_name`

Returns true if `name` is one of the builtin scalar types
(`i8..i64`, `u8..u64`, `f32`, `f64`, `bool`, `void`, `byte`).

```luma
pub is_builtin_name -> fn(
    name: *byte
) bool
```

### `is_basic_named`

Returns true if `t` is a basic type node named `name`.

```luma
pub is_basic_named -> fn(
    t: *AST::AstNode,
    name: *byte
) bool
```

### `is_numeric_type`

Returns true if `t` is a numeric basic type (int, uint, float, or byte).

```luma
pub is_numeric_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_integer_kind_name`

Returns true if `n` names a signed or unsigned integer type.

```luma
pub is_integer_kind_name -> fn(
    n: *byte
) bool
```

### `is_integer_type`

Returns true if `t` is an integer basic type (including `byte`).

```luma
pub is_integer_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_float_type`

Returns true if `t` is `f32` or `f64`.

```luma
pub is_float_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_pointer_type`

Returns true if `t` is a pointer type.

```luma
pub is_pointer_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_array_type`

Returns true if `t` is an array type.

```luma
pub is_array_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_function_type`

Returns true if `t` is a function type.

```luma
pub is_function_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_void_type`

Returns true if `t` is the `void` type.

```luma
pub is_void_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_bool_type`

Returns true if `t` is the `bool` type.

```luma
pub is_bool_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_nominal_type`

Returns true if `t` is a non-builtin named (struct/enum) type.

```luma
pub is_nominal_type -> fn(
    t: *AST::AstNode
) bool
```

### `is_pointer_to_function_type`

Returns true if `t` is a pointer whose pointee is a function type.

```luma
pub is_pointer_to_function_type -> fn(
    t: *AST::AstNode
) bool
```

### `make_int_type`

Returns a new `i32` type node.

```luma
pub #returns_ownership
make_int_type -> fn(
) *AST::AstNode
```

### `make_i8_type`

Returns a new `i8` type node.

```luma
pub #returns_ownership
make_i8_type -> fn(
) *AST::AstNode
```

### `make_i16_type`

Returns a new `i16` type node.

```luma
pub #returns_ownership
make_i16_type -> fn(
) *AST::AstNode
```

### `make_i32_type`

Returns a new `i32` type node.

```luma
pub #returns_ownership
make_i32_type -> fn(
) *AST::AstNode
```

### `make_i64_type`

Returns a new `i64` type node.

```luma
pub #returns_ownership
make_i64_type -> fn(
) *AST::AstNode
```

### `make_u8_type`

Returns a new `u8` type node.

```luma
pub #returns_ownership
make_u8_type -> fn(
) *AST::AstNode
```

### `make_u16_type`

Returns a new `u16` type node.

```luma
pub #returns_ownership
make_u16_type -> fn(
) *AST::AstNode
```

### `make_u32_type`

Returns a new `u32` type node.

```luma
pub #returns_ownership
make_u32_type -> fn(
) *AST::AstNode
```

### `make_u64_type`

Returns a new `u64` type node.

```luma
pub #returns_ownership
make_u64_type -> fn(
) *AST::AstNode
```

### `make_f32_type`

Returns a new `f32` type node.

```luma
pub #returns_ownership
make_f32_type -> fn(
) *AST::AstNode
```

### `make_f64_type`

Returns a new `f64` type node.

```luma
pub #returns_ownership
make_f64_type -> fn(
) *AST::AstNode
```

### `make_bool_type`

Returns a new `bool` type node.

```luma
pub #returns_ownership
make_bool_type -> fn(
) *AST::AstNode
```

### `make_void_type`

Returns a new `void` type node.

```luma
pub #returns_ownership
make_void_type -> fn(
) *AST::AstNode
```

### `make_byte_type`

Returns a new `byte` type node.

```luma
pub #returns_ownership
make_byte_type -> fn(
) *AST::AstNode
```

### `make_named_type`

Returns a basic type node named `name`.

```luma
pub #returns_ownership
make_named_type -> fn(
    name: *byte
) *AST::AstNode
```

### `make_string_type`

Returns a new pointer-to-`byte` (string) type node.

```luma
pub #returns_ownership
make_string_type -> fn(
) *AST::AstNode
```

### `make_voidptr_type`

Returns a new `*void` type node.

```luma
pub #returns_ownership
make_voidptr_type -> fn(
) *AST::AstNode
```

### `make_pointer_to`

Returns a new pointer type over `pointee`.

```luma
pub #returns_ownership
make_pointer_to -> fn(
    pointee: *AST::AstNode
) *AST::AstNode
```

### `numeric_rank`

Returns a promotion rank for numeric types (`f64` highest down to the 1-byte
integers), or -1 if `t` is not a numeric basic type.

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

### `match_basic`

Typecompat between two basic types. Exact names match exactly; `f32`<->`f64`
and any signed/unsigned integer pair are compatible; a non-builtin nominal
(enum) is compatible with an integer type.

```luma
      match_basic -> fn(
    a: *AST::AstNode,
    b: *AST::AstNode
) i64
```

### `match_pointer`

Typecompat between two pointer types, treating a `void` pointee on either
side as compatible with anything.

```luma
      match_pointer -> fn(
    a: *AST::AstNode,
    b: *AST::AstNode
) i64
```

### `match_array`

Typecompat between two array types. Element types must be compatible those
of the other; unsized arrays are compatible, and differing literal integer
sizes reject the match.

```luma
      match_array -> fn(
    a: *AST::AstNode,
    b: *AST::AstNode
) i64
```

### `match_array_pointer`

Array<->pointer (decay) compatibility, comparing the array element type to
the pointer pointee.

```luma
      match_array_pointer -> fn(
    arr: *AST::AstNode,
    ptr: *AST::AstNode
) i64
```

### `match_function`

Typecompat between two function types: equal parameter counts with matching
parameter and return types.

```luma
      match_function -> fn(
    a: *AST::AstNode,
    b: *AST::AstNode
) i64
```

### `types_match`

Determines compatibility between two type nodes, handling basic/pointer/
array/function kinds plus array<->pointer and function<->pointer coercion.


```luma
pub types_match -> fn(
    a: *AST::AstNode,
    b: *AST::AstNode
) i64
```

**Returns:**
`TYPE_MATCH_EXACT`, `TYPE_MATCH_COMPATIBLE`, or `TYPE_MATCH_NONE`.

### `is_cast_valid`

Returns true if an explicit cast from `from` to `to` is permitted.

```luma
pub is_cast_valid -> fn(
    from: *AST::AstNode,
    to: *AST::AstNode
) bool
```

### `append_str`

Appends `s` to `buf` at byte offset `pos`, returning the new offset.

```luma
      append_str -> fn(
    buf: *byte,
    pos: i64,
    s: *byte
) i64
```

### `type_to_string`

```luma
pub #returns_ownership
type_to_string -> fn(
    t: *AST::AstNode
) *byte
```

### `type_to_string_into`

Renders type `t` into `buf` starting at offset `pos`, returning the new
offset. Internal worker for `type_to_string`.

```luma
      type_to_string_into -> fn(
    t: *AST::AstNode,
    buf: *byte,
    pos: i64
) i64
```

### `type_to_string`

Returns a heap-allocated string rendering of type `t`
(e.g. `*i64`, `fn(i64) void`). The caller owns the result.

```luma
pub #returns_ownership
type_to_string -> fn(
    t: *AST::AstNode
) *byte
```

### `symbol_is_nominal_named`

Returns true if `sym` is non-null and its type is a bare nominal type named
`name`.

```luma
      symbol_is_nominal_named -> fn(
    sym: *CORE::Symbol,
    name: *byte
) bool
```

### `nominal_known_in_module`

Returns true if `mscope`'s own registry holds `name` as a bare nominal
symbol (struct or enum declared in that module).

```luma
pub nominal_known_in_module -> fn(
    mscope: *CORE::Scope,
    name: *byte
) bool
```

### `check_nominal_bare`

Validates a bare `name` used in type position: reports an error when it only
resolves via an import (must be qualified) or is not a type at all, and
defers the verdict when the name is not yet registered.

```luma
pub check_nominal_bare -> fn(
    name: *byte,
    scope: *CORE::Scope,
    err_node: *AST::AstNode
) void
```

### `resolve_type_leaf`

Validates and normalizes a single type leaf (`TYPE_BASIC` /
`TYPE_RESOLUTION`). Qualified names (`ALIAS::Name`) and generic references
(`Box<i64>`) are resolved down to a plain concrete named type.


```luma
      #returns_ownership
resolve_type_leaf -> fn(
    t: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

**Returns:**
The resolved type node (possibly a new node).

### `resolve_type_ref`

Recursively resolves every nominal type leaf reachable from `t`, including
through pointer/array/function wrappers. Safe to call repeatedly on the same
node.


```luma
pub #returns_ownership
resolve_type_ref -> fn(
    t: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

**Returns:**
`t` itself (wrappers are rewritten in place).

