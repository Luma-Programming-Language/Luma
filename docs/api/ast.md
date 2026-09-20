# Module: ast

*Source: `src/ast/ast.lx`*

Core AST definitions: the `AstNode` base struct, the `NodeType`,
`NodeCategory` and operator/literal enums, and one concrete node struct per
node kind (preprocessor, expression, statement, and type). Also provides
`free_node` (recursive teardown) and `clone_node` (deep copy used for
generic instantiation).

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `AstNode`

Base of every AST node: a `kind`/`category` pair plus source location.
All concrete node structs embed `...AstNode` as their first member.

| Field | Type | Description |
|-------|------|-------------|
| `kind` | i64 |  |
| `category` | i64 |  |
| `line` | i64 |  |
| `col` | i64 |  |
| `resolved_type` | *AstNode |  |

**Methods:**

#### `AstNode::make()`

```luma
static #returns_ownership
AstNode::make<T> -> fn(
    kind: NodeType,
    category: NodeCategory,
    line: i64,
    col: i64
) *T
```

### `ModuleNode`

`@module` node: module name, doc comment, file path, body statement list,
and the module's source text and token stream (retained for diagnostics).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `doc_comment` | *byte |  |
| `file_path` | *byte |  |
| `body` | **AstNode |  |
| `body_count` | i64 |  |
| `source` | *byte |  |
| `tokens` | *TOK::Token |  |
| `token_count` | i64 |  |

### `UseNode`

`@use` import node: the imported module name and its local alias.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `module_name` | *byte |  |
| `alias` | *byte |  |

### `OsNode`

`@os` conditional section: one body per platform plus an optional default
body used when no platform matches.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `platforms` | **byte |  |
| `bodies` | **AstNode |  |
| `has_default` | i64 |  |
| `default_body` | *AstNode |  |

### `LinkNode`

`@link` node naming a library to link against.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `lib_name` | *byte |  |

### `LiteralNode`

