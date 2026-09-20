# Module: ast_stmt

*Source: `src/ast/stmt.lx`*

Constructors for the statement AST nodes: the program root, declarations
(var, const, function, struct, enum, field), and all control-flow
statements. Each returns a freshly allocated node boxed as an
`*AST::AstNode`.

## Table of Contents

- [Functions](#functions)


## Functions

### `make_program`

Builds a `PROGRAM` node holding the compilation unit's list of modules.

```luma
pub #returns_ownership
make_program -> fn(
    modules: **AST::AstNode,
    module_count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_expr_stmt`

Builds a `STMT_EXPRESSION` node wrapping a single expression statement.

```luma
pub #returns_ownership
make_expr_stmt -> fn(
    expression: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_var_decl`

Builds a `STMT_VAR_DECL` node for a mutable or immutable `let`/`var`
declaration: name, doc comment, optional type and initializer, and the
`is_mutable`/`is_public` flags.

```luma
pub #returns_ownership
make_var_decl -> fn(
    name: *byte,
    doc_comment: *byte,
    var_type: *AST::AstNode,
    initializer: *AST::AstNode,
    is_mutable: i64,
    is_public: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_const_decl`

Builds a `STMT_CONST_DECL` node for an immutable `const` declaration
(`is_mutable` is forced to 0).

```luma
pub #returns_ownership
make_const_decl -> fn(
    name: *byte,
    doc_comment: *byte,
    var_type: *AST::AstNode,
    initializer: *AST::AstNode,
    is_public: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_func_decl`

Builds a `STMT_FUNCTION` node describing a function declaration: name, doc
comment, parameter names/types (`param_count` entries), return type,
visibility, ownership flags, body, and whether it is forward declared.

```luma
pub #returns_ownership
make_func_decl -> fn(
    name: *byte,
    doc_comment: *byte,
    param_names: **byte,
    param_types: **AST::AstNode,
    param_count: i64,
    return_type: *AST::AstNode,
    is_public: i64,
    body: *AST::AstNode,
    returns_ownership: i64,
    takes_ownership: i64,
    forward_declared: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_struct_decl`

Builds a `STMT_STRUCT` node holding the struct's name, doc comment,
separate public/private member lists, and `is_public` visibility.

```luma
pub #returns_ownership
make_struct_decl -> fn(
    name: *byte,
    doc_comment: *byte,
    public_members: **AST::AstNode,
    public_count: i64,
    private_members: **AST::AstNode,
    private_count: i64,
    is_public: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_field_decl`

Builds a `STMT_FIELD_DECL` node for one struct member: a data field with
`type_node`, a method with a `function` node, or an embedded type.

```luma
pub #returns_ownership
make_field_decl -> fn(
    name: *byte,
    doc_comment: *byte,
    type_node: *AST::AstNode,
    function: *AST::AstNode,
    is_public: i64,
    is_embedded: i64,
    is_static: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_enum_decl`

Builds a `STMT_ENUM` node with the enum's name, doc comment, member name
list (`member_count` entries), and `is_public` visibility.

```luma
pub #returns_ownership
make_enum_decl -> fn(
    name: *byte,
    doc_comment: *byte,
    members: **byte,
    member_count: i64,
    is_public: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_if_stmt`

Builds a `STMT_IF` node from the `condition`, `then_stmt`, an `elif_stmts`
list (`elif_count` entries), and an optional `else_stmt`.

```luma
pub #returns_ownership
make_if_stmt -> fn(
    condition: *AST::AstNode,
    then_stmt: *AST::AstNode,
    elif_stmts: **AST::AstNode,
    elif_count: i64,
    else_stmt: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_loop_stmt`

Builds a `STMT_LOOP` node from the `condition`, optional step clause,
loop `body`, and an `initializer` list (`init_count` entries).

```luma
pub #returns_ownership
make_loop_stmt -> fn(
    condition: *AST::AstNode,
    optional: *AST::AstNode,
    body: *AST::AstNode,
    initializer: **AST::AstNode,
    init_count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_return_stmt`

Builds a `STMT_RETURN` node wrapping the returned expression `value`.

```luma
pub #returns_ownership
make_return_stmt -> fn(
    value: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_block`

Builds a `STMT_BLOCK` node from a list of `statements` (`stmt_count`).

```luma
pub #returns_ownership
make_block -> fn(
    statements: **AST::AstNode,
    stmt_count: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_print_stmt`

Builds a `STMT_PRINT` node for `print(...)`: a list of `expressions`
(`expr_count`), with `ln` selecting the newline-emitting form.

```luma
pub #returns_ownership
make_print_stmt -> fn(
    expressions: **AST::AstNode,
    expr_count: i64,
    ln: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_break_continue`

Builds a `STMT_BREAK_CONTINUE` node; `is_continue` selects `continue`
(1) over `break` (0).

```luma
pub #returns_ownership
make_break_continue -> fn(
    is_continue: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_defer_stmt`

Builds a `STMT_DEFER` node wrapping the deferred `statement`.

```luma
pub #returns_ownership
make_defer_stmt -> fn(
    statement: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_switch_stmt`

Builds a `STMT_SWITCH` node from the `condition`, a `cases` list
(`case_count`), and an optional `default_case`.

```luma
pub #returns_ownership
make_switch_stmt -> fn(
    condition: *AST::AstNode,
    cases: **AST::AstNode,
    case_count: i64,
    default_case: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_impl_stmt`

Builds a `STMT_IMPL` node connecting function names/types to struct names
with a body: `impl fn1, fn2 for Struct1, Struct2 { ... }`.

```luma
pub #returns_ownership
make_impl_stmt -> fn(
    function_name_list: **byte,
    function_type_list: **AST::AstNode,
    struct_name_list: **byte,
    function_name_count: i64,
    struct_name_count: i64,
    body: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_case_stmt`

Builds a `STMT_CASE` node for one switch case: the match `values` list
(`value_count`) and its `body`.

```luma
pub #returns_ownership
make_case_stmt -> fn(
    values: **AST::AstNode,
    value_count: i64,
    body: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_default_stmt`

Builds a `STMT_DEFAULT` node holding a switch default case's `body`.

```luma
pub #returns_ownership
make_default_stmt -> fn(
    body: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

