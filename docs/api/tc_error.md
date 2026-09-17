# Module: tc_error

*Source: `src/typechecker/tc_error.lx`*

## Table of Contents

- [Functions](#functions)
- [Variables](#variables)


## Functions

### `tc_error_init`

```luma
pub tc_error_init -> fn(
    mod: *AST::ModuleNode
) void
```

### `generate_line_for_current_module`

```luma
pub #returns_ownership
generate_line_for_current_module -> fn(
    line: i64
) *byte
```

### `tc_error`

```luma
pub #takes_ownership
tc_error -> fn(
    node: *AST::AstNode,
    error_type: *byte,
    message: *byte,
    label: *byte
) void
```

### `tc_error_help`

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

- **`g_tokens`** : *TOK::Token *(let)*
- **`g_token_count`** : i64 *(let)*
- **`g_file_path`** : *byte *(let)*
