# Module: ast

*Source: `src/ast/ast.lx`*

## Table of Contents

- [Structures](#structures)
- [Enumerations](#enumerations)
- [Functions](#functions)

---

## Structures

### `AstNode`

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

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `module_name` | *byte |  |
| `alias` | *byte |  |

### `OsNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `platforms` | **byte |  |
| `bodies` | **AstNode |  |
| `has_default` | i64 |  |
| `default_body` | *AstNode |  |

### `LinkNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `lib_name` | *byte |  |

### `LiteralNode`

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

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |

### `BinaryNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `op` | i64 |  |
| `left` | *AstNode |  |
| `right` | *AstNode |  |

### `UnaryNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `op` | i64 |  |
| `operand` | *AstNode |  |

### `CallNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `callee` | *AstNode |  |
| `args` | **AstNode |  |
| `arg_count` | i64 |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |

### `AssignNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `target` | *AstNode |  |
| `value` | *AstNode |  |

### `TernaryNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `then_expr` | *AstNode |  |
| `else_expr` | *AstNode |  |

### `MemberNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `is_compiletime` | i64 |  |
| `object` | *AstNode |  |
| `member` | *byte |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |

### `IndexNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `object` | *AstNode |  |
| `index` | *AstNode |  |

### `ArrayNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `elements` | **AstNode |  |
| `element_count` | i64 |  |
| `target_size` | i64 |  |

### `CastNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `type_node` | *AstNode |  |
| `castee` | *AstNode |  |

### `AllocNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `size` | *AstNode |  |

### `FreeNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `ptr` | *AstNode |  |

### `MemcpyNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `to` | *AstNode |  |
| `from` | *AstNode |  |
| `size` | *AstNode |  |

### `SizeofNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `object` | *AstNode |  |
| `is_type` | i64 |  |

### `SyscallNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `args` | **AstNode |  |
| `count` | i64 |  |

### `StructExprNode`

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

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `modules` | **AstNode |  |
| `module_count` | i64 |  |

### `ExprStmtNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `expression` | *AstNode |  |

### `VarDeclNode`

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

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `then_stmt` | *AstNode |  |
| `elif_stmts` | **AstNode |  |
| `elif_count` | i64 |  |
| `else_stmt` | *AstNode |  |

### `LoopStmtNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `optional` | *AstNode |  |
| `body` | *AstNode |  |
| `initializer` | **AstNode |  |
| `init_count` | i64 |  |

### `ReturnStmtNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `value` | *AstNode |  |

### `BlockNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `statements` | **AstNode |  |
| `stmt_count` | i64 |  |

### `PrintStmtNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `expressions` | **AstNode |  |
| `expr_count` | i64 |  |
| `ln` | i64 |  |

### `BreakContinueNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `is_continue` | i64 |  |

### `DeferNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `statement` | *AstNode |  |

### `SwitchNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `condition` | *AstNode |  |
| `cases` | **AstNode |  |
| `case_count` | i64 |  |
| `default_case` | *AstNode |  |

### `ImplNode`

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

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `values` | **AstNode |  |
| `value_count` | i64 |  |
| `body` | *AstNode |  |

### `DefaultNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `body` | *AstNode |  |

### `BasicTypeNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `name` | *byte |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |

### `PointerTypeNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `pointee_type` | *AstNode |  |

### `ArrayTypeNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `element_type` | *AstNode |  |
| `size` | *AstNode |  |

### `FuncTypeNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `param_types` | **AstNode |  |
| `param_count` | i64 |  |
| `return_type` | *AstNode |  |

### `ResolutionNode`

| Field | Type | Description |
|-------|------|-------------|
| `AstNode` | AstNode |  |
| `parts` | **byte |  |
| `part_count` | i64 |  |
| `type_args` | **AstNode |  |
| `type_arg_count` | i64 |  |


## Enumerations

### pub `NodeType`

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

**Values:**

- `Node_Category_EXPR`
- `Node_Category_STMT`
- `Node_Category_TYPE`
- `Node_Category_PREPROCESSOR`


## Functions

### `free_node`

```luma
pub #takes_ownership
free_node -> fn(
    node: *AstNode
) void
```

### `clone_node`

```luma
pub #returns_ownership
clone_node -> fn(
    node: *AstNode
) *AstNode
```

### `clone_node_substituting`

```luma
pub #returns_ownership
clone_node_substituting -> fn(
    node: *AstNode,
    names: **byte,
    types: **AstNode,
    count: i64
) *AstNode
```

### `clone_node`

```luma
pub #returns_ownership
clone_node -> fn(
    node: *AstNode
) *AstNode
```

