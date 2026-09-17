# Module: std_io

*Source: `std/io.lx`*

Input/output operations module

Raw fd primitives backed by direct syscalls (POSIX) or the Win32 file and
console API (Windows), plus higher-level helpers for whole-file reads,
whole-buffer writes, and formatted output.

The low-level layer is deliberately unbuffered and synchronous: it reads
and writes exactly the requested byte count, with no stdio buffering that
could hide bytes from exact-read/exact-write consumers (src/lsp/
lsp_transport.lx's Content-Length framing depends on this).

This module is completely self-contained — it imports no other std
modules: the handful of memory/string primitives it needs (memcpy,
realloc, strlen, integer-to-string) are defined here.

PLATFORM: Linux x86_64, macOS x86_64/ARM64, Windows x86_64

# Memory ownership
* `read_file` returns a heap buffer owned by the caller (freed with
  the built-in `free()`).
* `read_binary`/`write_binary` never allocate — they touch caller buffers.
* `print`/`print_err` own nothing on return.

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)
- [Variables](#variables)
- [OS-Specific](#os-specific)

---

## Structures

### `FormatArg`

Type-tagged argument for `print`, so each specifier and the value bound to
it stay in sync: %d takes `int_arg`, %c takes `byte_arg`, %s takes
`str_arg`. A specifier whose argument has the wrong tag (or runs past the
end of `args`) is emitted literally.

| Field | Type | Description |
|-------|------|-------------|
| `tag` | i64 |  |
| `str_ptr` | *byte |  |
| `int_val` | i64 |  |
| `byte_val` | byte |  |


## Functions

### `local_write`

Writes `count` bytes from `buf` to `fd`.

Returns the number of bytes actually written (may be fewer than `count`
for a short write; callers that need a full write must loop).


```luma
pub local_write -> fn(
    fd: i64,
    buf: *void,
    count: i64
) i64
```

**Parameters:**
* `fd` - Target file descriptor (or Win32 handle)
* `buf` - Bytes to write
* `count` - Number of bytes to write

### `local_read`

Reads up to `count` bytes from `fd` into `buf`.

Returns the number of bytes read: 0 on EOF, a negative error indicator on
failure (check with `local_is_error`).


```luma
pub local_read -> fn(
    fd: i64,
    buf: *void,
    count: i64
) i64
```

**Parameters:**
* `fd` - Source file descriptor (or Win32 handle)
* `buf` - Destination buffer
* `count` - Maximum number of bytes to read

### `local_open_read`

Opens an existing file read-only.

Returns an fd that must be closed with `local_close`, or a negative /
INVALID_HANDLE_VALUE error on failure.


```luma
pub local_open_read -> fn(
    path: *byte
) i64
```

**Parameters:**
* `path` - Null-terminated path to open

### `local_open_write`

Opens an existing file read-write. Does NOT create the file (O_RDWR with
no O_CREAT); use `local_open_create` when the file may not exist yet.

Returns an fd that must be closed with `local_close`, or an error on
failure.


```luma
pub local_open_write -> fn(
    path: *byte
) i64
```

**Parameters:**
* `path` - Null-terminated path to open

### `local_open_create`

Opens a file for writing, creating and truncating it if necessary
(O_WRONLY | O_CREAT | O_TRUNC), so the named file is guaranteed to exist
and have zero length on success.

Returns an fd that must be closed with `local_close`, or an error on
failure.


```luma
pub local_open_create -> fn(
    path: *byte
) i64
```

**Parameters:**
* `path` - Null-terminated path to open

### `local_close`

Closes an fd (or Win32 handle) previously returned by an `local_open_*`.


```luma
pub local_close -> fn(
    fd: i64
) i64
```

**Parameters:**
* `fd` - Handle to close

### `local_is_error`

Detects the platform's "invalid handle / read or write failed" return
value for the fds returned by `local_open_*`, reads, and writes.


```luma
pub local_is_error -> fn(
    result: i64
) bool
```

**Parameters:**
* `result` - Return value from an `local_*` operation to test

### `write_binary`

Writes `size` raw bytes from `data` to `path`, creating the file first.

Returns the number of bytes written, or -1 if the file could not be opened.


```luma
pub write_binary -> fn(
    path: *byte,
    data: *void,
    size: i64
) i64
```

**Parameters:**
* `path` - Null-terminated destination path
* `data` - Raw bytes to write
* `size` - Number of bytes to write

### `read_binary`

Reads up to `size` raw bytes from `path` into `data`.

Returns the number of bytes read (0 for an empty file), or -1 if the file
could not be opened. The caller must ensure `data` can hold `size` bytes.


```luma
pub read_binary -> fn(
    path: *byte,
    data: *void,
    size: i64
) i64
```

**Parameters:**
* `path` - Null-terminated source path
* `data` - Destination buffer
* `size` - Maximum number of bytes to read

### `read_file`

Reads an entire text file into a single heap-allocated, NUL-terminated
buffer, growing as needed.

The returned buffer is tightly sized to (content + 3) and its last three
bytes are NUL, so tokenizers and LSP lookahead that sniff a byte or two
past the terminator never read uninitialized heap — this matches the
historical contract of this function. The caller owns the returned buffer
and must `free()` it.

Returns NULL (0) on any open/read/allocation failure, after emitting a
diagnostic on stderr.


```luma
pub #returns_ownership
read_file -> fn(
    path: *byte
) *byte
```

**Parameters:**
* `path` - Null-terminated path to read

### `write_buffer_to_file`

Writes the NUL-terminated `buffer` to `path`.

The target file must already exist (`local_open_write` does not create
files); see `write_binary` for create-capable raw output. Returns the
number of bytes written, or -1 on error.


```luma
pub write_buffer_to_file -> fn(
    path: *byte,
    buffer: *byte
) i64
```

**Parameters:**
* `path` - Null-terminated destination path
* `buffer` - Null-terminated text to write

### `str_arg`

Builds a `%s` argument from a null-terminated string.


```luma
pub str_arg -> fn(
    arg: *byte
) FormatArg
```

**Parameters:**
* `arg` - String to render

### `int_arg`

Builds a `%d` argument from an integer.


```luma
pub int_arg -> fn(
    arg: i64
) FormatArg
```

**Parameters:**
* `arg` - Value to render

### `byte_arg`

Builds a `%c` argument from a single character.


```luma
pub byte_arg -> fn(
    arg: byte
) FormatArg
```

**Parameters:**
* `arg` - Character to render

### `print`

printf-style output to stdout. `%s`, `%d` and `%c` each consume one
`FormatArg` built via `str_arg`/`int_arg`/`byte_arg`; a specifier with no
matching argument is emitted literally.

The result is buffered into a growable heap buffer and written in a single
syscall, so output is never truncated. Returns the number of bytes written,
or -1 on allocation failure.


```luma
pub print -> fn(
    s: *byte,
    args: [FormatArg; 256]
) i64
```

**Parameters:**
* `s` - Format string; `%s`, `%d` and `%c` are supported
* `args` - One `FormatArg` per specifier used in `s`

### `print_err`

printf-style output to stderr. Takes plain integers so error messages can
be formatted inline without building `FormatArg` values: `%d` consumes one
argument, everything else is copied literally.

The result is buffered into a growable heap buffer and written in a single
syscall, so output is never truncated. Returns the number of bytes written,
or -1 on allocation failure.


```luma
pub print_err -> fn(
    s: *byte,
    args: [i64; 256]
) i64
```

**Parameters:**
* `s` - Format string; `%d` is supported
* `args` - Integer values, one per `%d` in `s`


## Variables

- **`NULL_FORMAT_ARG`** : FormatArg *(const)* — Default/empty `FormatArg` sentinel (tag -1 matches no specifier).

## OS-Specific

### `"linux"`


### `"macos"`


### `"windows64"`

### `WriteFile`

```luma
pub #dll_import("kernel32.dll, callconv: "stdcall")
WriteFile -> fn(
    hFile: i64,
    lpBuffer: *void,
    nNumberOfBytesToWrite: i64,
    lpNumberOfBytesWritten: *i64,
    lpOverlapped: *void
) i64
```

### `ReadFile`

```luma
pub #dll_import("kernel32.dll, callconv: "stdcall")
ReadFile -> fn(
    hFile: i64,
    lpBuffer: *void,
    nNumberOfBytesToRead: i64,
    lpNumberOfBytesRead: *i64,
    lpOverlapped: *void
) i64
```

### `CreateFileA`

```luma
pub #dll_import("kernel32.dll, callconv: "stdcall")
CreateFileA -> fn(
    lpFileName: *byte,
    dwDesiredAccess: i64,
    dwShareMode: i64,
    lpSecurityAttributes: *void,
    dwCreationDisposition: i64,
    dwFlagsAndAttributes: i64,
    hTemplateFile: i64
) i64
```

### `CloseHandle`

```luma
pub #dll_import("kernel32.dll, callconv: "stdcall")
CloseHandle -> fn(
    hObject: i64
) i64
```

### `GetStdHandle`

```luma
pub #dll_import("kernel32.dll, callconv: "stdcall")
GetStdHandle -> fn(
    nStdHandle: i64
) i64
```

### `SetStdHandle`

```luma
pub #dll_import("kernel32.dll, callconv: "stdcall")
SetStdHandle -> fn(
    nStdHandle: i64,
    hHandle: i64
) i64
```


