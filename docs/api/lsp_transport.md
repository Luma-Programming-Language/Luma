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

### `read_message`

```luma
pub #returns_ownership
read_message -> fn(
) *byte
```

### `send_raw`

```luma
pub send_raw -> fn(
    body: *byte
) void
```

### `send_response`

```luma
pub send_response -> fn(
    id: i64,
    result: *J::JsonValue
) void
```

### `send_notification`

```luma
pub send_notification -> fn(
    method: *byte,
    params: *J::JsonValue
) void
```

### `send_error`

```luma
pub send_error -> fn(
    id: i64,
    code: i64,
    message: *byte
) void
```

### `suppress_stdout_begin`

```luma
pub suppress_stdout_begin -> fn(
) i64
```

### `suppress_stdout_end`

```luma
pub suppress_stdout_end -> fn(
    saved: i64
) void
```

