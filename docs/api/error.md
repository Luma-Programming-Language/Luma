# Module: error

*Source: `src/error/error.lx`*

Compiler error reporting.

Collects diagnostics into a bounded list and renders them with source
excerpts, line numbers, and caret indicators before the compiler aborts.

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `ErrorInformation`

Describes one compiler error with its location, source line, and notes.


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

### `c_reset`

Emits the ANSI reset escape sequence.

```luma
      c_reset -> fn(
) void
```

### `c_bold_red`

Emits the bold-red escape sequence (error labels).

```luma
      c_bold_red -> fn(
) void
```

### `c_bold_white`

Emits the bold-white escape sequence (messages).

```luma
      c_bold_white -> fn(
) void
```

### `c_bold_blue`

Emits the bold-blue escape sequence (file locations).

```luma
      c_bold_blue -> fn(
) void
```

### `c_bold_yellow`

Emits the bold-yellow escape sequence (caret indicators).

```luma
      c_bold_yellow -> fn(
) void
```

### `c_bold_cyan`

Emits the bold-cyan escape sequence (notes).

```luma
      c_bold_cyan -> fn(
) void
```

### `c_bold_green`

Emits the bold-green escape sequence (help text).

```luma
      c_bold_green -> fn(
) void
```

### `c_blue`

Emits the plain blue escape sequence.

```luma
      c_blue -> fn(
) void
```

### `c_gray`

Emits the gray (bright black) escape sequence.

```luma
      c_gray -> fn(
) void
```

### `error_add`

Queues an error if the list is not full.

```luma
pub error_add -> fn(
    err: ErrorInformation
) void
```

### `error_add_msg`

Queues an error, marking `message` as consumed so the static analyzer
does not report a owned message as leaked.

```luma
pub #takes_ownership
error_add_msg -> fn(
    message: *byte,
    err: ErrorInformation
) void
```

### `error_clear`

Clears all queued errors.

```luma
pub error_clear -> fn(
) void
```

### `error_get_count`

Returns the number of queued errors.

```luma
pub error_get_count -> fn(
) i64
```

### `error_truncate`

Reduces the queued error count to at most `n`.

```luma
pub error_truncate -> fn(
    n: i64
) void
```

### `error_has_errors`

Returns 1 if any errors are queued, else 0.

```luma
pub error_has_errors -> fn(
) i64
```

### `error_get_at_index`

Returns a pointer to the queued error at `index`, or NULL when the index
is out of bounds.

```luma
pub error_get_at_index -> fn(
    index: i64
) *ErrorInformation
```

### `generate_line`

Reconstructs the source text of `target_line` from its tokens.

Restores inter-token whitespace, wraps string tokens in quotes, and
re-emits `///` / `//!` doc-comment prefixes so the line can be shown
verbatim in diagnostics. Returns an allocated string (an allocated empty
string when the line is out of range or `tokens` is NULL).

```luma
pub #returns_ownership
generate_line -> fn(
    tokens: *TOK::Token,
    token_count: i64,
    target_line: i64
) *byte
```

### `get_line_width`

Returns the number of digits in `line` (used for gutter alignment).

```luma
      get_line_width -> fn(
    line: i64
) i64
```

### `get_max_line_width`

Returns the widest line number among the queued errors.

```luma
      get_max_line_width -> fn(
) i64
```

### `print_line_padding`

Prints spaces so `current_line` aligns to `max_width`.

```luma
      print_line_padding -> fn(
    current_line: i64,
    max_width: i64
) void
```

### `print_gutter`

Prints the blank gutter with its `|` separator.

```luma
      print_gutter -> fn(
    max_width: i64
) void
```

### `print_source_line`

Prints one source line prefixed by its line number in the gutter.

```luma
      print_source_line -> fn(
    line: i64,
    text: *byte,
    max_width: i64
) void
```

### `print_indicator`

Prints the `^` caret indicator under a source position, plus its label.

```luma
      print_indicator -> fn(
    col: i64,
    length: i64,
    max_width: i64,
    label: *byte
) void
```

### `error_report`

Prints all queued errors, each with location, source excerpt, and any
note/help lines.


```luma
pub error_report -> fn(
) i64
```

**Returns:**
`1` if errors were printed, `0` when the queue was empty.


## Variables

- **`MAX_ERRORS`** : i64 *(const)* — Maximum number of errors kept before the list silently stops growing.
- **`error_list`** : [ErrorInformation; 256] *(let)* — Fixed-size storage for queued errors.
- **`error_count`** : i64 *(let)* — Number of errors currently queued in `error_list`.
