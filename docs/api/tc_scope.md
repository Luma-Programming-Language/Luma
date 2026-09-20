# Module: tc_scope

*Source: `src/typechecker/scope.lx`*

Scope management for the type checker.

Provides arena-backed scope creation and teardown, symbol lookup and
insertion, module-import resolution, and the static-memory analyzer's
allocation/free/alias tracking API.

## Table of Contents

- [Functions](#functions)
- [Variables](#variables)


## Functions

### `scope_arena_init`

Initializes the scope arena. Call once before typechecking starts (pairs
with `scope_arena_teardown`); repeatable since teardown resets the ready
flag.

```luma
pub scope_arena_init -> fn(
) void
```

### `scope_arena_teardown`

Frees every scope's backing vectors, then the arena itself. Call once after
typechecking finishes; no `*Scope` is valid afterwards.

```luma
pub scope_arena_teardown -> fn(
) void
```

### `track_scope`

Records `s` in `g_all_scopes` for later teardown.

```luma
      track_scope -> fn(
    s: *CORE::Scope
) void
```

### `new_vector`

Allocates a `Vector` from the arena (struct) and heap (data buffer),
wrapping the fixed `SCOPE_VEC_CAPACITY`.

```luma
      #returns_ownership
new_vector -> fn(
    elem_size: i64
) *VEC::Vector
```

### `scope_init`

Creates a new root/module scope with its own static memory analyzer.

```luma
pub #returns_ownership
scope_init -> fn(
    name: *byte,
    check_mem: bool,
    os: *byte
) *CORE::Scope
```

### `scope_create_child`

Creates a child scope of `parent`, inheriting its ownership/OS flags and
sharing its memory analyzer.

```luma
pub #returns_ownership
scope_create_child -> fn(
    parent: *CORE::Scope,
    name: *byte
) *CORE::Scope
```

### `find_containing_module`

Walks the scope chain upward to the nearest module scope, or null.

```luma
pub find_containing_module -> fn(
    scope: *CORE::Scope
) *CORE::Scope
```

### `get_enclosing_function`

Walks the scope chain upward to the nearest function scope, or null.

```luma
pub get_enclosing_function -> fn(
    scope: *CORE::Scope
) *CORE::Scope
```

### `scope_is_conditional`

Returns true if `scope` lies inside a conditional or loop branch nested
within `fscope`.

```luma
pub scope_is_conditional -> fn(
    scope: *CORE::Scope,
    fscope: *CORE::Scope
) bool
```

### `add_module_import`

Appends a resolved module import to `scope`'s imported-modules list.

```luma
pub add_module_import -> fn(
    scope: *CORE::Scope,
    module_name: *byte,
    alias: *byte,
    module_scope: *CORE::Scope
) void
```

### `find_module_import`

Searches `scope` and its ancestors for an import with the given alias.

```luma
pub find_module_import -> fn(
    scope: *CORE::Scope,
    alias: *byte
) *CORE::ModuleImport
```

### `scope_lookup_current`

Looks up `name` directly in `scope` only (no ancestor walk).

```luma
pub scope_lookup_current -> fn(
    scope: *CORE::Scope,
    name: *byte
) *CORE::Symbol
```

### `scope_add_symbol_full`

Adds a symbol to `scope` with full ownership attributes.


```luma
pub scope_add_symbol_full -> fn(
    scope: *CORE::Scope,
    name: *byte,
    type: *AST::AstNode,
    is_public: bool,
    is_mutable: bool,
    returns_ownership: bool,
    takes_ownership: bool
) bool
```

**Returns:**
False if `name` is already bound in this scope, true on success.

### `scope_add_symbol`

Adds a symbol to `scope` with default (false) ownership attributes.


```luma
pub scope_add_symbol -> fn(
    scope: *CORE::Scope,
    name: *byte,
    type: *AST::AstNode,
    is_public: bool,
    is_mutable: bool
) bool
```

**Returns:**
False if `name` is already bound in this scope, true on success.

### `scope_lookup`

Looks up `name` across the scope chain, then in the modules imported at
each level (public symbols only).

```luma
pub scope_lookup -> fn(
    scope: *CORE::Scope,
    name: *byte
) *CORE::Symbol
```

### `scope_lookup_in_module`

Looks up `name` directly inside a specific module scope (used for
`ALIAS::name`).

```luma
pub scope_lookup_in_module -> fn(
    module_scope: *CORE::Scope,
    name: *byte
) *CORE::Symbol
```

### `get_enclosing_function_name`

Returns the enclosing function's name, or `""` when not inside a function.

```luma
pub get_enclosing_function_name -> fn(
    scope: *CORE::Scope
) *byte
```

### `analyzer_find`

Finds the tracked allocation for `name` in `func_name`, looking first under
the variable's own name and then under any recorded aliases.

```luma
pub analyzer_find -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte
) *CORE::StaticAllocation
```

### `analyzer_track_alloc`

Records a new allocation for `name` at `line`/`col` in `func_name`, unless
memory tracking is being skipped.

```luma
pub analyzer_track_alloc -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte,
    file_path: *byte,
    line: i64,
    col: i64
) void
```

### `analyzer_track_free`

Marks the matching free of `name`'s allocation, tallying conditional frees
separately from unconditional ones.

```luma
pub analyzer_track_free -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte,
    is_conditional: bool
) void
```

### `analyzer_track_alias`

Records `target_name` as an alias of `source_name`'s allocation.

```luma
pub analyzer_track_alias -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    target_name: *byte,
    source_name: *byte,
    func_name: *byte
) void
```

### `analyzer_mark_address_taken`

Marks the tracked allocation for `name` as having its address taken.

```luma
pub analyzer_mark_address_taken -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte
) void
```

### `analyzer_check_use_after_free`

Records one use-after-free access for `name`; returns true on the first one
so the diagnostic is emitted only once.

```luma
pub analyzer_check_use_after_free -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte
) bool
```

### `find_enum_members`

Returns the member names of `enum_name` as registered in the enclosing
module scope (`Enum.member` symbols).

```luma
pub #returns_ownership
find_enum_members -> fn(
    scope: *CORE::Scope,
    enum_name: *byte
) VEC::Vector
```


## Variables

- **`SCOPE_VEC_CAPACITY`** : i64 *(const)* — Initial capacity for every arena-allocated scope backing vector.
- **`SCOPE_ARENA_SIZE`** : i64 *(const)* — Size of the bump arena backing all Scope/StaticMemoryAnalyzer structs
- **`g_scope_arena`** : ARENA::Arena *(let)* — The bump arena backing all scope structs. Lazily created; see
- **`g_scope_arena_ready`** : bool *(let)* — Whether the scope arena has been initialized.
- **`g_all_scopes`** : VEC::Vector *(let)* — Every scope allocated during the current run, tracked so its backing
- **`g_root_analyzer`** : *CORE::StaticMemoryAnalyzer *(let)* — The root (module) memory analyzer, whose allocations vector is freed once
