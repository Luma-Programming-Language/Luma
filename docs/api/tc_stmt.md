# Module: tc_stmt

*Source: `src/typechecker/stmt.lx`*

Statement typechecking and declaration registration.

Dispatches statements to per-form handlers and provides the
`register_declaration` forward-registration pass that tc.lx drives for
every module, including generic templates and their concrete
instantiations.

## Table of Contents

- [Functions](#functions)


## Functions

### `typecheck_statement`

```luma
pub typecheck_statement -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `register_declaration`

```luma
pub register_declaration -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `resolve_os_body`

```luma
pub resolve_os_body -> fn(
    node: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `handle_var_decl`

```luma
      handle_var_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_func_decl`

```luma
      handle_func_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `register_struct_decl`

```luma
      register_struct_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_struct_decl`

```luma
      handle_struct_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `register_enum_decl`

```luma
      register_enum_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_enum_decl`

```luma
      handle_enum_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_if`

```luma
      handle_if -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_loop`

```luma
      handle_loop -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_switch`

```luma
      handle_switch -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_return`

```luma
      handle_return -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_block`

```luma
      handle_block -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_defer`

```luma
      handle_defer -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `typecheck_statement`

Dispatches a statement node to its handler; returns false when the
statement failed to typecheck. `@os` conditionals are handled inline.

```luma
pub typecheck_statement -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `resolve_os_body`

Returns the `@os` arm matching the target OS (or the default body), or
NULL when no arm matches.

```luma
pub resolve_os_body -> fn(
    node: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `register_declaration`

Forward-registers a declaration's symbols without typechecking bodies:
generic templates, functions, structs/enums, typed globals, and `@os`
contents.

```luma
pub register_declaration -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_var_decl`

Typechecks a `let`/`const` declaration: resolves/infers the type, checks
the initializer against the declared type, and registers the symbol.

```luma
      handle_var_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_func_decl`

Typechecks a function declaration: resolves param/return types, registers
the function symbol, and checks the body in a fresh function scope with
parameters as mutable locals.

```luma
      handle_func_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `register_field`

Registers a struct member (field or method) as a `Struct.member` symbol;
generic methods become templates and embedded members are skipped.

```luma
      register_field -> fn(
    struct_name: *byte,
    member: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `register_struct_decl`

Registers a struct's type symbol, its fields/methods, and its nominal
declaration; generic structs are registered as templates instead.

```luma
      register_struct_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_struct_decl`

Body typecheck of a struct declaration: re-registers the struct and checks
each method body in a method scope (`self` for instance methods).

```luma
      handle_struct_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `register_enum_decl`

Registers an enum's type symbol, its `Enum.Member` symbols, and its
nominal declaration; generic enums are registered as templates instead.

```luma
      register_enum_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_enum_decl`

Body typecheck of an enum declaration: generic templates are skipped;
concrete enums re-register their members.

```luma
      handle_enum_decl -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `check_condition`

Typechecks a condition expression, erroring when it is neither boolean
nor numeric.

```luma
      check_condition -> fn(
    cond: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `handle_if`

Typechecks an if / elif / else chain, each branch in its own child scope.

```luma
      handle_if -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_loop`

Typechecks a loop: initializers, condition, optional step, and body all in
the (shared) loop scope.

```luma
      handle_loop -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_switch`

Typechecks a switch: enum case values are deduplicated, duplicate cases
are errors, and non-exhaustive switches without a default are reported.

```luma
      handle_switch -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_return`

Typechecks a `return` against the enclosing function's return type.

```luma
      handle_return -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_block`

Typechecks a block's statements in a child scope.

```luma
      handle_block -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `handle_defer`

Typechecks a `defer` statement with memory tracking suppressed, so the
deferred action isn't double-analyzed as a free inside the function.

```luma
      handle_defer -> fn(
    stmt: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

