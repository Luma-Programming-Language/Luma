# Module: commands

*Source: `src/commands.lx`*

Command-line argument parsing and driver text for the `luma` tool.

Provides the help/version/license output, the `parse_args` flag →
`LumaBuildConfig` walk, and std-lib install-path resolution.

## Table of Contents

- [Functions](#functions)
- [OS-Specific](#os-specific)


## Functions

### `print_commands`

Prints the full usage banner and option list for the `luma` driver.

```luma
pub print_commands -> fn(
) void
```

### `print_license`

Prints the compiler version and license info.

```luma
pub print_license -> fn(
) void
```

### `match_command`

Returns 1 if `cmd` matches one of the aliases in `usr_cmd`, else 0.


```luma
pub match_command -> fn(
    cmd: *byte,
    usr_cmd: [*byte; 10],
    count: i64
) i64
```

**Parameters:**
- `cmd`: The argument to test
- `usr_cmd`: Array of accepted alias strings
- `count`: Number of aliases in `usr_cmd`

### `parse_args`

Parses command-line arguments into a build config.

Handles the `-h`/`-v`/`-lc`/`-lsp` short-circuit cases first, then walks
the remaining flags (`-l`/`--link`, `-name`, `-O*`, `-t`/`--target-os`,
`-save`, `-debug`, `-c`/`--no-compile`, `-docs`, ...) into `build_config`.


```luma
pub parse_args -> fn(
    args: *ARGS::Args,
    build_config: *CONST::LumaBuildConfig
) i64
```

**Returns:**
`0` to proceed with the build, `1` after an informational print, or
`LumaErrorCode::NO_FILES_ERROR` when no file argument was supplied.

### `build_user_prefix`

Builds the per-user std-lib prefix (`$HOME/.luma/`, or
`%USERPROFILE%\.luma\` on Windows). Returns NULL when the home
environment variable is unset; the caller owns the result.

```luma
      #returns_ownership
build_user_prefix -> fn(
) *byte
```

### `resolve_path`

Resolves a std-lib import to an existing file. Tries the path exactly as
given first (so local source wins), then under the per-user install, then
under the system prefix; returns the first readable match (an allocated
path, or the given `path` itself as a fallback). The caller owns the
result.

```luma
pub #returns_ownership
resolve_path -> fn(
    path: *byte
) *byte
```


## OS-Specific

### `"linux"`

- **`SYS_STD_PREFIX`** : *byte *(const)*

### `"macos"`

- **`SYS_STD_PREFIX`** : *byte *(const)*

### `"windows32"`

- **`SYS_STD_PREFIX`** : *byte *(const)*

### `"windows64"`

- **`SYS_STD_PREFIX`** : *byte *(const)*

### `"linux"`

- **`HOME_ENV_VAR`** : *byte *(const)*
- **`USER_STD_SUFFIX`** : *byte *(const)*

### `"macos"`

- **`HOME_ENV_VAR`** : *byte *(const)*
- **`USER_STD_SUFFIX`** : *byte *(const)*

### `"windows32"`

- **`HOME_ENV_VAR`** : *byte *(const)*
- **`USER_STD_SUFFIX`** : *byte *(const)*

### `"windows64"`

- **`HOME_ENV_VAR`** : *byte *(const)*
- **`USER_STD_SUFFIX`** : *byte *(const)*

