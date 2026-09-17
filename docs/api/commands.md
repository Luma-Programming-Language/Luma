# Module: commands

*Source: `src/commands.lx`*

## Table of Contents

- [Functions](#functions)
- [OS-Specific](#os-specific)


## Functions

### `print_commands`

```luma
pub print_commands -> fn(
) void
```

### `print_license`

```luma
pub print_license -> fn(
) void
```

### `match_command`

```luma
pub match_command -> fn(
    cmd: *byte,
    usr_cmd: [*byte; 10],
    count: i64
) i64
```

### `parse_args`

```luma
pub parse_args -> fn(
    args: *ARGS::Args,
    build_config: *CONST::LumaBuildConfig
) i64
```

### `resolve_path`

```luma
pub #returns_ownership
resolve_path -> fn(
    path: *byte
) *byte
```


## OS-Specific

### `"linux"`


### `"macos"`


### `"windows32"`


### `"windows64"`


### `"linux"`


### `"macos"`


### `"windows32"`


### `"windows64"`


