# Module: cg_stmt

*Source: `src/codegen/codegen_stmt.lx`*

Statement codegen for the C backend.

Dispatches a statement AST node and writes the corresponding C statements
into the output buffer: declarations, control flow, blocks, and defer
bookkeeping.

## Table of Contents

- [Functions](#functions)


## Functions

### `gen_stmt`

```luma
pub gen_stmt -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

### `var_decl_type`

The declared type of a variable declaration, or the initializer's
resolved_type when no type was written (the only place codegen can read
the inferred type).

```luma
      var_decl_type -> fn(
    ctx: *CG::CodegenContext,
    n: *AST::VarDeclNode
) *AST::AstNode
```

### `gen_local_var_decl`

Emits a local `let`/`const` declaration: the initializer is generated in
the *previous* binding's scope first (so `let x = x + 1` reads the old
value), then the declarator uses the newly registered C name.

```luma
      gen_local_var_decl -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

### `push_defer`

Records a `defer` statement's target on the context's defer stack.

```luma
      push_defer -> fn(
    ctx: *CG::CodegenContext,
    stmt: *AST::AstNode
) void
```

### `defer_at`

Returns the deferred statement at index `i` of the context's defer stack.

```luma
      defer_at -> fn(
    ctx: *CG::CodegenContext,
    i: i64
) *AST::AstNode
```

### `flush_defers_from`

Emits every deferred statement from the top of the stack down through
index `from_len` (LIFO order).

```luma
      flush_defers_from -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    from_len: i64
) void
```

### `gen_return`

Emits a `return`: plain when no defers are pending; otherwise the value is
stashed in `__retval`, deferred statements are flushed, and the return
uses the stashed value.

```luma
      gen_return -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

### `gen_block`

Emits a block as `{ ... }`, flushing and popping any defers pushed inside
it and truncating the local stack so names don't escape the block.

```luma
      gen_block -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

### `gen_if`

Emits an `if`/`elif`/`else` chain, flattening elifs into `else if`
branches rather than re-dispatching nested IfStmtNodes.

```luma
      gen_if -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

### `gen_loop`

Emits a `loop` as a C `for`: init declarations, condition (defaulting to
constant 1 when absent), optional post expression, then the body. Init
vars stay scoped to the whole for-statement.

```luma
      gen_loop -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

### `gen_switch`

Emits a `switch`: each case's values as `case` labels with a trailing
`break;`, plus the `default` arm if present.

```luma
      gen_switch -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

### `gen_stmt`

Top-level statement dispatcher: routes a statement node by kind to the
per-node generators. Nested struct/function/enum declarations are no-ops.

```luma
pub gen_stmt -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    stmt: *AST::AstNode
) void
```

