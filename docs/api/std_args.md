# Module: std_args

*Source: `std/args.lx`*

Command-line argument parsing and iteration.

Provides utilities for working with command-line arguments (argc/argv).
The Args struct offers array-like access to arguments, while ArgsIter
provides sequential iteration. Arguments are indexed from 0, where
index 0 is typically the program name.

# Example
```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    let args: Args = args::init_args(argc, argv);

    if (args.contains("--help")) {
        output("Usage: program [options]\n");
        return 0;
    }

    let filename: *byte = args.get(1);
    return 0;
}
```

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `ArgsIter`

Iterator for traversing command-line arguments.

Provides methods to peek at, consume, and skip arguments sequentially.
Maintains an internal position that advances with each next() call.


| Field | Type | Description |
|-------|------|-------------|
| `data` | **byte |  |
| `remaining` | i64 | Current position in argument array |

**Methods:**

#### `ArgsIter::peek()`

```luma
ArgsIter::peek -> fn(
) *byte
```

#### `ArgsIter::next()`

```luma
ArgsIter::next -> fn(
) *byte
```

#### `ArgsIter::skip()`

```luma
ArgsIter::skip -> fn(
    n: i64
) void
```

### `Args`

Command-line arguments container.

Wraps argc/argv from main() and provides convenient methods for
accessing and querying arguments. Index 0 is the program name,
subsequent indices are user-provided arguments.


| Field | Type | Description |
|-------|------|-------------|
| `data` | **byte |  |
| `count` | i64 | Array of argument strings |

**Methods:**

#### `Args::len()`

```luma
Args::len -> fn(
) i64
```

#### `Args::is_empty()`

```luma
Args::is_empty -> fn(
) i64
```

#### `Args::get()`

```luma
Args::get -> fn(
    index: i64
) *byte
```

#### `Args::contains()`

```luma
Args::contains -> fn(
    val: *byte
) i64
```

#### `Args::tail()`

```luma
Args::tail -> fn(
) Args
```

#### `Args::iter()`

```luma
Args::iter -> fn(
) ArgsIter
```


## Functions

### `init_args`

Initializes Args struct from main() parameters.

Wraps the standard main() argc/argv parameters into an Args struct.
This should be the first call in main() when argument processing is needed.

@param argc Argument count from main()
@param argv Argument vector from main()
@return Initialized Args struct


```luma
pub init_args -> fn(
    argc: i64,
    argv: **byte
) Args
```

**Example:**
```luma
const main -> fn (argc: i64, argv: **byte) i64 {
    let args: Args = args::init_args(argc, argv);

    if (args.contains("--version")) {
        output("Version 1.0.0\n");
        return 0;
    }

    return 0;
}
```

