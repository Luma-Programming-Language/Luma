# Module: constants

*Source: `src/constants.lx`*

Compiler-wide constants and shared configuration types.

Defines the compiler version string, the error-code enum, and the
`LumaBuildConfig` structure that carries CLI options through the
compilation pipeline.

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Variables](#variables)

---

## Structures

### `LumaErrorMessage`

A message/code pair describing a single compiler error.


| Field | Type | Description |
|-------|------|-------------|
| `message` | *byte |  |
| `code` | i64 |  |

### `LumaBuildConfig`

Full compiler configuration derived from command-line arguments.


| Field | Type | Description |
|-------|------|-------------|
| `filepath` | *byte |  |
| `name` | *byte |  |
| `os` | *byte |  |
| `target_explicit` | bool |  |
| `no_compile` | bool |  |
| `save` | bool |  |
| `clean` | bool |  |
| `check_mem` | bool |  |
| `lsp_mode` | bool |  |
| `debug` | bool |  |
| `anim` | bool |  |
| `docs` | bool |  |
| `docs_dir` | *byte |  |
| `file_count` | i64 |  |
| `opt_level` | i64 |  |
| `link_files` | VEC::Vector |  |
| `link_libraries` | VEC::Vector |  |

**Methods:**

#### `LumaBuildConfig::init_build_config()`

```luma
static LumaBuildConfig::init_build_config -> fn(
    filepath: *byte
) LumaBuildConfig
```


## Enumerations

### pub `LumaErrorCode`

Named exit/status codes used across the compiler.

**Values:**

- `Argc_Error`
- `File_Error`
- `Memory_Error`
- `Lexer_Error`
- `Parser_Error`
- `Runtime_Error`
- `NO_FILES_ERROR`
- `Unknown_Error`


## Variables

- **`Luma_Compiler_version`** : *byte *(const)* — Version string reported by `-v`/`--version` and the driver banner.
