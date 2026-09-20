# Module: cg_core

*Source: `src/codegen/codegen_core.lx`*

Core code-generation support shared by the C backend.

Growable output-buffer helpers, C identifier mangling, and the registrar
and lookup infrastructure (CodegenContext and its struct/enum/func/global
registries) that the type/expr/stmt codegen passes build on.

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `Buf`

Growable byte buffer used to assemble emitted C output before it is
flushed to the generated file.

| Field | Type | Description |
|-------|------|-------------|
| `data` | *byte |  |
| `len` | i64 |  |
| `cap` | i64 |  |

### `StructInfo`

Registrar entry for a struct: its name, owning module, AST decl node, and
emission state (whether its C body has been emitted / is mid-emission).

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `module_name` | *byte |  |
| `decl` | *AST::StructDeclNode |  |
| `defined` | bool |  |
| `in_progress` | bool |  |

### `EnumInfo`

Registrar entry for an enum: its name, owning module, and AST decl node.

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `module_name` | *byte |  |
| `decl` | *AST::EnumDeclNode |  |

### `FuncInfo`

Registrar entry for a function or method: name, owning module, AST decl
node, and the flags (extern/main/owner/static) that drive its emission.

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

Registrar entry for a module-level global variable and its AST decl node.

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `module_name` | *byte |  |
| `decl` | *AST::VarDeclNode |  |

### `UseAlias`

Maps an alias used inside one module to the real module it points at.

| Field | Type | Description |
|-------|------|-------------|
| `owner_module` | *byte |  |
| `alias` | *byte |  |
| `target_module` | *byte |  |

### `LocalBinding`

A live local or parameter: Luma source name bound to the C identifier
actually emitted (uniquified with a suffix on shadowing collisions).

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `c_name` | *byte |  |

### `CodegenContext`

Per-compilation state threaded through all codegen: the output FILE*,
the registries above, and the active function's bookkeeping (live locals,
pending defers, current module/return type/struct owner, label counter).

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

Allocates and returns a new buffer with a 4096-byte initial capacity.

```luma
pub #returns_ownership
buf_init -> fn(
) *Buf
```

### `buf_ensure`

Ensures `b` can hold `b.len + extra + 1` bytes, growing (doubling) the
underlying allocation as needed.

```luma
      buf_ensure -> fn(
    b: *Buf,
    extra: i64
) void
```

### `buf_str`

Appends the null-terminated string `s` to `b` (returning early if `s` is null).

```luma
pub buf_str -> fn(
    b: *Buf,
    s: *byte
) void
```

### `buf_nstr`

Appends the first `n` bytes of `s` to `b`.

```luma
pub buf_nstr -> fn(
    b: *Buf,
    s: *byte,
    n: i64
) void
```

### `buf_char`

Appends the single byte `c` to `b`.

```luma
pub buf_char -> fn(
    b: *Buf,
    c: byte
) void
```

### `buf_int`

Appends the decimal representation of the integer `v` to `b`.

```luma
pub buf_int -> fn(
    b: *Buf,
    v: i64
) void
```

### `buf_last_char`

Returns the last byte in `b`, or 0 if the buffer is empty.

```luma
pub buf_last_char -> fn(
    b: *Buf
) byte
```

### `buf_finish`

Null-terminates `b`'s contents, returns them, and frees the buffer
itself; caller owns the returned string.

```luma
pub #returns_ownership
buf_finish -> fn(
    b: *Buf
) *byte
```

### `dup_str`

Returns a heap-allocated copy of the null-terminated string `s`.

```luma
pub #returns_ownership
dup_str -> fn(
    s: *byte
) *byte
```

### `strcat2`

Returns a heap-allocated string combining `a` followed by `b`.

```luma
pub #returns_ownership
strcat2 -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `mangle2`

Mangles `a` and `b` into an `a__b` C identifier — the `module__name`
symbol-naming scheme. Caller owns the result.

```luma
pub #returns_ownership
mangle2 -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `mangle3`

Mangles `a`, `b`, and `c` into an `a__b_c` C identifier — the
`module__Struct_method` symbol-naming scheme. Caller owns the result.

```luma
pub #returns_ownership
mangle3 -> fn(
    a: *byte,
    b: *byte,
    c: *byte
) *byte
```

### `ctx_init`

Allocates a `CodegenContext` writing to `out`, with empty registries and
defaulted bookkeeping fields.

