# Module: ast_print

*Source: `src/ast/ast_print.lx`*

Pretty-printer for the AST: renders any `*AST::AstNode` as an indented,
colorized tree via `print_node`. The `pn` dispatcher handles every
`NodeType` and, when `USE_COLOR` is off, emits plain text through the
no-color escape constants.

## Table of Contents

- [Functions](#functions)
- [Variables](#variables)


## Functions

### `print_node`

Public entry point: prints `node` as an indented tree, starting each line
at `indent`.

```luma
pub print_node -> fn(
    node: *AST::AstNode,
    indent: i64
) void
```

### `pf`

Prints the indentation prefix for a line at `depth`, emitting a vertical
pipe for each ancestor level whose `flags[i]` says more siblings follow.

```luma
      pf -> fn(
    flags: *i64,
    depth: i64
) void
```

### `cn`

Prints the tree connector: `└──` for a last sibling, `├──` otherwise.

```luma
      cn -> fn(
    is_last: i64
) void
```

### `pn`

Prints one node of the tree at `depth`, dispatching on `node.kind` to
render each node's fields and recurse into its children.

```luma
      pn -> fn(
    node: *AST::AstNode,
    flags: *i64,
    depth: i64,
    is_last: i64,
    color: *byte
) void
```

### `r`

Prints a child node using the default cyan color.

```luma
      r -> fn(
    node: *AST::AstNode,
    flags: *i64,
    depth: i64,
    is_last: i64
) void
```

### `rt`

Prints a type node using the blue color.

```luma
      rt -> fn(
    node: *AST::AstNode,
    flags: *i64,
    depth: i64
) void
```

### `field`

Prints a labeled child `field` (e.g. `cond`, `body`) and recurses into
it; `more` adds a connector implying further siblings follow.

```luma
      field -> fn(
    label: *byte,
    node: *AST::AstNode,
    flags: *i64,
    parent_depth: i64,
    more: i64,
    color: *byte
) void
```

### `col`

Returns the ANSI code `c` when colors are on, else its empty no-color
counterpart `c0`.

```luma
      col -> fn(
    c: *byte,
    c0: *byte
) *byte
```

### `ann_lc`

Prints a dim `(line:col)` annotation, but only when colors are enabled.

```luma
      ann_lc -> fn(
    node: *AST::AstNode
) void
```

### `ann_n`

Prints a dim ` (count word)` annotation (e.g. ` (3 stmts)`), but only
when colors are enabled.

```luma
      ann_n -> fn(
    cnt: i64,
    word: *byte
) void
```

### `pn`

Implements `pn`: prints one tree line for `node` (rendering `<null>` for a
null node and `Unknown(kind)` for unhandled kinds) and recurses into its
children.

```luma
      pn -> fn(
    node: *AST::AstNode,
    flags: *i64,
    depth: i64,
    is_last: i64,
    color: *byte
) void
```

### `print_node`

Implements `print_node`: allocates the depth-flag array, walks the tree
from `node`, and frees the flags when done.

```luma
pub print_node -> fn(
    node: *AST::AstNode,
    indent: i64
) void
```


## Variables

- **`USE_COLOR`** : i64 *(const)* — When 1 (default), ANSI escape sequences are emitted; the plain `_*0`
- **`_C`** : *byte *(const)* — ANSI cyan escape sequence.
- **`_G`** : *byte *(const)* — ANSI green escape sequence.
- **`_Y`** : *byte *(const)* — ANSI yellow escape sequence.
- **`_M`** : *byte *(const)* — ANSI magenta escape sequence.
- **`_B`** : *byte *(const)* — ANSI blue escape sequence.
- **`_D`** : *byte *(const)* — ANSI dim escape sequence.
- **`_R`** : *byte *(const)* — ANSI reset escape sequence.
- **`_C0`** : *byte *(const)* — Empty (no-color) counterpart of `_C`.
- **`_G0`** : *byte *(const)* — Empty (no-color) counterpart of `_G`.
- **`_Y0`** : *byte *(const)* — Empty (no-color) counterpart of `_Y`.
- **`_M0`** : *byte *(const)* — Empty (no-color) counterpart of `_M`.
- **`_B0`** : *byte *(const)* — Empty (no-color) counterpart of `_B`.
- **`_D0`** : *byte *(const)* — Empty (no-color) counterpart of `_D`.
- **`_R0`** : *byte *(const)* — Empty (no-color) counterpart of `_R`.
- **`MAX_DEPTH`** : i64 *(const)* — Maximum recursion depth supported by the tree printer's flag array.
