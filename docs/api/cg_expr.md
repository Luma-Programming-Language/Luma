# Module: cg_expr

*Source: `src/codegen/codegen_expr.lx`*

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `FieldLookup`

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

### `gen_output_call`

```luma
pub gen_output_call -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    args: **AST::AstNode,
    arg_count: i64
) void
```

### `gen_array_literal`

```luma
pub gen_array_literal -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode,
    as_compound_literal: bool
) void
```

### `gen_expr`

```luma
pub gen_expr -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    expr: *AST::AstNode
) void
```

