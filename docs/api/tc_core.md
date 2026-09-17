# Module: tc_core

*Source: `src/typechecker/core.lx`*

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)

---

## Structures

### `TypeError`

| Field | Type | Description |
|-------|------|-------------|
| `message` | *byte |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `context` | *byte |  |

### `StaticAllocation`

| Field | Type | Description |
|-------|------|-------------|
| `line` | i64 |  |
| `col` | i64 |  |
| `variable_name` | *byte |  |
| `original_variable` | *byte |  |
| `has_matching_free` | bool |  |
| `free_count` | i64 |  |
| `conditional_free` | i64 |  |
| `use_after_free_count` | i64 |  |
| `aliases` | *VEC::Vector |  |
| `reported` | bool |  |
| `address_taken` | bool |  |
| `function_name` | *byte |  |
| `file_path` | *byte |  |

### `StaticMemoryAnalyzer`

| Field | Type | Description |
|-------|------|-------------|
| `allocations` | *VEC::Vector |  |
| `skip_memory_tracking` | bool |  |

### `Symbol`

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `type` | *AST::AstNode |  |
| `is_public` | bool |  |
| `is_mutable` | bool |  |
| `scope_depth` | i64 |  |
| `returns_ownership` | bool |  |
| `takes_ownership` | bool |  |

### `Scope`

| Field | Type | Description |
|-------|------|-------------|
| `parent` | *Scope |  |
| `symbols` | *VEC::Vector |  |
| `scope_name` | *byte |  |
| `depth` | i64 |  |
| `is_function_scope` | bool |  |
| `associated_node` | *AST::AstNode |  |
| `is_module_scope` | bool |  |
| `module_name` | *byte |  |
| `imported_modules` | *VEC::Vector |  |
| `returns_ownership` | bool |  |
| `takes_ownership` | bool |  |
| `memory_analyzer` | *StaticMemoryAnalyzer |  |
| `deferred_frees` | *VEC::Vector |  |
| `check_mem` | bool |  |
| `os` | *byte |  |

### `ModuleImport`

| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `alias` | *byte |  |
| `module_scope` | *Scope |  |

### `ModuleDependency`

| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `dependencies` | VEC::Vector |  |
| `processed` | bool |  |

### `GeneratedDecl`

| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `decl` | *AST::AstNode |  |
| `owner` | *byte |  |
| `is_static` | i64 |  |

### `UnresolvedTypeName`

| Field | Type | Description |
|-------|------|-------------|
| `module_scope` | *Scope |  |
| `name` | *byte |  |
| `err_node` | *AST::AstNode |  |

### `NominalDecl`

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `decl` | *AST::AstNode |  |


## Enumerations

### pub `TypeMatchResult`

**Values:**

- `TYPE_MATCH_NONE`
- `TYPE_MATCH_COMPATIBLE`
- `TYPE_MATCH_EXACT`


## Functions

### `record_generated_decl`

```luma
pub record_generated_decl -> fn(
    module_name: *byte,
    decl: *AST::AstNode,
    owner: *byte,
    is_static: i64
) void
```

### `generated_decl_count`

```luma
pub generated_decl_count -> fn(
) i64
```

### `generated_decl_at`

```luma
pub generated_decl_at -> fn(
    i: i64
) *GeneratedDecl
```

### `record_unresolved_type_name`

```luma
pub record_unresolved_type_name -> fn(
    module_scope: *Scope,
    name: *byte,
    err_node: *AST::AstNode
) void
```

### `unresolved_type_name_count`

```luma
pub unresolved_type_name_count -> fn(
) i64
```

### `unresolved_type_name_at`

```luma
pub unresolved_type_name_at -> fn(
    i: i64
) *UnresolvedTypeName
```

### `nominal_registry_clear`

```luma
pub nominal_registry_clear -> fn(
) void
```

### `record_nominal_decl`

```luma
pub record_nominal_decl -> fn(
    name: *byte,
    decl: *AST::AstNode
) void
```

### `find_nominal_decl`

```luma
pub find_nominal_decl -> fn(
    name: *byte
) *AST::AstNode
```

