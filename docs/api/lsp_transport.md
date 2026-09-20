# Module: lsp_transport

*Source: `src/lsp/lsp_transport.lx`*

JSON-RPC-over-stdio transport for the Luma language server.

Frames messages the way LSP requires: `Content-Length: N\r\n\r\n` followed
by exactly N bytes of UTF-8 JSON. Reading and writing both go through raw
fd 0 / fd 1 syscalls (std_io::local_read/local_write) rather than
buffered stdio, since the message loop needs to read exactly N bytes and
nothing more.

## Table of Contents

- [Functions](#functions)


## Functions

### `read_byte`

Reads a single raw byte from stdin (fd 0).


```luma
      read_byte -> fn(
) i64
```

**Returns:**
-1 on EOF or read error.

### `read_message`

Reads one full Content-Length-framed JSON-RPC message from stdin.


```luma
pub #returns_ownership
read_message -> fn(
) *byte
```

**Parameters:**
- `saved`: real stdout fd to restore once the body is read
- `bytes_len`: output slot that receives the total bytes read


**Returns:**
A heap-allocated, NUL-terminated JSON body, or null on EOF or a bad header.

### `send_raw`

Writes a Content-Length-framed message to stdout.

```luma
pub send_raw -> fn(
    body: *byte
) void
```

### `send_response`

Sends a JSON-RPC response carrying `result` for the request `id`.

```luma
pub send_response -> fn(
    id: i64,
    result: *J::JsonValue
) void
```

### `send_notification`

Sends a JSON-RPC notification with the given `method` and `params`.

```luma
pub send_notification -> fn(
    method: *byte,
    params: *J::JsonValue
) void
```

### `send_error`

Sends a JSON-RPC error response with `code` and `message` for `id`.

```luma
pub send_error -> fn(
    id: i64,
    code: i64,
    message: *byte
) void
```

### `suppress_stdout_begin`

Redirects stdout (fd 1) to /dev/null so the compiler's diagnostic
printer cannot corrupt the JSON-RPC channel.


```luma
pub suppress_stdout_begin -> fn(
) i64
```

**Returns:**
The saved real stdout fd to pass back to `suppress_stdout_end`.

### `suppress_stdout_end`

Restores stdout from the fd saved by `suppress_stdout_begin`.

```luma
pub suppress_stdout_end -> fn(
    saved: i64
) void
```

