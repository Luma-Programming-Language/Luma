# Module: tc_generics

*Source: `src/typechecker/generics.lx`*

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `GenericTemplate`

| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `name` | *byte |  |
| `decl` | *AST::AstNode |  |
| `def_scope` | *CORE::Scope |  |
| `owner` | *byte |  |
| `method_is_static` | i64 |  |

### `Instantiation`

| Field | Type | Description |
|-------|------|-------------|
| `key` | *byte |  |
| `ftype` | *AST::AstNode |  |
| `mangled_name` | *byte |  |

### `Pending`

| Field | Type | Description |
|-------|------|-------------|
| `decl` | *AST::AstNode |  |
| `def_scope` | *CORE::Scope |  |
| `module_name` | *byte |  |
| `owner` | *byte |  |
| `is_static` | i64 |  |


## Functions

### `register_template`

```luma
pub register_template -> fn(
    module_name: *byte,
    name: *byte,
    decl: *AST::AstNode,
    def_scope: *CORE::Scope
) void
```

### `register_method_template`

```luma
pub #returns_ownership
register_method_template -> fn(
    module_name: *byte,
    owner: *byte,
    decl: *AST::AstNode,
    is_static: i64,
    def_scope: *CORE::Scope
) void
```

### `find_template`

```luma
pub find_template -> fn(
    module_name: *byte,
    name: *byte
) *GenericTemplate
```

### `pending_count`

```luma
pub pending_count -> fn(
) i64
```

### `pending_pop`

```luma
pub pending_pop -> fn(
) Pending
```

### `instantiate_function_call`

```luma
pub #returns_ownership
instantiate_function_call -> fn(
    call_node: *AST::CallNode,
    tmpl: *GenericTemplate,
    caller_scope: *CORE::Scope
) *AST::AstNode
```

### `instantiate_method_call`

```luma
pub #returns_ownership
instantiate_method_call -> fn(
    call_node: *AST::CallNode,
    owner: *byte,
    tmpl: *GenericTemplate,
    caller_scope: *CORE::Scope
) *AST::AstNode
```

### `resolve_nominal_type_ref`

```luma
pub #returns_ownership
resolve_nominal_type_ref -> fn(
    leaf: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `resolve_nominal_type_ref_in`

```luma
pub #returns_ownership
resolve_nominal_type_ref_in -> fn(
    leaf: *AST::AstNode,
    scope: *CORE::Scope,
    def_module_name: *byte
) *AST::AstNode
```

### `struct_template_has_method`

```luma
pub struct_template_has_method -> fn(
    sd: *AST::StructDeclNode,
    method_name: *byte
) bool
```

