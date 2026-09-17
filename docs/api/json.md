# Module: json

*Source: `lib/json.lx`*

A small, self-contained JSON library.

Parses JSON text into a `JsonValue` tree and serializes a `JsonValue` tree
back into compact JSON text. Objects and arrays are backed by
`std_vector::Vector` and preserve insertion order; object lookups are a
linear scan over the keys (fine for message-sized documents like LSP
payloads — not meant for huge documents).

Every `*JsonValue` returned by a constructor or by `json_parse` is owned
by the caller and must eventually be released with `json_free`, which
recursively frees children, keys, and vectors.

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)

---

## Structures

### `JsonValue`

| Field | Type | Description |
|-------|------|-------------|
| `kind` | i64 |  |
| `b` | bool |  |
| `num` | f64 |  |
| `str` | *byte |  |
| `arr` | VEC::Vector |  |
| `keys` | VEC::Vector |  |
| `vals` | VEC::Vector |  |


## Enumerations

### pub `JsonType`

**Values:**

- `JSON_NULL`
- `JSON_BOOL`
- `JSON_NUMBER`
- `JSON_STRING`
- `JSON_ARRAY`
- `JSON_OBJECT`


## Functions

### `json_free`

```luma
pub json_free -> fn(
    v: *JsonValue
) void
```

### `json_null`

```luma
pub #returns_ownership
json_null -> fn(
) *JsonValue
```

### `json_bool`

```luma
pub #returns_ownership
json_bool -> fn(
    val: bool
) *JsonValue
```

### `json_number`

```luma
pub #returns_ownership
json_number -> fn(
    val: f64
) *JsonValue
```

### `json_int`

```luma
pub #returns_ownership
json_int -> fn(
    val: i64
) *JsonValue
```

### `json_string`

```luma
pub #returns_ownership
json_string -> fn(
    s: *byte
) *JsonValue
```

### `json_array`

```luma
pub #returns_ownership
json_array -> fn(
) *JsonValue
```

### `json_object`

```luma
pub #returns_ownership
json_object -> fn(
) *JsonValue
```

### `json_array_push`

```luma
pub json_array_push -> fn(
    arr: *JsonValue,
    item: *JsonValue
) void
```

### `json_object_set`

```luma
pub json_object_set -> fn(
    obj: *JsonValue,
    key: *byte,
    val: *JsonValue
) void
```

### `json_array_len`

```luma
pub json_array_len -> fn(
    v: *JsonValue
) i64
```

### `json_array_get`

```luma
pub json_array_get -> fn(
    v: *JsonValue,
    idx: i64
) *JsonValue
```

### `json_object_get`

```luma
pub json_object_get -> fn(
    obj: *JsonValue,
    key: *byte
) *JsonValue
```

### `json_is_null`

```luma
pub json_is_null -> fn(
    v: *JsonValue
) bool
```

### `json_get_string`

```luma
pub json_get_string -> fn(
    v: *JsonValue
) *byte
```

### `json_get_int`

```luma
pub json_get_int -> fn(
    v: *JsonValue
) i64
```

### `json_get_double`

```luma
pub json_get_double -> fn(
    v: *JsonValue
) f64
```

### `json_get_bool`

```luma
pub json_get_bool -> fn(
    v: *JsonValue
) bool
```

### `json_object_get_string`

```luma
pub json_object_get_string -> fn(
    obj: *JsonValue,
    key: *byte
) *byte
```

### `json_object_get_int`

```luma
pub json_object_get_int -> fn(
    obj: *JsonValue,
    key: *byte
) i64
```

### `json_free`

```luma
pub json_free -> fn(
    v: *JsonValue
) void
```

### `json_parse`

```luma
pub #returns_ownership
json_parse -> fn(
    src: *byte
) *JsonValue
```

### `json_stringify`

```luma
pub #returns_ownership
json_stringify -> fn(
    v: *JsonValue
) *byte
```

