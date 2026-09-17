# Module: parse_stmt

*Source: `src/parser/stmt.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `parse_stmt`

```luma
pub #returns_ownership
parse_stmt -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `consume_doc_comments`

```luma
pub #returns_ownership
consume_doc_comments -> fn(
    p: *PARS::Parser
) *byte
```

