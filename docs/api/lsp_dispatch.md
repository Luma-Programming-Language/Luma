# Module: lsp_dispatch

*Source: `src/lsp/lsp_dispatch.lx`*

JSON-RPC method dispatch for the Luma language server.

## Table of Contents

- [Functions](#functions)


## Functions

### `wrap_code_block`

Formats the declaration as code and keeps its documentation as Markdown.

```luma
      #returns_ownership
wrap_code_block -> fn(
    sig: *byte
) *byte
```

### `capabilities_result`

Builds the ServerCapabilities result returned for the `initialize` request.

```luma
      #returns_ownership
capabilities_result -> fn(
) *J::JsonValue
```

### `handle_message`

Parses and dispatches one JSON-RPC message to the document API.


```luma
pub handle_message -> fn(
    raw: *byte
) bool
```

**Returns:**
True when the client sent the `exit` notification.

