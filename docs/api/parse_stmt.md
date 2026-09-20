# Module: parse_stmt

*Source: `src/parser/stmt.lx`*

Parses Luma statements into AST nodes.

Dispatches top-level and block statements by keyword: const/var/use/return
declarations, `if`/`loop`/`switch`/`defer`, `@os`, `@link`, `@use`, and
expression statements. Also handles doc comments, `pub`/`priv` visibility,
and the `#returns_ownership` / `#takes_ownership` / `#dll_import` /
`#lib_import` attributes.

## Table of Contents

- [Functions](#functions)


## Functions

### `_const`

```luma
      #returns_ownership
_const -> fn(
    p: *PARS::Parser,
    is_public: i64,
    returns_ownership: i64,
    takes_ownership: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `_fn`

```luma
      #returns_ownership
_fn -> fn(
    p: *PARS::Parser,
    name: *byte,
    is_public: i64,
    returns_ownership: i64,
    takes_ownership: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `break_continue_stmt`

```luma
      #returns_ownership
break_continue_stmt -> fn(
    p: *PARS::Parser,
    is_continue: i64
) *AST::AstNode
```

### `_enum`

```luma
      #returns_ownership
_enum -> fn(
    p: *PARS::Parser,
    name: *byte,
    is_public: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `_struct`

```luma
      #returns_ownership
_struct -> fn(
    p: *PARS::Parser,
    name: *byte,
    is_public: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `parse_type_param_list`

```luma
      #returns_ownership
parse_type_param_list -> fn(
    p: *PARS::Parser
) VEC::Vector
```

### `var`

```luma
      #returns_ownership
var -> fn(
    p: *PARS::Parser,
    is_public: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `infinite_loop_stmt`

```luma
      #returns_ownership
infinite_loop_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `for_loop_stmt`

```luma
      #returns_ownership
for_loop_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `print_stmt`

```luma
      #returns_ownership
print_stmt -> fn(
    p: *PARS::Parser,
    ln: i64
) *AST::AstNode
```

### `expr`

```luma
      #returns_ownership
expr -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `use`

```luma
      #returns_ownership
use -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `os_stmt`

```luma
      #returns_ownership
os_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `link_stmt`

```luma
      #returns_ownership
link_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `consume_doc_comments`

```luma
      #returns_ownership
consume_doc_comments -> fn(
    p: *PARS::Parser
) *byte
```

### `apply_dll_import`

```luma
      apply_dll_import -> fn(
    node: *AST::AstNode,
    dll_name: *byte,
    dll_callconv: *byte
) void
```

### `apply_lib_import`

```luma
      apply_lib_import -> fn(
    node: *AST::AstNode,
    lib_name: *byte
) void
```

### `return_stmt`

```luma
      #returns_ownership
return_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `block`

```luma
      #returns_ownership
block -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_if`

```luma
      #returns_ownership
_if -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_defer`

```luma
      #returns_ownership
_defer -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_switch`

```luma
      #returns_ownership
_switch -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `parse_case_value`

```luma
      #returns_ownership
parse_case_value -> fn(
    p: *PARS::Parser,
    using_path: *VEC::Vector
) *AST::AstNode
```

### `scoped_member`

```luma
      #returns_ownership
scoped_member -> fn(
    using_path: *VEC::Vector,
    member: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `free_using_path`

```luma
      free_using_path -> fn(
    using_path: *VEC::Vector
) void
```

### `_impl`

```luma
      #returns_ownership
_impl -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_loop`

```luma
      #returns_ownership
_loop -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `parse_stmt`

Parses a single statement, collecting the leading doc comment, ownership
and import attributes, and visibility, then dispatching on the keyword.
Applies `#dll_import`/`#lib_import` to the resulting function node.


```luma
pub #returns_ownership
parse_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

**Returns:**
The statement node, or null after a lexical/parse error.

### `consume_doc_comments`

Consumes consecutive doc-comment tokens (`///` and `//!`), returning their
newline-joined text, or null when the current token is not a doc comment.

```luma
pub #returns_ownership
consume_doc_comments -> fn(
    p: *PARS::Parser
) *byte
```

### `apply_dll_import`

Marks `node` (a function declaration) as a DLL import, storing the DLL name
and optional calling convention.

```luma
      apply_dll_import -> fn(
    node: *AST::AstNode,
    dll_name: *byte,
    dll_callconv: *byte
) void
```

### `apply_lib_import`

Marks `node` (a function declaration) as a library import, storing the
library name.

```luma
      apply_lib_import -> fn(
    node: *AST::AstNode,
    lib_name: *byte
) void
```

### `os_stmt`

Parses `@os { "platform" -> stmt, _ -> stmt, ... }`, building an OS node
with per-platform bodies and an optional default arm.

```luma
      #returns_ownership
os_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `link_stmt`

Parses `@link("name")` into a link directive node.

```luma
      #returns_ownership
link_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_const`

Parses a `const` declaration: either a typed constant
(`const name: Type = value;`) or the start of a function, struct, or enum
declaration, delegated to `_fn`/`_struct`/`_enum`.

```luma
      #returns_ownership
_const -> fn(
    p: *PARS::Parser,
    is_public: i64,
    returns_ownership: i64,
    takes_ownership: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `parse_type_param_list`

```luma
      #returns_ownership
parse_type_param_list -> fn(
    p: *PARS::Parser
) VEC::Vector
```

### `_fn`

Parses a function declaration (optionally generic), producing a
function-declaration node. A trailing `;` yields a forward declaration
without a body.

```luma
      #returns_ownership
_fn -> fn(
    p: *PARS::Parser,
    name: *byte,
    is_public: i64,
    returns_ownership: i64,
    takes_ownership: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `_enum`

Parses an enum declaration (optionally generic; parameters act as phantom
type tags), producing an enum-declaration node.

```luma
      #returns_ownership
_enum -> fn(
    p: *PARS::Parser,
    name: *byte,
    is_public: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `_struct`

Parses a struct declaration (optionally generic), producing a
struct-declaration node. Handles `pub:`/`priv:` sections, `...Embedded`
members, static and ownership markers, and method members.

```luma
      #returns_ownership
_struct -> fn(
    p: *PARS::Parser,
    name: *byte,
    is_public: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `print`

Stub; always returns null.

```luma
      #returns_ownership
print -> fn(
    p: *PARS::Parser,
    ln: i64
) *AST::AstNode
```

### `var`

Parses `var name: Type` with an optional `= value;` initializer into a
variable-declaration node.

```luma
      #returns_ownership
var -> fn(
    p: *PARS::Parser,
    is_public: i64,
    doc_comment: *byte
) *AST::AstNode
```

### `expr`

Parses an expression statement followed by `;`.

```luma
      #returns_ownership
expr -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `use`

Parses `@use "module" as alias` into a use-declaration node.

```luma
      #returns_ownership
use -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `return_stmt`

Parses a `return;` or `return expr;` statement.

```luma
      #returns_ownership
return_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `block`

Parses a `{ ... }` block of statements into a block node.

```luma
      #returns_ownership
block -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_if`

Parses an `if (cond) ...` statement with optional `elif` chain and `else`
branch into an if node.

```luma
      #returns_ownership
_if -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `break_continue_stmt`

Parses a `break;` or `continue;` statement.

```luma
      #returns_ownership
break_continue_stmt -> fn(
    p: *PARS::Parser,
    is_continue: i64
) *AST::AstNode
```

### `_defer`

Parses a `defer <stmt>` statement.

```luma
      #returns_ownership
_defer -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_switch`

Parses a `switch(cond) { ... }` statement, supporting an optional
`using Path::To::Enum` label namespace, multi-value case labels, and a
default case.

```luma
      #returns_ownership
_switch -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `parse_case_value`

Parses one switch case-label expression. A bare identifier under a `using`
path is desugared to `<path>::<identifier>`.

```luma
      #returns_ownership
parse_case_value -> fn(
    p: *PARS::Parser,
    using_path: *VEC::Vector
) *AST::AstNode
```

### `scoped_member`

Builds `<path[0]>::<path[1]>::...::<member>` as a chain of compiletime
member-access nodes, duplicating each segment string.

```luma
      #returns_ownership
scoped_member -> fn(
    using_path: *VEC::Vector,
    member: *byte,
    line: i64,
    col: i64
) *AST::AstNode
```

### `free_using_path`

Frees every segment string in `using_path` and the vector itself.

```luma
      free_using_path -> fn(
    using_path: *VEC::Vector
) void
```

### `_impl`

Stub; always returns null.

```luma
      #returns_ownership
_impl -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `loop_init`

Parses a loop initializer `name: Type = expr` inside a `loop [ ... ]`
header into a variable-declaration node.

```luma
      #returns_ownership
loop_init -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `for_loop_stmt`

Parses `loop [init, ...] (condition) : (post) { body }` — a loop with a
header — into a loop node.

```luma
      #returns_ownership
for_loop_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `infinite_loop_stmt`

Parses `loop { body }`, an infinite loop with no condition, into a loop
node.

```luma
      #returns_ownership
infinite_loop_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_loop`

Parses a `loop` statement: dispatch to `infinite_loop_stmt` for `loop { }`,
`for_loop_stmt` for `loop [ ... ]`, otherwise `loop (cond) : (post) { }`.

```luma
      #returns_ownership
_loop -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

