# Module: progress_bar

*Source: `lib/progress_bar.lx`*

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)
- [OS-Specific](#os-specific)

---

## Structures


## Functions


## OS-Specific

### `"linux"`

### `start`

```luma
pub start -> fn(
    bar: *ProgressBar
) i64
```

### `stop`

```luma
pub stop -> fn(
    tid: i64,
    bar: *ProgressBar
) void
```


### `"macos"`

### `start`

```luma
pub start -> fn(
    bar: *ProgressBar
) i64
```

### `stop`

```luma
pub stop -> fn(
    tid: i64,
    bar: *ProgressBar
) void
```


### `"windows64"`

### `start`

```luma
pub start -> fn(
    bar: *ProgressBar
) i64
```

### `stop`

```luma
pub stop -> fn(
    tid: i64,
    bar: *ProgressBar
) void
```


### `"windows32"`

### `start`

```luma
pub start -> fn(
    bar: *ProgressBar
) i64
```

### `stop`

```luma
pub stop -> fn(
    tid: i64,
    bar: *ProgressBar
) void
```


