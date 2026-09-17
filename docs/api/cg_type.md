# Module: cg_type

*Source: `src/codegen/codegen_type.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `c_base_type_string`

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

### `emit_declarator`

```luma
pub emit_declarator -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    t: *AST::AstNode,
    name: *byte
) void
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

### `c_type_string`

```luma
pub #returns_ownership
c_type_string -> fn(
    ctx: *CG::CodegenContext,
    t: *AST::AstNode
) *byte
```

