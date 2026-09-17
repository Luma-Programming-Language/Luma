# Module: parser

*Source: `src/parser/parser.lx`*

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)

---

## Structures

### `Savepoint`

| Field | Type | Description |
|-------|------|-------------|
| `pos` | i64 |  |
| `had_error` | i64 |  |
| `err_count` | i64 |  |

### `Parser`

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


## Enumerations

### pub `BindingPower`

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

```luma
pub search_literal_map -> fn(
    kind: i64
) i64
```

### `search_binop_map`

```luma
pub search_binop_map -> fn(
    kind: i64
) i64
```

### `search_unop_map`

```luma
pub search_unop_map -> fn(
    kind: i64
) i64
```

### `get_bp`

```luma
pub get_bp -> fn(
    kind: i64
) i64
```

### `create_parser`

```luma
pub #returns_ownership
create_parser -> fn(
    tks: *TOK::Token,
    tok_count: i64,
    file_path: *byte
) *Parser
```

### `free_parser`

```luma
pub #takes_ownership
free_parser -> fn(
    p: *Parser
) void
```

### `vec_to_node_array`

```luma
pub #returns_ownership
vec_to_node_array -> fn(
    v: *VEC::Vector
) **AST::AstNode
```

### `vec_to_byte_array`

```luma
pub #returns_ownership
vec_to_byte_array -> fn(
    v: *VEC::Vector
) **byte
```

