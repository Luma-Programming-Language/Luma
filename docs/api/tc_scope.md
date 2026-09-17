# Module: tc_scope

*Source: `src/typechecker/scope.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `scope_arena_init`

```luma
pub scope_arena_init -> fn(
) void
```

### `scope_arena_teardown`

```luma
pub scope_arena_teardown -> fn(
) void
```

### `scope_init`

```luma
pub #returns_ownership
scope_init -> fn(
    name: *byte,
    check_mem: bool,
    os: *byte
) *CORE::Scope
```

### `scope_create_child`

```luma
pub #returns_ownership
scope_create_child -> fn(
    parent: *CORE::Scope,
    name: *byte
) *CORE::Scope
```

### `find_containing_module`

```luma
pub find_containing_module -> fn(
    scope: *CORE::Scope
) *CORE::Scope
```

### `get_enclosing_function`

```luma
pub get_enclosing_function -> fn(
    scope: *CORE::Scope
) *CORE::Scope
```

### `scope_is_conditional`

```luma
pub scope_is_conditional -> fn(
    scope: *CORE::Scope,
    fscope: *CORE::Scope
) bool
```

### `add_module_import`

```luma
pub add_module_import -> fn(
    scope: *CORE::Scope,
    module_name: *byte,
    alias: *byte,
    module_scope: *CORE::Scope
) void
```

### `find_module_import`

```luma
pub find_module_import -> fn(
    scope: *CORE::Scope,
    alias: *byte
) *CORE::ModuleImport
```

### `scope_lookup_current`

```luma
pub scope_lookup_current -> fn(
    scope: *CORE::Scope,
    name: *byte
) *CORE::Symbol
```

### `scope_add_symbol_full`

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

### `scope_add_symbol`

```luma
pub scope_add_symbol -> fn(
    scope: *CORE::Scope,
    name: *byte,
    type: *AST::AstNode,
    is_public: bool,
    is_mutable: bool
) bool
```

### `scope_lookup`

```luma
pub scope_lookup -> fn(
    scope: *CORE::Scope,
    name: *byte
) *CORE::Symbol
```

### `scope_lookup_in_module`

```luma
pub scope_lookup_in_module -> fn(
    module_scope: *CORE::Scope,
    name: *byte
) *CORE::Symbol
```

### `get_enclosing_function_name`

```luma
pub get_enclosing_function_name -> fn(
    scope: *CORE::Scope
) *byte
```

### `analyzer_find`

```luma
pub analyzer_find -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte
) *CORE::StaticAllocation
```

### `analyzer_track_alloc`

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

```luma
pub analyzer_track_free -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte,
    is_conditional: bool
) void
```

### `analyzer_track_alias`

```luma
pub analyzer_track_alias -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    target_name: *byte,
    source_name: *byte,
    func_name: *byte
) void
```

### `analyzer_mark_address_taken`

```luma
pub analyzer_mark_address_taken -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte
) void
```

### `analyzer_check_use_after_free`

```luma
pub analyzer_check_use_after_free -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    name: *byte,
    func_name: *byte
) bool
```

### `find_enum_members`

```luma
pub #returns_ownership
find_enum_members -> fn(
    scope: *CORE::Scope,
    enum_name: *byte
) VEC::Vector
```

