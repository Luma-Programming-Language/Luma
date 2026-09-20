# Module: tc

*Source: `src/typechecker/tc.lx`*

Driver for the Luma typechecker.

Runs every module through scope setup, `@use` resolution,
forward-registration, and body typechecking; reports per-module leak /
double-free diagnostics, then drains queued generic instantiations.

## Table of Contents

- [Functions](#functions)


## Functions

### `find_module_scope`

Returns the module scope registered under `name`, or NULL.

```luma
      find_module_scope -> fn(
    registry: *VEC::Vector,
    name: *byte
) *CORE::Scope
```

### `resolve_uses`

Adds a module import to `ms` for every `@use` directive in the module
body.

```luma
      resolve_uses -> fn(
    mnode: *AST::AstNode,
    ms: *CORE::Scope,
    registry: *VEC::Vector
) void
```

### `check_module_uses`

Reports a Module Error for every `@use` whose target module is missing
from the registry.

```luma
      check_module_uses -> fn(
    mnode: *AST::AstNode,
    registry: *VEC::Vector
) void
```

### `register_module_body`

Forward-registers every declaration in the module body via
`STMT::register_declaration`.

```luma
      register_module_body -> fn(
    mnode: *AST::AstNode,
    ms: *CORE::Scope
) void
```

### `check_module_body`

Typechecks each statement of the module body, applying `@os` conditionals;
returns false if any statement failed.

```luma
      check_module_body -> fn(
    mnode: *AST::AstNode,
    ms: *CORE::Scope
) bool
```

### `report_module_allocations`

Reports double-free and never-freed allocations recorded for `file_path`.

```luma
      report_module_allocations -> fn(
    analyzer: *CORE::StaticMemoryAnalyzer,
    file_path: *byte
) void
```

### `typecheck`

Entry point: typechecks a whole program (or a single stmt node). Registers
per-module scopes, resolves `@use`, forward-registers and body-checks every
module, drains generic instantiations, and reports errors.


```luma
pub typecheck -> fn(
    node: *AST::AstNode,
    config: CONST::LumaBuildConfig
) bool
```

**Returns:**
True when the program typechecked without errors.

