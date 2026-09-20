# Module: main

*Source: `src/main.lx`*

Entry point of the Luma compiler driver.

Parses command-line arguments, then drives the parse → typecheck →
codegen → compile pipeline (or the docs / LSP passes) and returns the
process exit status.

## Table of Contents

- [Functions](#functions)


## Functions

### `main`

Program entry point: parses CLI arguments into a build config and runs
the requested pipeline — the Markdown docs pass, the LSP, or typecheck
plus transpile-to-C plus compile.


```luma
pub main -> fn(
    argc: i64,
    argv: **byte
) i64
```

**Returns:**
`0` on success; `1` or a `LumaErrorCode` status on failure.