```luma
pub #returns_ownership
ctx_init -> fn(
    out: *void
) *CodegenContext
```

### `find_struct`

Returns the `StructInfo` registered under `name`, or null.

```luma
pub find_struct -> fn(
    ctx: *CodegenContext,
    name: *byte
) *StructInfo
```

### `find_enum`

Returns the `EnumInfo` registered under `name`, or null.

```luma
pub find_enum -> fn(
    ctx: *CodegenContext,
    name: *byte
) *EnumInfo
```

### `find_func`

Returns the first non-method `FuncInfo` registered under `name` (any
module), or null.

```luma
pub find_func -> fn(
    ctx: *CodegenContext,
    name: *byte
) *FuncInfo
```

### `find_method`

Returns the method `FuncInfo` defined as `method_name` on `struct_name`,
or null.

```luma
pub find_method -> fn(
    ctx: *CodegenContext,
    struct_name: *byte,
    method_name: *byte
) *FuncInfo
```

### `find_method_owner_containing`

Static-method fallback for concrete generic templates: returns the method
named `method_name` whose registered owner embeds `struct_name` followed
by the `__` mangling separator, or null.

```luma
pub find_method_owner_containing -> fn(
    ctx: *CodegenContext,
    struct_name: *byte,
    method_name: *byte
) *FuncInfo
```

### `find_func_in_module`

Returns the non-method `FuncInfo` for `name` inside `module_name`, or null.

```luma
pub find_func_in_module -> fn(
    ctx: *CodegenContext,
    module_name: *byte,
    name: *byte
) *FuncInfo
```

### `find_global_in_module`

Returns the `GlobalInfo` for `name` inside `module_name`, or null.

```luma
pub find_global_in_module -> fn(
    ctx: *CodegenContext,
    module_name: *byte,
    name: *byte
) *GlobalInfo
```

### `resolve_alias`

Resolves `alias` used inside `owner_module` to the real module name it
points at, or null.

```luma
pub resolve_alias -> fn(
    ctx: *CodegenContext,
    owner_module: *byte,
    alias: *byte
) *byte
```

### `c_name_for_func`

Returns the C name to emit for `f`: "main" for main, the bare name for
body-less extern functions, otherwise a `module__name` mangle.
Caller owns the result.

```luma
pub #returns_ownership
c_name_for_func -> fn(
    f: *FuncInfo
) *byte
```

### `c_name_for_method`

Returns the `module__Struct__method` C name for method `f`.
Caller owns the result.

```luma
pub #returns_ownership
c_name_for_method -> fn(
    f: *FuncInfo
) *byte
```

### `c_name_for_struct`

Returns the `module__name` C name for struct `s`. Caller owns the result.

```luma
pub #returns_ownership
c_name_for_struct -> fn(
    s: *StructInfo
) *byte
```

### `c_name_for_enum`

Returns the `module__name` C name for enum `e`. Caller owns the result.

```luma
pub #returns_ownership
c_name_for_enum -> fn(
    e: *EnumInfo
) *byte
```

### `c_name_for_enum_member`

Returns the `module__Enum__member` C name for enum member `member`.
Caller owns the result.

```luma
pub #returns_ownership
c_name_for_enum_member -> fn(
    e: *EnumInfo,
    member: *byte
) *byte
```

### `resolve_os_for`

Resolves an `@os {}` node to the body for `os`: the matching platform
arm, the default body if present, or null.

```luma
pub resolve_os_for -> fn(
    node: *AST::AstNode,
    os: *byte
) *AST::AstNode
```

### `next_label`

Returns the next value of the context-wide label/identifier counter.

```luma
pub next_label -> fn(
    ctx: *CodegenContext
) i64
```

### `local_lookup`

Returns the emitted C name for the innermost live local/param named
`name` (newest binding first), or null.

```luma
pub local_lookup -> fn(
    ctx: *CodegenContext,
    name: *byte
) *byte
```

### `local_bind_as`

Registers a live local binding mapping Luma `name` to C name `c_name`.

```luma
pub local_bind_as -> fn(
    ctx: *CodegenContext,
    name: *byte,
    c_name: *byte
) void
```

### `local_declare`

Registers `name` as a live local and returns its C name, uniquifying it
with a numeric suffix if already bound (shadowing). Caller owns the result.

```luma
pub #returns_ownership
local_declare -> fn(
    ctx: *CodegenContext,
    name: *byte
) *byte
```