Expression node for a literal value: one of int, float, double, string,
char, bool, or null (selected by `lit_type`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `lit_type` | i64 |  |
| `int_val` | i64 |  |
| `float_val` | i64 |  |
| `string_val` | *byte |  |
| `char_val` | byte |  |
| `bool_val` | i64 |  |

### `IdentifierNode`

Expression node naming a symbol (`name`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |

### `BinaryNode`

Expression node for a binary operation (`left op right`); also used for
range expressions (`EXPR_RANGE`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `op` | i64 |  |
| `left` | *AstNode |  |
| `right` | *AstNode |  |

### `UnaryNode`

Expression node for a unary operation applied to `operand`; also reused
for deref, address-of, grouping, and system-expression nodes.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `op` | i64 |  |
| `operand` | *AstNode |  |

### `CallNode`

Expression node for a function call: the `callee`, the `args` list
(`arg_count`), and optional explicit generic `type_args`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `callee` | *AstNode |  |
| `args` | **AstNode |  |
| `arg_count` | i64 |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |

### `AssignNode`

Expression node for an assignment (`target = value`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `target` | *AstNode |  |
| `value` | *AstNode |  |

### `TernaryNode`

Expression node for a ternary `condition ? then_expr : else_expr`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `then_expr` | *AstNode |  |
| `else_expr` | *AstNode |  |

### `MemberNode`

Expression node for a member access: `object.member` (`.`), or a compile
time / qualified `object::member` (`is_compiletime`), with optional
explicit generic `type_args`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `is_compiletime` | i64 |  |
| `object` | *AstNode |  |
| `member` | *byte |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |

### `IndexNode`

Expression node for an index access (`object[index]`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `object` | *AstNode |  |
| `index` | *AstNode |  |

### `ArrayNode`

Expression node for an array literal: the `elements` list
(`element_count`) and an optional fixed `target_size`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `elements` | **AstNode |  |
| `element_count` | i64 |  |
| `target_size` | i64 |  |

### `CastNode`

Expression node for a cast (`cast<T>(expr)`); also reused for `input`
expressions (`EXPR_INPUT`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `type_node` | *AstNode |  |
| `castee` | *AstNode |  |

### `AllocNode`

Expression node for `alloc(size)`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `size` | *AstNode |  |

### `FreeNode`

Expression node for `free(ptr)`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `ptr` | *AstNode |  |

### `MemcpyNode`

Expression node for `memcpy(to, from, size)`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `to` | *AstNode |  |
| `from` | *AstNode |  |
| `size` | *AstNode |  |

### `SizeofNode`

Expression node for `sizeof(...)`, taking the size of an object or a type
(`is_type`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `object` | *AstNode |  |
| `is_type` | i64 |  |

### `SyscallNode`

Expression node for an inline syscall with a raw argument list.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `args` | **AstNode |  |
| `count` | i64 |  |

### `StructExprNode`

Expression node for a struct literal `Name { field: value, ... }`, optionally
qualified as `Alias::Name`, with optional explicit generic `type_args`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `alias` | *byte |  |
| `field_names` | **byte |  |
| `field_vals` | **AstNode |  |
| `field_count` | i64 |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |

### `ProgramNode`

Root node of the whole program: the list of parsed `modules`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `modules` | **AstNode |  |
| `module_count` | i64 |  |

### `ExprStmtNode`

Statement node wrapping a single expression statement.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `expression` | *AstNode |  |

### `VarDeclNode`

Statement node for a `let`/`var`/`const` declaration: name, doc comment,
optional type and initializer, and the `is_mutable`/`is_public` flags
(const declarations store `is_mutable` = 0).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `doc_comment` | *byte |  |
| `var_type` | *AstNode |  |
| `initializer` | *AstNode |  |
| `is_mutable` | i64 |  |
| `is_public` | i64 |  |

### `FuncDeclNode`

Statement node for a function declaration: name, doc comment, parameter
names/types (`param_count`), return type, visibility, ownership flags,
body, DLL/lib import markers, and optional generic `type_params`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `doc_comment` | *byte |  |
| `param_names` | **byte |  |
| `param_types` | **AstNode |  |
| `param_count` | i64 |  |
| `return_type` | *AstNode |  |
| `is_public` | i64 |  |
| `body` | *AstNode |  |
| `returns_ownership` | i64 |  |
| `takes_ownership` | i64 |  |
| `forward_declared` | i64 |  |
| `dll_import` | i64 |  |
| `dll_name` | *byte |  |
| `dll_callconv` | *byte |  |
| `lib_import` | i64 |  |
| `lib_name` | *byte |  |
| `type_params` | **byte |  |
| `type_param_count` | i64 |  |

### `StructDeclNode`

Statement node for a struct declaration: separate public/private member
lists and optional generic `type_params`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `doc_comment` | *byte |  |
| `public_members` | **AstNode |  |
| `public_count` | i64 |  |
| `private_members` | **AstNode |  |
| `private_count` | i64 |  |
| `is_public` | i64 |  |
| `type_params` | **byte |  |
| `type_param_count` | i64 |  |

### `FieldDeclNode`

Statement node for one struct member: a data field with a `type_node`, a
method carried in `function`, or an embedded type (`is_embedded`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `doc_comment` | *byte |  |
| `type_node` | *AstNode |  |
| `function` | *AstNode |  |
| `is_public` | i64 |  |
| `is_embedded` | i64 |  |
| `is_static` | i64 |  |

### `EnumDeclNode`

Statement node for a C-style enum declaration: name, doc comment, member
name list (`member_count`), and optional generic `type_params`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `doc_comment` | *byte |  |
| `members` | **byte |  |
| `member_count` | i64 |  |
| `is_public` | i64 |  |
| `type_params` | **byte |  |
| `type_param_count` | i64 |  |

### `IfStmtNode`

Statement node for an `if` with a condition, then-branch, `elif` list,
and optional else-branch.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `then_stmt` | *AstNode |  |
| `elif_stmts` | **AstNode |  |
| `elif_count` | i64 |  |
| `else_stmt` | *AstNode |  |

### `LoopStmtNode`

Statement node for a `loop` with an optional init list, condition step,
and body.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `optional` | *AstNode |  |
| `body` | *AstNode |  |
| `initializer` | **AstNode |  |
| `init_count` | i64 |  |

### `ReturnStmtNode`

Statement node for `return value` (value may be null for bare returns).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `value` | *AstNode |  |

### `BlockNode`

Statement node for a brace-delimited block of `statements`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `statements` | **AstNode |  |
| `stmt_count` | i64 |  |

### `PrintStmtNode`

Statement node for `print(...)`: the expression list (`expr_count`) and
the `ln` flag selecting the newline-emitting form.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `expressions` | **AstNode |  |
| `expr_count` | i64 |  |
| `ln` | i64 |  |

### `BreakContinueNode`

Statement node for `break` or `continue` (`is_continue` selects which).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `is_continue` | i64 |  |

### `DeferNode`

Statement node for `defer statement`, run when the enclosing scope exits.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `statement` | *AstNode |  |

### `SwitchNode`

Statement node for a `switch` on `condition`, with a `cases` list and an
optional `default_case`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `cases` | **AstNode |  |
| `case_count` | i64 |  |
| `default_case` | *AstNode |  |

### `ImplNode`

Statement node for `impl`: pairs function names/types with the struct
names they extend, plus a body.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `function_name_list` | **byte |  |
| `function_type_list` | **AstNode |  |
| `struct_name_list` | **byte |  |
| `function_name_count` | i64 |  |
| `struct_name_count` | i64 |  |
| `body` | *AstNode |  |

### `CaseNode`

Statement node for one switch `case`: a list of match `values` and a body.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `values` | **AstNode |  |
| `value_count` | i64 |  |
| `body` | *AstNode |  |

### `DefaultNode`

Statement node for a switch `default` case, holding its body.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `body` | *AstNode |  |

### `BasicTypeNode`

Type node for a named type (`i64`, user structs, ...), with optional
explicit generic `type_args`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |

### `PointerTypeNode`

Type node for a pointer type (`*pointee_type`).

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `pointee_type` | *AstNode |  |

### `ArrayTypeNode`

Type node for an array of `element_type` sized by `size`.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `element_type` | *AstNode |  |
| `size` | *AstNode |  |

### `FuncTypeNode`

Type node for a function type: parameter types plus a return type.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `param_types` | **AstNode |  |
| `param_count` | i64 |  |
| `return_type` | *AstNode |  |

### `ResolutionNode`

Type node for a qualified type reference (e.g. `A::B`), assembled from
`parts`, with optional explicit generic `type_args` on the last part.

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `parts` | **byte |  |
| `part_count` | i64 |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |


## Enumerations

### pub `NodeType`

Discriminates every AST node kind; stored in `AstNode.kind` and matched by
`free_node`, `clone_node`, and the tree printer.

**Values:**

- `PREPROCESSOR_MODULE`
- `PREPROCESSOR_USE`
- `PREPROCESSOR_OS`
- `PREPROCESSOR_LINK`
- `EXPR_LITERAL`
- `EXPR_IDENTIFIER`
- `EXPR_BINARY`
- `EXPR_UNARY`
- `EXPR_CALL`
- `EXPR_ASSIGNMENT`
- `EXPR_TERNARY`
- `EXPR_MEMBER`
- `EXPR_INDEX`
- `EXPR_GROUPING`
- `EXPR_RANGE`
- `EXPR_ARRAY`
- `EXPR_DEREF`
- `EXPR_ADDR`
- `EXPR_ALLOC`
- `EXPR_MEMCPY`
- `EXPR_FREE`
- `EXPR_CAST`
- `EXPR_INPUT`
- `EXPR_SIZEOF`
- `EXPR_SYSTEM`
- `EXPR_SYSCALL`
- `EXPR_STRUCT`
- `PROGRAM`
- `STMT_EXPRESSION`
- `STMT_VAR_DECL`
- `STMT_CONST_DECL`
- `STMT_FUNCTION`
- `STMT_IF`
- `STMT_LOOP`
- `STMT_BREAK_CONTINUE`
- `STMT_RETURN`
- `STMT_BLOCK`
- `STMT_PRINT`
- `STMT_MODULE`
- `STMT_ENUM`
- `STMT_STRUCT`
- `STMT_FIELD_DECL`
- `STMT_DEFER`
- `STMT_SWITCH`
- `STMT_IMPL`
- `STMT_CASE`
- `STMT_DEFAULT`
- `TYPE_RESOLUTION`
- `TYPE_BASIC`
- `TYPE_POINTER`
- `TYPE_ARRAY`
- `TYPE_FUNCTION`
- `TYPE_STRUCT`
- `TYPE_ENUM`

### pub `LiteralType`

Tells which scalar flavor a `LiteralNode` stores (ident, int, float,
double, string, char, bool, null).

**Values:**

- `LITERAL_IDENT`
- `LITERAL_INT`
- `LITERAL_FLOAT`
- `LITERAL_DOUBLE`
- `LITERAL_STRING`
- `LITERAL_CHAR`
- `LITERAL_BOOL`
- `LITERAL_NULL`

### pub `BinaryOp`

Binary operators for `BinaryNode`; stored in `BinaryNode.op`.

**Values:**

- `BINOP_ADD`
- `BINOP_SUB`
- `BINOP_MUL`
- `BINOP_DIV`
- `BINOP_MOD`
- `BINOP_POW`
- `BINOP_EQ`
- `BINOP_NE`
- `BINOP_LT`
- `BINOP_LE`
- `BINOP_GT`
- `BINOP_GE`
- `BINOP_AND`
- `BINOP_OR`
- `BINOP_BIT_AND`
- `BINOP_BIT_OR`
- `BINOP_BIT_XOR`
- `BINOP_SHL`
- `BINOP_SHR`
- `BINOP_RANGE`

### pub `UnaryType`

Unary operators for `UnaryNode`; stored in `UnaryNode.op`.

**Values:**

- `UNOP_NOT`
- `UNOP_NEG`
- `UNOP_POS`
- `UNOP_BIT_NOT`
- `UNOP_PRE_INC`
- `UNOP_PRE_DEC`
- `UNOP_POST_INC`
- `UNOP_POST_DEC`
- `UNOP_DEREF`
- `UNOP_ADDR`

### pub `NodeCategory`

Broad bucket classifying a node as an expression, statement, type, or
preprocessor construct; stored in `AstNode.category`.

**Values:**

- `Node_Category_EXPR`
- `Node_Category_STMT`
- `Node_Category_TYPE`
- `Node_Category_PREPROCESSOR`


## Functions

### `free_node`

Recursively frees an entire AST subtree, dispatching on `node.kind` to
free each concrete node's owned buffers and child nodes. Takes ownership
of `node` and frees the node itself; is null-safe.

```luma
pub #takes_ownership
free_node -> fn(
    node: *AstNode
) void
```

### `clone_node`

Deep-clones an entire AST subtree (used for generic monomorphization),
substituting any active generic type parameters as it goes. Returns a
fresh, independent clone, or null for node kinds that cannot appear in a
generic template.

```luma
pub #returns_ownership
clone_node -> fn(
    node: *AstNode
) *AstNode
```

### `clone_node_substituting`

Deep-clones `node` while replacing every bare type name matching `names[i]`
with a clone of `types[i]` — the entry point used to instantiate a generic
template at one concrete call site. Reentrant: an inner call saves and
restores the outer substitution table.

```luma
pub #returns_ownership
clone_node_substituting -> fn(
    node: *AstNode,
    names: **byte,
    types: **AstNode,
    count: i64
) *AstNode
```

### `dup_str`

Duplicates a null-terminated string (or returns null for a null input).

```luma
      #returns_ownership
dup_str -> fn(
    s: *byte
) *byte
```

### `clone_node_array`

Deep-clones an array of `count` nodes into a fresh allocation.

```luma
      #returns_ownership
clone_node_array -> fn(
    arr: **AstNode,
    count: i64
) **AstNode
```

### `dup_str_array`

Duplicates an array of `count` strings into a fresh allocation.

```luma
      #returns_ownership
dup_str_array -> fn(
    arr: **byte,
    count: i64
) **byte
```

### `clone_node`

Implements `clone_node`: dispatches on `node.kind` to allocate a matching
concrete clone and recursively deep-copy every owned field.

```luma
pub #returns_ownership
clone_node -> fn(
    node: *AstNode
) *AstNode
```


## Variables

- **`g_subst_names`** : **byte *(let)*
- **`g_subst_types`** : **AstNode *(let)*
- **`g_subst_count`** : i64 *(let)*
