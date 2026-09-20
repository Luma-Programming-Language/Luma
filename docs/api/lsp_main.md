# Module: lsp_main

*Source: `src/lsp/lsp_main.lx`*

Entry point for the self-hosted Luma language server.

Speaks LSP over stdio: reads Content-Length-framed JSON-RPC messages
from stdin and writes responses/notifications to stdout. Reached via
the main `luma` binary's `-lsp`/`--lsp` flag (see src/main.lx) rather
than being its own executable — an editor's LSP client config just
points at `luma -lsp`.

## Table of Contents

- [Functions](#functions)


## Functions

### `lsp_run`

Runs the language-server event loop, reading messages from stdin and
dispatching them until stdin closes.


```luma
pub lsp_run -> fn(
) i64
```

**Returns:**
0 when the loop ends.

