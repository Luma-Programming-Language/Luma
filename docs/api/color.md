# Module: color

*Source: `lib/color.lx`*

## Table of Contents

- [Enumerations](#enumerations)
- [Functions](#functions)
- [Variables](#variables)


## Enumerations

### pub `Color`

**Values:**

- `RESET`
- `BLACK`
- `RED`
- `GREEN`
- `YELLOW`
- `BLUE`
- `MAGENTA`
- `CYAN`
- `WHITE`
- `BRIGHT_BLACK`
- `BRIGHT_RED`
- `BRIGHT_GREEN`
- `BRIGHT_YELLOW`
- `BRIGHT_BLUE`
- `BRIGHT_MAGENTA`
- `BRIGHT_CYAN`
- `BRIGHT_WHITE`

### pub `Style`

**Values:**

- `RESET_ALL`
- `BOLD`
- `DIM`
- `ITALIC`
- `UNDERLINE`
- `BLINK`
- `FAST_BLINK`
- `REVERSE`
- `HIDDEN`
- `STRIKETHROUGH`


## Functions

### `fg`

```luma
pub fg -> fn(
    c: i64
) *byte
```

### `bg`

```luma
pub bg -> fn(
    c: i64
) *byte
```

### `style`

```luma
pub style -> fn(
    s: i64
) *byte
```

### `paint`

```luma
pub #returns_ownership
paint -> fn(
    text: *byte,
    c: i64
) *byte
```

### `paint_bg`

```luma
pub #returns_ownership
paint_bg -> fn(
    text: *byte,
    c: i64
) *byte
```

### `paint_with`

```luma
pub #returns_ownership
paint_with -> fn(
    text: *byte,
    c: i64,
    s: i64
) *byte
```

### `fg_rgb`

```luma
pub #returns_ownership
fg_rgb -> fn(
    r: i64,
    g: i64,
    b: i64
) *byte
```

### `bg_rgb`

```luma
pub #returns_ownership
bg_rgb -> fn(
    r: i64,
    g: i64,
    b: i64
) *byte
```


## Variables

- **`RESET_CODE`** : *byte *(const)*
