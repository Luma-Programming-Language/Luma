# Module: std_libc

*Source: `std/libc.lx`*

Direct FFI bindings to libc standard library functions.

Provides raw access to commonly used C standard library functions
grouped by header: stdio.h, stdlib.h, time.h, string.h, and math.h.
These are thin wrappers — see the corresponding C man pages for
detailed documentation.

PLATFORM: Linux x86_64 (glibc)

# Example
```luma
let msg: *byte = "hello, world\n";
libc::printf("%s\n", msg);
```

## Table of Contents

- [Functions](#functions)
- [Variables](#variables)
- [Linked Libraries](#linked-libraries)


## Functions

### `printf`

Formatted print to stdout

```luma
pub printf -> fn(
    fmt: *byte,
    val: *byte
) i64
```

### `puts`

Print string with trailing newline to stdout

```luma
pub puts -> fn(
    s: *byte
) i64
```

### `putchar`

Write a single character to stdout

```luma
pub putchar -> fn(
    c: i64
) i64
```

### `getchar`

Read a single character from stdin

```luma
pub getchar -> fn(
) i64
```

### `fflush`

Flush a file stream's output buffer

```luma
pub fflush -> fn(
    stream: *void
) i64
```

### `fopen`

Open a file for reading/writing

```luma
pub fopen -> fn(
    path: *byte,
    mode: *byte
) *void
```

### `fclose`

Close an open file

```luma
pub fclose -> fn(
    stream: *void
) i64
```

### `fread`

Read raw bytes from a file

```luma
pub fread -> fn(
    ptr: *void,
    size: i64,
    count: i64,
    stream: *void
) i64
```

### `fwrite`

Write raw bytes to a file

```luma
pub fwrite -> fn(
    ptr: *void,
    size: i64,
    count: i64,
    stream: *void
) i64
```

### `fgets`

Read a line from a file

```luma
pub fgets -> fn(
    buf: *byte,
    n: i64,
    stream: *void
) *byte
```

### `fputs`

Write a string to a file

```luma
pub fputs -> fn(
    s: *byte,
    stream: *void
) i64
```

### `feof`

Test for end-of-file on a stream

```luma
pub feof -> fn(
    stream: *void
) i64
```

### `ferror`

Test for error on a stream

```luma
pub ferror -> fn(
    stream: *void
) i64
```

### `rewind`

Set file position to the beginning

```luma
pub rewind -> fn(
    stream: *void
) void
```

### `fseek`

Set file position indicator

```luma
pub fseek -> fn(
    stream: *void,
    offset: i64,
    whence: i64
) i64
```

### `ftell`

Get current file position

```luma
pub ftell -> fn(
    stream: *void
) i64
```

### `remove`

Delete a file

```luma
pub remove -> fn(
    path: *byte
) i64
```

### `rename`

Rename a file

```luma
pub rename -> fn(
    old: *byte,
    new: *byte
) i64
```

### `tmpfile`

Create a temporary binary file

```luma
pub tmpfile -> fn(
) *void
```

### `malloc`

Allocate memory

```luma
pub malloc -> fn(
    size: i64
) *void
```

### `calloc`

Allocate and zero-initialize memory

```luma
pub calloc -> fn(
    count: i64,
    size: i64
) *void
```

### `realloc`

Resize an allocated memory block

```luma
pub realloc -> fn(
    ptr: *void,
    size: i64
) *void
```

### `_free`

Free allocated memory

```luma
pub _free -> fn(
    ptr: *void
) void
```

### `exit`

Terminate the program with a status code

```luma
pub exit -> fn(
    code: i64
) void
```

### `abort`

Terminate the program abnormally

```luma
pub abort -> fn(
) void
```

### `abs`

Return the absolute value of an integer

```luma
pub abs -> fn(
    n: i64
) i64
```

### `atoi`

Convert ASCII string to i64

```luma
pub atoi -> fn(
    s: *byte
) i64
```

### `atof`

Convert ASCII string to f64

```luma
pub atof -> fn(
    s: *byte
) f64
```

### `atol`

Convert ASCII string to long

```luma
pub atol -> fn(
    s: *byte
) i64
```

### `rand`

Generate a pseudo-random integer

```luma
pub rand -> fn(
) i64
```

### `srand`

Seed the pseudo-random number generator

```luma
pub srand -> fn(
    seed: i64
) void
```

### `getenv`

Get an environment variable

```luma
pub getenv -> fn(
    name: *byte
) *byte
```

### `_system`

Execute a shell command

```luma
pub _system -> fn(
    cmd: *byte
) i64
```

### `qsort`

Sort an array using quicksort

```luma
pub qsort -> fn(
    base: *void,
    count: i64,
    size: i64,
    cmp: *void
) void
```

### `bsearch`

Binary search in a sorted array

```luma
pub bsearch -> fn(
    key: *void,
    base: *void,
    count: i64,
    size: i64,
    cmp: *void
) *void
```

### `time`

Get current calendar time

```luma
pub time -> fn(
    t: *i64
) i64
```

### `clock`

Get processor clock time

```luma
pub clock -> fn(
) i64
```

### `difftime`

Compute difference between two times

```luma
pub difftime -> fn(
    t1: i64,
    t0: i64
) f64
```

### `strlen`

Get string length

```luma
pub strlen -> fn(
    s: *byte
) i64
```

### `strcpy`

Copy a string (unsafe — may overflow)

```luma
pub strcpy -> fn(
    dst: *byte,
    src: *byte
) *byte
```

### `strncpy`

Copy a string with length limit

```luma
pub strncpy -> fn(
    dst: *byte,
    src: *byte,
    n: i64
) *byte
```

### `strcat`

Concatenate strings (unsafe — may overflow)

```luma
pub strcat -> fn(
    dst: *byte,
    src: *byte
) *byte
```

### `strncat`

Concatenate strings with length limit

```luma
pub strncat -> fn(
    dst: *byte,
    src: *byte,
    n: i64
) *byte
```

### `strcmp`

Compare two strings

```luma
pub strcmp -> fn(
    a: *byte,
    b: *byte
) i64
```

### `strncmp`

Compare two strings with length limit

```luma
pub strncmp -> fn(
    a: *byte,
    b: *byte,
    n: i64
) i64
```

### `strchr`

Find first occurrence of a character in a string

```luma
pub strchr -> fn(
    s: *byte,
    c: i64
) *byte
```

### `strrchr`

Find last occurrence of a character in a string

```luma
pub strrchr -> fn(
    s: *byte,
    c: i64
) *byte
```

### `strstr`

Find a substring in a string

```luma
pub strstr -> fn(
    haystack: *byte,
    needle: *byte
) *byte
```

### `strtok`

Split string into tokens

```luma
pub strtok -> fn(
    s: *byte,
    delim: *byte
) *byte
```

### `memset`

Fill memory with a constant byte

```luma
pub memset -> fn(
    ptr: *void,
    val: i64,
    n: i64
) *void
```

### `memcpy`

Copy memory (may overlap)

```luma
pub memcpy -> fn(
    dst: *void,
    src: *void,
    n: i64
) *void
```

### `memmove`

Copy memory (handles overlap)

```luma
pub memmove -> fn(
    dst: *void,
    src: *void,
    n: i64
) *void
```

### `memcmp`

Compare memory regions

```luma
pub memcmp -> fn(
    a: *void,
    b: *void,
    n: i64
) i64
```

### `strerror`

Return a human-readable message for an errno value

```luma
pub strerror -> fn(
    errnum: i64
) *byte
```

### `access`

Check a path's existence/accessibility (see F_OK/R_OK/W_OK/X_OK below)

```luma
pub access -> fn(
    path: *byte,
    mode: i64
) i64
```

### `sqrt`

Square root

```luma
pub #lib_import("libm.so")
sqrt -> fn(
    x: f64
) f64
```

### `pow`

Power (base^exp)

```luma
pub #lib_import("libm.so")
pow -> fn(
    base: f64,
    exp: f64
) f64
```

### `floor`

Round down to nearest integer

```luma
pub #lib_import("libm.so")
floor -> fn(
    x: f64
) f64
```

### `ceil`

Round up to nearest integer

```luma
pub #lib_import("libm.so")
ceil -> fn(
    x: f64
) f64
```

### `fabs`

Absolute value of an f64

```luma
pub #lib_import("libm.so")
fabs -> fn(
    x: f64
) f64
```

### `fmod`

Floating-point remainder of x/y

```luma
pub #lib_import("libm.so")
fmod -> fn(
    x: f64,
    y: f64
) f64
```

### `log`

Natural logarithm

```luma
pub #lib_import("libm.so")
log -> fn(
    x: f64
) f64
```

### `log2`

Base-2 logarithm

```luma
pub #lib_import("libm.so")
log2 -> fn(
    x: f64
) f64
```

### `log10`

Base-10 logarithm

```luma
pub #lib_import("libm.so")
log10 -> fn(
    x: f64
) f64
```

### `exp`

Exponential (e^x)

```luma
pub #lib_import("libm.so")
exp -> fn(
    x: f64
) f64
```

### `sin`

Sine of an angle (radians)

```luma
pub #lib_import("libm.so")
sin -> fn(
    x: f64
) f64
```

### `cos`

Cosine of an angle (radians)

```luma
pub #lib_import("libm.so")
cos -> fn(
    x: f64
) f64
```

### `tan`

Tangent of an angle (radians)

```luma
pub #lib_import("libm.so")
tan -> fn(
    x: f64
) f64
```

### `atan2`

Arc tangent (y/x) in radians

```luma
pub #lib_import("libm.so")
atan2 -> fn(
    y: f64,
    x: f64
) f64
```


## Variables

- **`F_OK`** : i64 *(const)* — `access()` mode: file exists
- **`R_OK`** : i64 *(const)* — `access()` mode: read permission
- **`W_OK`** : i64 *(const)* — `access()` mode: write permission
- **`X_OK`** : i64 *(const)* — `access()` mode: execute (or search, for a directory) permission

## Linked Libraries

External native libraries linked by this module.

> **FFI library:** `libc.so.6`
>

