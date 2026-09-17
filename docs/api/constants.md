# Module: constants

*Source: `src/constants.lx`*

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `LumaErrorMessage`

| Field | Type | Description |
|-------|------|-------------|
| `message` | *byte |  |
| `code` | i64 |  |

### `LumaBuildConfig`

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


## Enumerations

### pub `LumaErrorCode`

**Values:**

- `Argc_Error`
- `File_Error`
- `Memory_Error`
- `Lexer_Error`
- `Parser_Error`
- `Runtime_Error`
- `NO_FILES_ERROR`
- `Unknown_Error`


## Functions

### `init_build_config`

```luma
pub init_build_config -> fn(
    filepath: *byte
) LumaBuildConfig
```


## Variables

- **`Luma_Compiler_version`** : *byte *(const)*
