# Module: parser

*Source: `src/parser/parser.lx`*

Core parser state and helpers: the Parser struct over the token stream,
operator binding powers, and the token-to-operator lookup tables used by
the expression and statement parsers.

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `PRIMARY_LITERAL`

Maps a literal-producing token kind to its AST LiteralType kind.

| Field | Type | Description |
|-------|------|-------------|
| `tok` | i64 |  |
| `literal` | i64 |  |

### `TOKEN_TO_BINOP`

Maps an operator token kind to its AST BinaryOp kind.

| Field | Type | Description |
|-------|------|-------------|
| `tok` | i64 |  |
| `binop` | i64 |  |

### `TOKEN_TO_UNOP`

Maps an operator token kind to its AST UnaryType kind.

| Field | Type | Description |
|-------|------|-------------|
| `tok` | i64 |  |
| `unop` | i64 |  |

### `Savepoint`

Snapshot of parser position, sticky error flag and global error-list
length, used to roll back a failed speculative parse.

| Field | Type | Description |
|-------|------|-------------|
| `pos` | i64 |  |
| `had_error` | i64 |  |
| `err_count` | i64 |  |

### `Parser`

The main parser state: owns the token table, current position, file path
and a sticky error flag, and exposes token-stream navigation plus
speculative-parse save/restore.


| Field | Type | Description |
|-------|------|-------------|
| `file_path` | *byte |  |
| `tks` | *TOK::Token |  |
| `tok_count` | i64 |  |
| `pos` | i64 |  |
| `had_error` | i64 |  |

**Methods:**

#### `Parser::has_tokens()`

```luma
Parser::has_tokens -> fn(
) bool
```

#### `Parser::peek()`

```luma
Parser::peek -> fn(
    offset: i64
) TOK::Token
```

#### `Parser::current()`

```luma
Parser::current -> fn(
) TOK::Token
```

#### `Parser::advance()`

```luma
Parser::advance -> fn(
) TOK::Token
```

#### `Parser::consume()`

```luma
Parser::consume -> fn(
    kind: i64,
    err_msg: *byte
) TOK::Token
```

#### `Parser::error()`

```luma
Parser::error -> fn(
    err_type: *byte,
    err_msg: *byte
) void
```

#### `Parser::save()`

```luma
Parser::save -> fn(
) Savepoint
```

#### `Parser::restore()`

```luma
Parser::restore -> fn(
    sp: Savepoint
) void
```

#### `Parser::get_name()`

```luma
#returns_ownership
Parser::get_name -> fn(
) *byte
```

#### `Parser::create_parser()`

```luma
static #returns_ownership
Parser::create_parser -> fn(
    tks: *TOK::Token,
    tok_count: i64,
    file_path: *byte
) *Parser
```

#### `Parser::free_parser()`

```luma
static #takes_ownership
Parser::free_parser -> fn(
    p: *Parser
) void
```


## Enumerations

### pub `BindingPower`

Precedence (binding power) levels assigned to operator tokens for the
Pratt-style expression parser.

**Values:**

- `BP_NONE`
- `BP_LOWEST`
- `BP_ASSIGN`
- `BP_TERNARY`
- `BP_LOGICAL_OR`
- `BP_LOGICAL_AND`
- `BP_BITWISE_OR`
- `BP_BITWISE_XOR`
- `BP_BITWISE_AND`
- `BP_EQUALITY`
- `BP_RELATIONAL`
- `BP_RANGE`
- `BP_SHIFT`
- `BP_SUM`
- `BP_PRODUCT`
- `BP_EXPONENT`
- `BP_UNARY`
- `BP_POSTFIX`
- `BP_CALL`
- `BP_PRIMARY`


## Functions

### `search_literal_map`

Searches `PRIMARY_LILERAL_MAP` for a token kind, returning the matching
LiteralType kind or -1.

```luma
pub search_literal_map -> fn(
    kind: i64
) i64
```

### `search_binop_map`

Searches `BINOP_MAP` for a token kind, returning the matching BinaryOp
kind or -1.

```luma
pub search_binop_map -> fn(
    kind: i64
) i64
```

### `search_unop_map`

Searches `UNOP_MAP` for a token kind, returning the matching UnaryType
kind or -1.

```luma
pub search_unop_map -> fn(
    kind: i64
) i64
```

### `get_bp`

Returns the binding-power level for an operator token kind, or BP_NONE.

```luma
pub get_bp -> fn(
    kind: i64
) i64
```

### `vec_to_node_array`

Copies the contents of a Vector of `*AST::AstNode` into a new
heap-allocated array and returns it (the caller owns it).

```luma
pub #returns_ownership
vec_to_node_array -> fn(
    v: *VEC::Vector
) **AST::AstNode
```

### `vec_to_byte_array`

Copies the contents of a Vector of `*byte` into a new heap-allocated array
and returns it (the caller owns it).

```luma
pub #returns_ownership
vec_to_byte_array -> fn(
    v: *VEC::Vector
) **byte
```


## Variables

- **`PRIMARY_LITERAL_TYPE_MAP`** : i64 *(const)* — Number of entries in `PRIMARY_LILERAL_MAP`.
- **`TOKEN_TO_BINOP_MAP`** : i64 *(const)* — Number of entries in `BINOP_MAP`.
- **`TOKEN_TO_UNOP_MAP`** : i64 *(const)* — Number of entries in `UNOP_MAP`.
- **`PRIMARY_LILERAL_MAP`** : [PRIMARY_LITERAL; 7] *(const)* — Lookup table from literal-producing token kinds to AST LiteralType kinds.
- **`BINOP_MAP`** : [TOKEN_TO_BINOP; 19] *(const)* — Lookup table from operator tokens to AST BinaryOp kinds.
- **`UNOP_MAP`** : [TOKEN_TO_UNOP; 6] *(const)* — Lookup table from prefix operator tokens to AST UnaryType kinds.
- **`EOF_TOK`** : TOK::Token *(const)* — The sentinel EOF token returned by navigation methods when the parser
