# Module: lsp_transport

*Source: `src/lsp/lsp_transport.lx`*

JSON-RPC-over-stdio transport for the Luma language server.

Frames messages the way LSP requires: `Content-Length: N\r\n\r\n` followed
by exactly N bytes of UTF-8 JSON. Reading and writing go through raw
std_io::local_read/local_write calls rather than buffered stdio, since
the message loop needs to read exactly N bytes and nothing more. POSIX
uses descriptors; Windows uses standard handles.

## Table of Contents

- [Functions](#functions)


## Functions

### `stdin_handle`

```luma
      stdin_handle -> fn(
) i64
```

### `stdout_handle`

```luma
      stdout_handle -> fn(
) i64
```

### `read_byte`

Reads a single raw byte from standard input.


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

