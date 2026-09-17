# Module: lexer_tokens

*Source: `src/lexer/tokens.lx`*

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `LexError`

| Field | Type | Description |
|-------|------|-------------|
| `kind` | i64 |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `len` | i64 |  |
| `bad` | byte |  |

### `Lexer`

| Field | Type | Description |
|-------|------|-------------|
| `current` | *byte |  |
| `src` | *byte |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `errors` | [LexError; 32] |  |
| `error_count` | i64 |  |
| `had_error` | i64 |  |

**Methods:**

#### `Lexer::make()`

```luma
static Lexer::make -> fn(
    src: *byte
) Lexer
```

#### `Lexer::is_end()`

```luma
Lexer::is_end -> fn(
) i64
```

#### `Lexer::peek()`

```luma
Lexer::peek -> fn(
    offset: i64
) byte
```

#### `Lexer::advance()`

```luma
Lexer::advance -> fn(
) byte
```

### `Token`

| Field | Type | Description |
|-------|------|-------------|
| `val` | *byte |  |
| `kind` | i64 |  |
| `len` | i64 |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `whitespace_len` | i64 |  |

**Methods:**

#### `Token::make()`

```luma
static Token::make -> fn(
    val: *byte,
    kind: i64,
    len: i64,
    line: i64,
    col: i64,
    whitespace_len: i64
) Token
```

#### `Token::token_value_string()`

```luma
#returns_ownership
Token::token_value_string -> fn(
) *byte
```


## Enumerations

### pub `LumaTokenType`

**Values:**

- `TOK_EOF`
- `TOK_ERROR`
- `TOK_IDENTIFIER`
- `TOK_KEYWORD`
- `TOK_NUMBER`
- `TOK_NUM_FLOAT`
- `TOK_STRING`
- `TOK_CHAR_LITERAL`
- `TOK_I8`
- `TOK_I16`
- `TOK_I32`
- `TOK_I64`
- `TOK_U8`
- `TOK_U16`
- `TOK_U32`
- `TOK_U64`
- `TOK_F32`
- `TOK_F64`
- `TOK_BOOL`
- `TOK_VOID`
- `TOK_CHAR`
- `TOK_IF`
- `TOK_ELIF`
- `TOK_ELSE`
- `TOK_LOOP`
- `TOK_RETURN`
- `TOK_BREAK`
- `TOK_CONTINUE`
- `TOK_STRUCT`
- `TOK_ENUM`
- `TOK_MOD`
- `TOK_IMPORT`
- `TOK_TRUE`
- `TOK_FALSE`
- `TOK_PUBLIC`
- `TOK_PRIVATE`
- `TOK_VAR`
- `TOK_CONST`
- `TOK_FN`
- `TOK_INPUT`
- `TOK_ALLOC`
- `TOK_FREE`
- `TOK_CAST`
- `TOK_SIZE_OF`
- `TOK_AS`
- `TOK_DEFER`
- `TOK_IN`
- `TOK_SWITCH`
- `TOK_USING`
- `TOK_STATIC`
- `TOK_SYSTEM`
- `TOK_IMPL`
- `TOK_SYSCALL`
- `TOK_MODULE`
- `TOK_USE`
- `TOK_OS`
- `TOK_LINK`
- `TOK_RETURNS_OWNERSHIP`
- `TOK_TAKES_OWNERSHIP`
- `TOK_DLL_IMPORT`
- `TOK_LIB_IMPORT`
- `TOK_SYMBOL`
- `TOK_LPAREN`
- `TOK_RPAREN`
- `TOK_LBRACE`
- `TOK_RBRACE`
- `TOK_LBRACKET`
- `TOK_RBRACKET`
- `TOK_SEMI`
- `TOK_COMMA`
- `TOK_DOT`
- `TOK_AT`
- `TOK_EQUAL`
- `TOK_PLUS`
- `TOK_MINUS`
- `TOK_STAR`
- `TOK_SLASH`
- `TOK_LT`
- `TOK_GT`
- `TOK_LE`
- `TOK_GE`
- `TOK_EQEQ`
- `TOK_NEQ`
- `TOK_AMP`
- `TOK_PIPE`
- `TOK_CARET`
- `TOK_TILDE`
- `TOK_AND`
- `TOK_OR`
- `TOK_RESOLVE`
- `TOK_COLON`
- `TOK_BANG`
- `TOK_QUESTION`
- `TOK_PLUSPLUS`
- `TOK_MINUSMINUS`
- `TOK_SHIFT_LEFT`
- `TOK_SHIFT_RIGHT`
- `TOK_RANGE`
- `TOK_ELLIPSIS`
- `TOK_RIGHT_ARROW`
- `TOK_LEFT_ARROW`
- `TOK_MODL`
- `TOK_WHITESPACE`
- `TOK_COMMENT`
- `TOK_DOC_COMMENT`
- `TOK_MODULE_DOC`
- `TOK_DOCUMENT`


## Functions

### `lookup_single`

```luma
pub lookup_single -> fn(
    c: byte
) i64
```

### `lookup_double`

```luma
pub lookup_double -> fn(
    c: byte,
    c2: byte
) i64
```

### `lookup_keyword`

```luma
pub lookup_keyword -> fn(
    val: *byte,
    len: i64
) i64
```


## Variables

- **`MAX_LEX_ERRORS`** : i64 *(const)*
- **`LEX_ERR_UNKNOWN_DIRECTIVE`** : i64 *(const)*
- **`LEX_ERR_UNTERMINATED_STRING`** : i64 *(const)*
- **`LEX_ERR_EMPTY_CHAR`** : i64 *(const)*
- **`LEX_ERR_BAD_CHAR`** : i64 *(const)*
- **`LEX_ERR_INVALID_CHARACTER`** : i64 *(const)*
- **`keyword_map`** : [Keyword; 52] *(const)*
