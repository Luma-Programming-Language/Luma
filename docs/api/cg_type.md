# Module: cg_type

*Source: `src/codegen/codegen_type.lx`*

C type emission for the codegen backend.

Maps Luma type nodes to C type strings and declarators: basic type names,
pointer/array/function declarators (spiral rule), and array-bound sizes.

## Table of Contents

- [Functions](#functions)


## Functions

### `basic_c_name`

Maps a Luma basic type name to its C fixed-width equivalent, or null if
the name isn't a recognized basic type.

```luma
      basic_c_name -> fn(
    name: *byte
) *byte
```

### `c_base_type_string`

Resolves a TYPE_BASIC / TYPE_RESOLUTION node's leaf type to its C spelling
(fixed-width builtin, mangled struct/enum name, or the bare name if
unknown). Caller owns the result.

```luma
pub #returns_ownership
c_base_type_string -> fn(
    ctx: *CG::CodegenContext,
    t: *AST::AstNode
) *byte
```

### `c_type_string`

```luma
pub #returns_ownership
c_type_string -> fn(
    ctx: *CG::CodegenContext,
    t: *AST::AstNode
) *byte
```

### `emit_value_declarator`

```luma
pub emit_value_declarator -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    t: *AST::AstNode,
    name: *byte
) void
```

### `emit_array_size`

Emits the C expression for an array-size node (`[i64; 10]`, `[i64; size]`,
simple arithmetic on those) without invoking full expression codegen,
which would be circular here; non-literal sizes become plain C VLAs.

```luma
      emit_array_size -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    node: *AST::AstNode
) void
```

### `emit_declarator`

Emits a full C declarator for `name` of Luma type `t` — standard
spiral-rule recursion covering pointer, array, and function types
(e.g. "long long x", "long long x[10]", "long long (*x)(long long)").

```luma
pub emit_declarator -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    t: *AST::AstNode,
    name: *byte
) void
```

### `emit_value_declarator`

Entry point for value-position declarators (struct fields, locals, params,
array elements, return types): a bare `fn (...) T` is wrapped as `(*name)`
so it becomes a valid C function pointer, then delegates to
emit_declarator.

```luma
pub emit_value_declarator -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    t: *AST::AstNode,
    name: *byte
) void
```

### `c_type_string`

Returns the C type string for `t` with no variable name attached (casts,
sizeof, parameter-list entries). Caller owns the result.

```luma
pub #returns_ownership
c_type_string -> fn(
    ctx: *CG::CodegenContext,
    t: *AST::AstNode
) *byte
```

