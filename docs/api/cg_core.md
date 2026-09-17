# Module: cg_core

*Source: `src/codegen/codegen_core.lx`*

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `Buf`

| Field | Type | Description |
|-------|------|-------------|
| `data` | *byte |  |
| `len` | i64 |  |
| `cap` | i64 |  |

### `StructInfo`

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `module_name` | *byte |  |
| `decl` | *AST::StructDeclNode |  |
| `defined` | bool |  |
| `in_progress` | bool |  |

### `EnumInfo`

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `module_name` | *byte |  |
| `decl` | *AST::EnumDeclNode |  |

### `FuncInfo`

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `module_name` | *byte |  |
| `decl` | *AST::FuncDeclNode |  |
| `is_extern` | bool |  |
| `is_main` | bool |  |
| `owner` | *byte |  |
| `is_static` | bool |  |

### `GlobalInfo`

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `module_name` | *byte |  |
| `decl` | *AST::VarDeclNode |  |

### `UseAlias`

| Field | Type | Description |
|-------|------|-------------|
| `owner_module` | *byte |  |
| `alias` | *byte |  |
| `target_module` | *byte |  |

### `LocalBinding`

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `c_name` | *byte |  |

### `CodegenContext`

| Field | Type | Description |
|-------|------|-------------|
| `out` | *void |  |
| `structs` | VEC::Vector |  |
| `enums` | VEC::Vector |  |
| `funcs` | VEC::Vector |  |
| `globals` | VEC::Vector |  |
| `use_aliases` | VEC::Vector |  |
| `locals` | VEC::Vector |  |
| `current_module` | *byte |  |
| `current_return_type` | *AST::AstNode |  |
| `current_struct_owner` | *byte |  |
| `label_counter` | i64 |  |
| `os` | *byte |  |
| `defer_stack` | VEC::Vector |  |


## Functions

### `buf_init`

```luma
pub #returns_ownership
buf_init -> fn(
) *Buf
```

### `buf_str`

```luma
pub buf_str -> fn(
    b: *Buf,
    s: *byte
) void
```

### `buf_nstr`

```luma
pub buf_nstr -> fn(
    b: *Buf,
    s: *byte,
    n: i64
) void
```

### `buf_char`

```luma
pub buf_char -> fn(
    b: *Buf,
    c: byte
) void
```

### `buf_int`

```luma
pub buf_int -> fn(
    b: *Buf,
    v: i64
) void
```

### `buf_last_char`

```luma
pub buf_last_char -> fn(
    b: *Buf
) byte
```

### `buf_finish`

```luma
pub #returns_ownership
buf_finish -> fn(
    b: *Buf
) *byte
```

### `dup_str`

```luma
pub #returns_ownership
dup_str -> fn(
    s: *byte
) *byte
```

### `strcat2`

```luma
pub #returns_ownership
strcat2 -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `mangle2`

```luma
pub #returns_ownership
mangle2 -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `mangle3`

```luma
pub #returns_ownership
mangle3 -> fn(
    a: *byte,
    b: *byte,
    c: *byte
) *byte
```

### `ctx_init`

```luma
pub #returns_ownership
ctx_init -> fn(
    out: *void
) *CodegenContext
```

### `find_struct`

```luma
pub find_struct -> fn(
    ctx: *CodegenContext,
    name: *byte
) *StructInfo
```

### `find_enum`

```luma
pub find_enum -> fn(
    ctx: *CodegenContext,
    name: *byte
) *EnumInfo
```

### `find_func`

```luma
pub find_func -> fn(
    ctx: *CodegenContext,
    name: *byte
) *FuncInfo
```

### `find_method`

```luma
pub find_method -> fn(
    ctx: *CodegenContext,
    struct_name: *byte,
    method_name: *byte
) *FuncInfo
```

### `find_method_owner_containing`

```luma
pub find_method_owner_containing -> fn(
    ctx: *CodegenContext,
    struct_name: *byte,
    method_name: *byte
) *FuncInfo
```

### `find_func_in_module`

```luma
pub find_func_in_module -> fn(
    ctx: *CodegenContext,
    module_name: *byte,
    name: *byte
) *FuncInfo
```

### `find_global_in_module`

```luma
pub find_global_in_module -> fn(
    ctx: *CodegenContext,
    module_name: *byte,
    name: *byte
) *GlobalInfo
```

### `resolve_alias`

```luma
pub resolve_alias -> fn(
    ctx: *CodegenContext,
    owner_module: *byte,
    alias: *byte
) *byte
```

### `c_name_for_func`

```luma
pub #returns_ownership
c_name_for_func -> fn(
    f: *FuncInfo
) *byte
```

### `c_name_for_method`

```luma
pub #returns_ownership
c_name_for_method -> fn(
    f: *FuncInfo
) *byte
```

### `c_name_for_struct`

```luma
pub #returns_ownership
c_name_for_struct -> fn(
    s: *StructInfo
) *byte
```

### `c_name_for_enum`

```luma
pub #returns_ownership
c_name_for_enum -> fn(
    e: *EnumInfo
) *byte
```

### `c_name_for_enum_member`

```luma
pub #returns_ownership
c_name_for_enum_member -> fn(
    e: *EnumInfo,
    member: *byte
) *byte
```

### `resolve_os_for`

```luma
pub resolve_os_for -> fn(
    node: *AST::AstNode,
    os: *byte
) *AST::AstNode
```

### `next_label`

```luma
pub next_label -> fn(
    ctx: *CodegenContext
) i64
```

### `local_lookup`

```luma
pub local_lookup -> fn(
    ctx: *CodegenContext,
    name: *byte
) *byte
```

### `local_bind_as`

```luma
pub local_bind_as -> fn(
    ctx: *CodegenContext,
    name: *byte,
    c_name: *byte
) void
```

### `local_declare`

```luma
pub #returns_ownership
local_declare -> fn(
    ctx: *CodegenContext,
    name: *byte
) *byte
```

