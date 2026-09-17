# Module: error

*Source: `src/error/error.lx`*

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `ErrorInformation`

| Field | Type | Description |
|-------|------|-------------|
| `error_type` | *byte |  |
| `file_path` | *byte |  |
| `message` | *byte |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `line_text` | *byte |  |
| `token_length` | i64 |  |
| `label` | *byte |  |
| `note` | *byte |  |
| `help` | *byte |  |


## Functions

### `error_add`

```luma
pub error_add -> fn(
    err: ErrorInformation
) void
```

### `error_add_msg`

```luma
pub #takes_ownership
error_add_msg -> fn(
    message: *byte,
    err: ErrorInformation
) void
```

### `error_clear`

```luma
pub error_clear -> fn(
) void
```

### `error_get_count`

```luma
pub error_get_count -> fn(
) i64
```

### `error_truncate`

```luma
pub error_truncate -> fn(
    n: i64
) void
```

### `error_has_errors`

```luma
pub error_has_errors -> fn(
) i64
```

### `error_get_at_index`

```luma
pub error_get_at_index -> fn(
    index: i64
) *ErrorInformation
```

### `generate_line`

```luma
pub #returns_ownership
generate_line -> fn(
    tokens: *TOK::Token,
    token_count: i64,
    target_line: i64
) *byte
```

### `error_report`

```luma
pub error_report -> fn(
) i64
```

