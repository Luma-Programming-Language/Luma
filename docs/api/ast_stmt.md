# Module: ast_stmt

*Source: `src/ast/stmt.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `make_program`

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

```luma
pub #returns_ownership
make_expr_stmt -> fn(
    expression: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_var_decl`

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

```luma
pub #returns_ownership
make_return_stmt -> fn(
    value: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_block`

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

```luma
pub #returns_ownership
make_break_continue -> fn(
    is_continue: i64,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_defer_stmt`

```luma
pub #returns_ownership
make_defer_stmt -> fn(
    statement: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

### `make_switch_stmt`

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

```luma
pub #returns_ownership
make_default_stmt -> fn(
    body: *AST::AstNode,
    line: i64,
    col: i64
) *AST::AstNode
```

