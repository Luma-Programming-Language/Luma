# Module: tc_expr

*Source: `src/typechecker/expr.lx`*

Typechecking of Luma expressions.

Dispatches every expression node form to a per-kind typechecker, resolving
each to a Luma type, and drives the static memory analyzer (use-after-free
and leak tracking) for pointer-producing expressions.

## Table of Contents

- [Functions](#functions)


## Functions

### `typecheck_expression`

```luma
pub #returns_ownership
typecheck_expression -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `typecheck_expression_inner`

```luma
      #returns_ownership
typecheck_expression_inner -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `tc_literal`

```luma
      #returns_ownership
tc_literal -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_identifier`

```luma
      #returns_ownership
tc_identifier -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_binary`

```luma
      #returns_ownership
tc_binary -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_unary`

```luma
      #returns_ownership
tc_unary -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_call`

```luma
      #returns_ownership
tc_call -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_assign`

```luma
      #returns_ownership
tc_assign -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_ternary`

```luma
      #returns_ownership
tc_ternary -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_member`

```luma
      #returns_ownership
tc_member -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_index`

```luma
      #returns_ownership
tc_index -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_array`

```luma
      #returns_ownership
tc_array -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `tc_cast`

```luma
      #returns_ownership
tc_cast -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_deref`

```luma
      #returns_ownership
tc_deref -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_addr`

```luma
      #returns_ownership
tc_addr -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_struct_expr`

```luma
      #returns_ownership
tc_struct_expr -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `check_uaf`

```luma
      check_uaf -> fn(
    node: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `track_free`

```luma
      track_free -> fn(
    ptr_expr: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `call_is_returns_ownership`

```luma
      call_is_returns_ownership -> fn(
    call_expr: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `track_pointer_source`

```luma
pub track_pointer_source -> fn(
    name: *byte,
    value_expr: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `qualify`

Returns a heap-allocated `a.b` string splicing two identifiers.

```luma
pub #returns_ownership
qualify -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `dup_str`

Returns a heap-allocated NUL-terminated copy of `s`.

```luma
      #returns_ownership
dup_str -> fn(
    s: *byte
) *byte
```

### `strip_pointer`

Unwraps a pointer type to its pointee type; non-pointers pass through.

```luma
      strip_pointer -> fn(
    t: *AST::AstNode
) *AST::AstNode
```

### `check_uaf`

Reports a use-after-free error if `node` is an identifier freed earlier
in the enclosing function.

```luma
      check_uaf -> fn(
    node: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `track_free`

Records an explicit free of an identifier on the enclosing function's
static memory analyzer (deferred to the function's deferred-frees list
while tracking is skipped).

```luma
      track_free -> fn(
    ptr_expr: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `call_is_returns_ownership`

Returns whether the callee symbol of a call is marked `#returns_ownership`.

```luma
      call_is_returns_ownership -> fn(
    call_expr: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `call_is_takes_ownership`

Returns whether a callee (identifier or module-qualified member) names a
symbol marked `#takes_ownership`.

```luma
      call_is_takes_ownership -> fn(
    callee: *AST::AstNode,
    scope: *CORE::Scope
) bool
```

### `track_pointer_source`

Tracks where a pointer binding gets its value — an `alloc`, a
`#returns_ownership` call, or an alias of another tracked variable — so
the leak checker can account for it.

```luma
pub track_pointer_source -> fn(
    name: *byte,
    value_expr: *AST::AstNode,
    scope: *CORE::Scope
) void
```

### `typecheck_expression`

Public entry point: typechecks an expression and stashes the resolved
type on the node (`AstNode.resolved_type`) so codegen can read a static
type straight off the AST without re-running inference.

```luma
pub #returns_ownership
typecheck_expression -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `typecheck_expression_inner`

Dispatches an expression to its per-kind typechecker; grouping, system,
alloc/free, sizeof, input, syscall, and memcpy forms are handled here.

```luma
      #returns_ownership
typecheck_expression_inner -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `tc_literal`

Typechecks a literal, mapping its literal kind to the corresponding type.

```luma
      #returns_ownership
tc_literal -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_identifier`

Looks up an identifier in scope, reports undeclared identifiers, runs the
use-after-free check, and returns the symbol's type.

```luma
      #returns_ownership
tc_identifier -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_binary`

Typechecks a binary or range expression: comparisons and logic yield
bool, bitwise yields i64, arithmetic follows numeric rank, and pointer
arithmetic returns the pointer type.

```luma
      #returns_ownership
tc_binary -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_unary`

Typechecks a unary expression: logical `!` yields bool, otherwise the
operand's type.

```luma
      #returns_ownership
tc_unary -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_deref`

Typechecks `*ptr`, returning the pointee type (error on non-pointers and
use-after-free).

```luma
      #returns_ownership
tc_deref -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_addr`

Typechecks `&x`, returning a pointer to the operand's type and marking
the identifier's address taken for the analyzer.

```luma
      #returns_ownership
tc_addr -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_index`

Typechecks `a[i]`: the index must be an integer, and the result is the
array's element type or the pointer's pointee type.

```luma
      #returns_ownership
tc_index -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_member`

Typechecks member access: compiletime `Enum::Member` / `Alias::symbol`
(including module-alias and generic enum members) and runtime `obj.field`
access on nominal types.

```luma
      #returns_ownership
tc_member -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `resolve_callee_type`

Resolves a callee to its function type; the `output()`/`outputln()`
builtins (variadic) are exempt and yield NULL.

```luma
      #returns_ownership
resolve_callee_type -> fn(
    callee: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_call`

Typechecks a call: builtin output, explicit generic instantiations,
argument count/types, `#takes_ownership` tracking, and error reporting for
non-callable or undeclared callees.

```luma
      #returns_ownership
tc_call -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_assign`

Typechecks an assignment: mutability, type match, pointer-source tracking,
and escape handling for member targets.

```luma
      #returns_ownership
tc_assign -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_ternary`

Typechecks `c ? a : b`, returning the then-branch type (or the else-branch
type if the then-branch didn't resolve).

```luma
      #returns_ownership
tc_ternary -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_cast`

Typechecks a cast by resolving the target type and rejecting casts between
incompatible types.

```luma
      #returns_ownership
tc_cast -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

### `tc_array`

Typechecks an array literal: the element type comes from the first element
(or the expected type), the rest must match, and empty literals defer to
the expected type.

```luma
      #returns_ownership
tc_array -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

### `struct_decl_has_field`

Returns whether a struct (directly or via `...Embedded` promotion) accepts
`field` in a literal; unknown decls stay permissive.

```luma
      struct_decl_has_field -> fn(
    scope: *CORE::Scope,
    sname: *byte,
    field: *byte
) bool
```

### `validate_struct_literal_fields`

Reports a Type Error for the first unknown field name in a struct literal.

```luma
      validate_struct_literal_fields -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    sname: *byte
) void
```

### `tc_struct_expr`

Typechecks a struct literal: validates field names, instantiates generic
structs (`Box<i64> { ... }`, including through a module alias), resolves
bare vs. aliased names, and returns the struct's type.

```luma
      #returns_ownership
tc_struct_expr -> fn(
    expr: *AST::AstNode,
    scope: *CORE::Scope,
    expected: *AST::AstNode
) *AST::AstNode
```

