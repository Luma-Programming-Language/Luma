# Module: tc_error

*Source: `src/typechecker/tc_error.lx`*

Error reporting for the Luma typechecker.

Bridges typechecker source locations to the shared error module: builds
`ErrorInformation` entries with reconstructed source-line text and queues
them for the final report.

## Table of Contents

- [Functions](#functions)
- [Variables](#variables)


## Functions

### `tc_error_init`

Sets the current module's tokens/path so subsequent errors resolve the
right source context.

```luma
pub tc_error_init -> fn(
    mod: *AST::ModuleNode
) void
```

### `generate_line_for_current_module`

Reconstructs the source text of `line` from the current module's tokens.

```luma
pub #returns_ownership
generate_line_for_current_module -> fn(
    line: i64
) *byte
```

### `do_error`

Builds an `ErrorInformation` from a node's location and queues it via
`error_add` for the final report.

```luma
      do_error -> fn(
    node: *AST::AstNode,
    error_type: *byte,
    message: *byte,
    help: *byte,
    token_len: i64,
    label: *byte
) void
```

### `tc_error`

Queues a typechecking error at the node's location with a caret width of
one.


```luma
pub #takes_ownership
tc_error -> fn(
    node: *AST::AstNode,
    error_type: *byte,
    message: *byte,
    label: *byte
) void
```

**Parameters:**
- `node`: erroneous AST node (source of the location)
- `error_type`: headline, e.g. "Type Error"
- `message`: full sentence stashed in the queued ErrorInformation
- `label`: short caption printed after the caret underline

### `tc_error_help`

Like `tc_error`, but carrying an additional `help` hint for the report.

```luma
pub #takes_ownership
tc_error_help -> fn(
    node: *AST::AstNode,
    error_type: *byte,
    help: *byte,
    message: *byte,
    label: *byte
) void
```

### `tc_error_id`

Queues an error whose caret underlines the whole `identifier` token (or a
single char when the identifier is NULL/empty).

```luma
pub #takes_ownership
tc_error_id -> fn(
    node: *AST::AstNode,
    identifier: *byte,
    error_type: *byte,
    message: *byte,
    label: *byte
) void
```

### `tc_error_len`

Like `tc_error`, but with an explicit caret width for spans wider than one
token (e.g. a quoted string literal).

```luma
pub #takes_ownership
tc_error_len -> fn(
    node: *AST::AstNode,
    error_type: *byte,
    message: *byte,
    token_len: i64,
    label: *byte
) void
```


## Variables

- **`g_tokens`** : *TOK::Token *(let)* — Current module's token array, used to reconstruct source lines for errors.
- **`g_token_count`** : i64 *(let)* — Token count for the current module.
- **`g_file_path`** : *byte *(let)* — Path of the current module's source file.
