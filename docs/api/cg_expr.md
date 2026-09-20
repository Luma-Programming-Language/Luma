# Module: cg_expr

*Source: `src/codegen/codegen_expr.lx`*

Expression codegen for the C backend.

Dispatches an expression AST node and writes the corresponding C
expression into the output buffer: literals, operators, calls, member
access, struct/array literals, casts, and builtins.

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `FieldLookup`

Result of resolving a member against a struct: whether a field or method
was found, its type, the dotted C path from the struct root, and (for
methods) the struct that actually owns the method.

| Field | Type | Description |
|-------|------|-------------|
| `found` | bool |  |
| `is_method` | bool |  |
| `type_node` | *AST::AstNode |  |
| `c_path` | *byte |  |
| `owner` | *byte |  |


## Functions

### `gen_expr`

```luma
pub gen_expr -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

### `static_type_of`

```luma
      static_type_of -> fn(
    ctx: *CG::CodegenContext,
    expr: *AST::AstNode
) *AST::AstNode
```

### `strip_ptr`

Returns `t`'s pointee type if it is a pointer type, else `t` unchanged.

```luma
      strip_ptr -> fn(
    t: *AST::AstNode
) *AST::AstNode
```

### `input_helper_name`

Returns the input helper C function name backing `input<T>(...)` for
type `t`, or null if T isn't one of the readable types yet.

```luma
      input_helper_name -> fn(
    t: *AST::AstNode
) *byte
```

### `not_found`

Returns a `FieldLookup` with `found` false.

```luma
      not_found -> fn(
) FieldLookup
```

### `resolve_field`

Looks `member` up on `struct_name` — direct public/private fields and
methods first, then recursively through `...Type,` embedded members.
Returns the describing `FieldLookup`, or a not-found lookup.

```luma
      resolve_field -> fn(
    ctx: *CG::CodegenContext,
    struct_name: *byte,
    member: *byte
) FieldLookup
```

### `find_embedded_path`

Dotted C path from `struct_name` down to an `...target,` embedded member
of type `target` (direct or nested), or null if not embedded anywhere.
Drives explicit base-method dispatch. Caller owns the result.

```luma
      #returns_ownership
find_embedded_path -> fn(
    ctx: *CG::CodegenContext,
    struct_name: *byte,
    target: *byte
) *byte
```

### `static_type_of`

Static type of an arbitrary expression, used to drive member access and
method-call resolution; falls back to `expr.resolved_type` where the
embedding-aware walk isn't needed.

```luma
      static_type_of -> fn(
    ctx: *CG::CodegenContext,
    expr: *AST::AstNode
) *AST::AstNode
```

### `emit_c_string_literal`

Writes `s` as a C string literal, escaping quotes, backslashes, and the
common control characters.

```luma
      emit_c_string_literal -> fn(
    buf: *CG::Buf,
    s: *byte
) void
```

### `emit_c_char_literal`

Writes the byte `c` as a C char literal, escaping quotes, backslashes, and
the common control characters.

```luma
      emit_c_char_literal -> fn(
    buf: *CG::Buf,
    c: byte
) void
```

### `gen_output_arg`

Writes the C argument expression for one `output(...)` argument, cast to
match its printf format (bool -> "true"/"false", byte -> int, floats ->
double, pointers as-is, else long long).

```luma
      gen_output_arg -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    arg: *AST::AstNode
) void
```

### `output_fmt_for`

Returns the printf format specifier matching one `output(...)` argument's
static type.

```luma
      output_fmt_for -> fn(
    ctx: *CG::CodegenContext,
    arg: *AST::AstNode
) *byte
```

### `gen_output_call`

Writes one `printf(...)` statement per `output(...)` argument.

```luma
pub gen_output_call -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    args: **AST::AstNode,
    arg_count: i64
) void
```

### `gen_call`

Emits a call expression: output/outputln lowered to printf, plain
identifier calls, module-qualified and static-method calls, explicit
base-method dispatch, instance-method calls (self passed by pointer), and
a function-pointer fallthrough.

```luma
      gen_call -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

### `gen_member`

Emits a member access: module-qualified globals/functions and enum members
(compiletime), embedding-aware field paths, and plain `.`/`->` field
access.

```luma
      gen_member -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

### `gen_struct_literal`

Emits a C compound literal for a struct expression, routing promoted
`...Embedded` fields to their dotted C paths.

```luma
      gen_struct_literal -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

### `binop_str`

Returns the C operator string for a `BinaryOp`, or null for operators
with no direct C spelling (pow, range).

```luma
      binop_str -> fn(
    op: i64
) *byte
```

### `gen_binary`

Emits a binary expression: `**` lowers to a `pow()` call, `..` to a
`__luma_range_t{ ... }` value, everything else to a parenthesized infix
C expression.

```luma
      gen_binary -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

### `gen_unary`

Emits a unary expression: postfix/prefix inc/dec and the prefix `!`, `-`,
`+`, `~` operators as parenthesized C.

```luma
      gen_unary -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

### `gen_array_literal`

Emits an array literal: a bare `{ ... }` brace list for initializer
position, or a C99 compound literal `(T[]){ ... }` for expression
position.

```luma
pub gen_array_literal -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode,
    as_compound_literal: bool
) void
```

### `gen_expr`

Top-level expression dispatcher: routes an expression node by kind to the
per-node generators, emitting the C expression into `buf`.

```luma
pub gen_expr -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

