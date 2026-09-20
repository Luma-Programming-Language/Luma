# Module: tc_core

*Source: `src/typechecker/core.lx`*

Core shared data types for the type checker.

Defines the enumerations and records used throughout type checking —
match results, scopes, symbols, static-memory analysis state, module
imports/dependencies — plus module-level registries for synthesized
(instantiated generic) declarations, deferred unresolved type names, and
nominal (struct/enum) declarations.

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `TypeError`

One typecheck error: message text, source position, and context.

| Field | Type | Description |
|-------|------|-------------|
| `message` | *byte |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `context` | *byte |  |

### `StaticAllocation`

Tracks a single allocation site for the static memory checker.

Records the variable, its matching free/alias/address-taken state, and the
counts used to report use-after-free and un-freed allocations.

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

Per-module scratch state for the static memory checker. One analyzer is
shared (via pointer) across every scope in a module.

| Field | Type | Description |
|-------|------|-------------|
| `allocations` | *VEC::Vector |  |
| `skip_memory_tracking` | bool |  |

### `Symbol`

A name bound in a scope: its type, visibility, mutability, ownership
attributes, and declaring scope depth.

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

A lexical scope or, for the root of each module, the module scope itself.

Links to its parent, its symbols and imported modules, the enclosing
function/module, ownership attributes, and the shared memory analyzer.

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

A resolved `@use "module" as alias` import, bound to the imported module's
scope.

| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `alias` | *byte |  |
| `module_scope` | *Scope |  |

### `ModuleDependency`

Records one module's dependency set for topological ordering of typecheck.

| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `dependencies` | VEC::Vector |  |
| `processed` | bool |  |

### `GeneratedDecl`

A concrete generic instantiation synthesized during typechecking.

Instantiated declarations are appended here (in addition to any module
body) so codegen's Pass A can emit a prototype/definition for each. For an
instantiated generic method, `owner` names the owning struct and `is_static`
records whether it is a static method.

| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `decl` | *AST::AstNode |  |
| `owner` | *byte |  |
| `is_static` | i64 |  |

### `UnresolvedTypeName`

A name referenced in bare type position that matched nothing when first
examined.

Type resolution runs in multiple passes, so a name used before its
declaration appears later in a module is expected to resolve by the end of
typecheck; reporting is deferred until then.

| Field | Type | Description |
|-------|------|-------------|
| `module_scope` | *Scope |  |
| `name` | *byte |  |
| `err_node` | *AST::AstNode |  |

### `NominalDecl`

One struct/enum declaration registered by raw name.

The declaration (not just the scope symbol) is needed to tell real fields
apart from promoted embedded ones when validating struct-literal field
names.

| Field | Type | Description |
|-------|------|-------------|
| `name` | *byte |  |
| `decl` | *AST::AstNode |  |


## Enumerations

### pub `TypeMatchResult`

Result of comparing two types: none, compatible, or exact.

**Values:**

- `TYPE_MATCH_NONE`
- `TYPE_MATCH_COMPATIBLE`
- `TYPE_MATCH_EXACT`


## Functions

### `generated_decls_init`

Lazily initializes the generated-decls registry exactly once.

```luma
      generated_decls_init -> fn(
) void
```

### `record_generated_decl`

Appends a synthesized generic instantiation to the registry, initializing
it if needed.

```luma
pub record_generated_decl -> fn(
    module_name: *byte,
    decl: *AST::AstNode,
    owner: *byte,
    is_static: i64
) void
```

### `generated_decl_count`

Returns how many generated declarations have been recorded.

```luma
pub generated_decl_count -> fn(
) i64
```

### `generated_decl_at`

Returns the generated declaration at index `i`.

```luma
pub generated_decl_at -> fn(
    i: i64
) *GeneratedDecl
```

### `unresolved_types_init`

Lazily initializes the unresolved-type registry exactly once.

```luma
      unresolved_types_init -> fn(
) void
```

### `record_unresolved_type_name`

Records a deferred unresolved type name, skipping duplicates for the same
error node.

```luma
pub record_unresolved_type_name -> fn(
    module_scope: *Scope,
    name: *byte,
    err_node: *AST::AstNode
) void
```

### `unresolved_type_name_count`

Returns how many unresolved type names are pending.

```luma
pub unresolved_type_name_count -> fn(
) i64
```

### `unresolved_type_name_at`

Returns the unresolved type name at index `i`.

```luma
pub unresolved_type_name_at -> fn(
    i: i64
) *UnresolvedTypeName
```

### `nominals_init`

Lazily initializes the nominal registry exactly once.

```luma
      nominals_init -> fn(
) void
```

### `nominal_registry_clear`

Clears the nominal registry. Called once per typecheck run, since the
registered declaration pointers are only valid while the current AST is
alive.

```luma
pub nominal_registry_clear -> fn(
) void
```

### `record_nominal_decl`

Registers a struct/enum declaration under `name`, initializing the registry
if needed.

```luma
pub record_nominal_decl -> fn(
    name: *byte,
    decl: *AST::AstNode
) void
```

### `find_nominal_decl`

Looks up a registered struct/enum declaration by name, or null.

```luma
pub find_nominal_decl -> fn(
    name: *byte
) *AST::AstNode
```


## Variables

- **`g_generated_decls`** : VEC::Vector *(let)* — Module-level registry of synthesized generic instantiations.
- **`g_generated_decls_ready`** : bool *(let)* — Whether the generated-decls registry has been initialized.
- **`g_unresolved_types_ready`** : bool *(let)* — Whether the unresolved-type registry has been initialized.
- **`g_unresolved_types`** : VEC::Vector *(let)* — Module-level registry of deferred, still-unresolved type names.
- **`g_nominals_ready`** : bool *(let)* — Whether the nominal registry has been initialized.
- **`g_nominals`** : VEC::Vector *(let)* — Module-level registry of struct/enum declarations, keyed by name.
