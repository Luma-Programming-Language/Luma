# Module: lexer

*Source: `src/lexer/lexer.lx`*

Scans raw source text into a stream of tokens: whitespace and comments
are skipped or emitted (for `//!` / `///` doc comments) while literals,
operators and directive keywords become tokens.

## Table of Contents

- [Functions](#functions)


## Functions

### `is_alpha`

Returns true if `c` is an ASCII letter or underscore.

```luma
      is_alpha -> fn(
    c: byte
) bool
```

### `is_digit`

Returns true if `c` is an ASCII digit.

```luma
      is_digit -> fn(
    c: byte
) bool
```

### `is_alnum`

Returns true if `c` is alphanumeric (letter, digit or underscore).

```luma
      is_alnum -> fn(
    c: byte
) bool
```

### `is_hex_digit`

Returns true if `c` is a hexadecimal digit (0-9, a-f, A-F).

```luma
      is_hex_digit -> fn(
    c: byte
) bool
```

### `is_bin_digit`

Returns true if `c` is a binary digit (0 or 1).

```luma
      is_bin_digit -> fn(
    c: byte
) bool
```

### `is_oct_digit`

Returns true if `c` is an octal digit (0-7).

```luma
      is_oct_digit -> fn(
    c: byte
) bool
```

### `skip_line_comment`

Advances past a `//` line comment up to (but not including) the newline.

```luma
      skip_line_comment -> fn(
    lx: *sym::Lexer
) void
```

### `skip_block_comment`

Advances past a `/* ... */` block comment (nesting is not supported).

```luma
      skip_block_comment -> fn(
    lx: *sym::Lexer
) void
```

### `skip_whitespace`

Skips whitespace and comments, returning the count of consecutive
whitespace bytes before the last newline (recorded as the token's
`whitespace_len`). Doc comments stop the scan so they can become tokens.

```luma
      skip_whitespace -> fn(
    lx: *sym::Lexer
) i64
```

### `scan_doc_comment`

Scans the text after a `//!` or `///` opener to end of line and emits it
as a TOK_MODULE_DOC or TOK_DOC_COMMENT token.

```luma
      scan_doc_comment -> fn(
    lx: *sym::Lexer,
    kind: i64,
    start: *byte,
    line: i64,
    col: i64,
    ws: i64
) sym::Token
```

### `scan_prefixed_word`

Scans an `@word` / `#word` directive whose prefix was already consumed.
Known words resolve to their directive/attribute token via the keyword
lookup; unknown ones are recorded as an error and returned as TOK_ERROR.

```luma
      scan_prefixed_word -> fn(
    lx: *sym::Lexer,
    start: *byte,
    line: i64,
    col: i64,
    ws: i64
) sym::Token
```

### `scan_number`

Consumes a numeric literal and returns TOK_NUMBER or TOK_NUM_FLOAT,
handling decimal/hex/binary/octal forms, `_` digit separators, fractional
parts and optional type suffixes.

```luma
      scan_number -> fn(
    lx: *sym::Lexer
) i64
```

### `lexer_add_error`

Records a lexical error in the lexer's fixed-size error buffer (up to
MAX_LEX_ERRORS); stashed errors are re-issued as diagnostics later.

```luma
      lexer_add_error -> fn(
    lx: *sym::Lexer,
    kind: i64,
    line: i64,
    col: i64,
    len: i64,
    bad: i64
) void
```

### `next_token`

Scans and returns the next token, skipping whitespace and comments;
characters the language has no token for become recorded error tokens
rather than being silently dropped.

```luma
pub next_token -> fn(
    lx: *sym::Lexer
) sym::Token
```

