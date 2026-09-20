# Module: tc_generics

*Source: `src/typechecker/generics.lx`*

Monomorphization for `fn<T>`/`struct<T>`/`enum<T>` (see docs/docs.md's
Generics section). An instantiation is just an ordinary, concrete
FuncDeclNode / StructDeclNode / EnumDeclNode
with a mangled name and every type-parameter occurrence substituted for
the caller's concrete type — synthesized once per unique
(module, template name, concrete types) combination, then run through
the same register_declaration -> typecheck_statement path any ordinary
declaration already uses. Nothing in codegen needs to know generics
exist; see tc_core's generated-decl registry, which codegen's Pass A
reads in addition to each module's normal body.

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `GenericTemplate`

One entry per `fn<T,...>` / `struct<T,...>` / `enum<T,...>` template.

Holds the raw, unresolved declaration and the scope it was declared in
(not the caller's scope), so instantiations are created there. Method
templates carry their owning struct and static flag.


| Field | Type | Description |
|-------|------|-------------|
| `module_name` | *byte |  |
| `name` | *byte |  |
| `decl` | *AST::AstNode |  |
| `def_scope` | *CORE::Scope |  |
| `owner` | *byte |  |
| `method_is_static` | i64 |  |

### `Instantiation`

One cached concrete instantiation per unique (module, name, types)
combination.


| Field | Type | Description |
|-------|------|-------------|
| `key` | *byte |  |
| `ftype` | *AST::AstNode |  |
| `mangled_name` | *byte |  |

### `Pending`

One queued concrete instantiation awaiting registration + typechecking by
tc.lx's drain.


| Field | Type | Description |
|-------|------|-------------|
| `decl` | *AST::AstNode |  |
| `def_scope` | *CORE::Scope |  |
| `module_name` | *byte |  |
| `owner` | *byte |  |
| `is_static` | i64 |  |


## Functions

### `dup_str`

Returns a heap-allocated NUL-terminated copy of `s`.

```luma
      #returns_ownership
dup_str -> fn(
    s: *byte
) *byte
```

### `join2`

Returns a heap-allocated string joining `a` and `b` with `__` between them.

```luma
      #returns_ownership
join2 -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `qualify_member`

Returns a heap-allocated `a.b` string splicing two identifiers.

Local copy of `EXPR::qualify`: tc_generics can't import tc_expr (tc_expr
imports tc_generics), so the splice is duplicated here.

```luma
      #returns_ownership
qualify_member -> fn(
    a: *byte,
    b: *byte
) *byte
```

### `sanitize`

Returns a heap-allocated copy of `s` with every char outside
`[A-Za-z0-9_]` replaced by `_`, so it forms a valid C identifier fragment.

```luma
      #returns_ownership
sanitize -> fn(
    s: *byte
) *byte
```

### `build_mangled_name`

Mangles a generic's base name by appending each concrete type as a
sanitized `__<type>` segment; returns a heap-allocated string.

```luma
      #returns_ownership
build_mangled_name -> fn(
    base_name: *byte,
    concretes: **AST::AstNode,
    count: i64
) *byte
```

### `templates_init`

Initializes `g_templates` on first use (idempotent).

```luma
      templates_init -> fn(
) void
```

### `register_template`

Registers a module-level `fn<T>` / `struct<T>` / `enum<T>` template under
its defining module and name.

```luma
pub register_template -> fn(
    module_name: *byte,
    name: *byte,
    decl: *AST::AstNode,
    def_scope: *CORE::Scope
) void
```

### `register_method_template`

Registers a generic method template (`Thing::make<T>` on a non-generic
struct). Keyed under the qualified name `Thing.make` so it can't collide
with module-level templates; the owner + static flag ride along for codegen.

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

Looks up a template by defining module and name; returns NULL when none
is registered.

```luma
pub find_template -> fn(
    module_name: *byte,
    name: *byte
) *GenericTemplate
```

### `insts_init`

Initializes `g_insts` on first use (idempotent).

```luma
      insts_init -> fn(
) void
```

### `find_instantiation`

Returns the cached concrete instantiation for `key`, or NULL.

```luma
      find_instantiation -> fn(
    key: *byte
) *Instantiation
```

### `add_instantiation`

Adds a concrete instantiation to the cache under `key` (key ownership
passes to the entry).

```luma
      add_instantiation -> fn(
    key: *byte,
    ftype: *AST::AstNode,
    mangled_name: *byte
) void
```

### `pending_init`

Initializes `g_pending` on first use (idempotent).

```luma
      pending_init -> fn(
) void
```

### `pending_push`

Queues a concrete instantiation for the drain (see module doc comment).

```luma
      pending_push -> fn(
    decl: *AST::AstNode,
    def_scope: *CORE::Scope,
    module_name: *byte,
    owner: *byte,
    is_static: i64
) void
```

### `pending_count`

Number of concretes still awaiting the drain.

```luma
pub pending_count -> fn(
) i64
```

### `pending_pop`

Removes and returns the last queued instantiation (order is irrelevant).

```luma
pub pending_pop -> fn(
) Pending
```

### `rename_callee`

Rewrites a call's callee to a new name, preserving identifier vs. member
shape so codegen still resolves module-qualified calls.

```luma
      rename_callee -> fn(
    call_node: *AST::CallNode,
    new_name: *byte
) void
```

### `instantiate_function_call`

Typechecks a generic function call: resolves the explicit type arguments,
returns the concrete FuncType to check the args/result against (or NULL
with an error already reported), rewrites the callee to the mangled name,
and queues the concrete body for tc.lx's drain.


```luma
pub #returns_ownership
instantiate_function_call -> fn(
    call_node: *AST::CallNode,
    tmpl: *GenericTemplate,
    caller_scope: *CORE::Scope
) *AST::AstNode
```

**Parameters:**
- `call_node`: the call site; callee is rewritten to the mangled name on success
- `tmpl`: the matched template
- `caller_scope`: scope the call is typechecked in

**Returns:**
Concrete function type, or NULL after reporting an error.

### `instantiate_method_call`

Method-template counterpart of `instantiate_function_call`, for
`Thing::make<T>(...)` on a non-generic struct. The owner struct name +
static flag ride through the pending queue so codegen registers the
concrete method with the right ownership.


```luma
pub #returns_ownership
instantiate_method_call -> fn(
    call_node: *AST::CallNode,
    owner: *byte,
    tmpl: *GenericTemplate,
    caller_scope: *CORE::Scope
) *AST::AstNode
```

**Parameters:**
- `call_node`: the call site; callee is rewritten to the mangled name on success
- `owner`: owning struct name (e.g. `Thing` in `Thing::make<T>`)
- `tmpl`: the matched method template
- `caller_scope`: scope the call is typechecked in

**Returns:**
Concrete function type, or NULL after reporting an error.

### `decl_tp_count`

Returns the type-parameter count of a struct/enum decl (0 otherwise).

```luma
      decl_tp_count -> fn(
    decl: *AST::AstNode
) i64
```

### `decl_tp_names`

Returns the type-parameter name array of a struct/enum decl (NULL
otherwise).

```luma
      decl_tp_names -> fn(
    decl: *AST::AstNode
) **byte
```

### `decl_base_name`

Returns the base (unmangled) name of a struct/enum template.

```luma
      decl_base_name -> fn(
    decl: *AST::AstNode
) *byte
```

### `degenericize_decl`

Frees and zeroes the `type_params`/`type_param_count` pair on a cloned
struct/enum decl, turning it into an ordinary concrete decl.

```luma
      degenericize_decl -> fn(
    cloned: *AST::AstNode
) void
```

### `strip_method_type_params`

Strips cloned-away type params from methods of a generic struct template
so the pending drain typechecks them as ordinary concrete methods.

```luma
      strip_method_type_params -> fn(
    members: **AST::AstNode,
    count: i64
) void
```

### `scope_module_name`

Returns the name of the innermost enclosing module scope, or NULL.

```luma
      #returns_ownership
scope_module_name -> fn(
    scope: *CORE::Scope
) *byte
```

### `resolve_nominal_type_ref`

Resolves a same-module `Box<i64>` type leaf (struct/enum instantiation):
rewrites it in place to the mangled concrete name and queues the concrete
decl for tc.lx's drain. Returns the rewritten leaf.


```luma
pub #returns_ownership
resolve_nominal_type_ref -> fn(
    leaf: *AST::AstNode,
    scope: *CORE::Scope
) *AST::AstNode
```

**Parameters:**
- `leaf`: TYPE_BASIC node carrying the concrete type arguments
- `scope`: scope to resolve the type arguments in

**Returns:**
The resolved leaf (rewritten in place), or errors at the leaf on failure.

### `resolve_nominal_type_ref_in`

`resolve_nominal_type_ref` for an explicitly named defining module, so a
qualified `G::Box<i64>` reference resolves the template in *G*'s registry
instead of the caller's own.

```luma
pub #returns_ownership
resolve_nominal_type_ref_in -> fn(
    leaf: *AST::AstNode,
    scope: *CORE::Scope,
    def_module_name: *byte
) *AST::AstNode
```

### `struct_template_has_method`

Checks whether a struct template declares a method (public or private)
with the given name — used to report missing static methods before the
call falls through to codegen.

```luma
pub struct_template_has_method -> fn(
    sd: *AST::StructDeclNode,
    method_name: *byte
) bool
```


## Variables

- **`g_templates`** : VEC::Vector *(let)* — Registry of all registered generic templates.
- **`g_templates_ready`** : bool *(let)* — Lazily-initialized flag for `g_templates`.
- **`g_insts`** : VEC::Vector *(let)* — Cache of concrete instantiation entries.
- **`g_insts_ready`** : bool *(let)* — Lazily-initialized flag for `g_insts`.
- **`g_pending`** : VEC::Vector *(let)* — Queue of pending concrete instantiations awaiting the drain.
- **`g_pending_ready`** : bool *(let)* — Lazily-initialized flag for `g_pending`.
