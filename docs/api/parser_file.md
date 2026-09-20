# Module: parser_file

*Source: `src/parser/file.lx`*

Entry point for parsing a whole source file: reads and lexes the file,
re-issues lexical errors as full diagnostics, and builds the module AST
node that represents the file.

## Table of Contents

- [Functions](#functions)


## Functions

### `report_read_error`

Reports a "could not read file" failure as a queued IO-error diagnostic
instead of a bare stdout print.

```luma
      #takes_ownership
report_read_error -> fn(
    path: *byte,
    message: *byte
) void
```

### `post_lex_errors`

Re-issues each error buffered by the lexer as a full error.lx diagnostic,
now that the token table and file path are available to render context.

```luma
      post_lex_errors -> fn(
    lexer: *TOK::Lexer,
    tks: *TOK::Token,
    tok_count: i64,
    file_path: *byte
) void
```

### `fail`

Frees the tokens, statements, source buffer and parser, then returns NULL
so callers can propagate parse failure.

```luma
      #takes_ownership
fail -> fn(
    tokens: *VEC::Vector,
    stmts: *VEC::Vector,
    src: *byte,
    p: *PARS::Parser
) *AST::AstNode
```

### `fail_early`

Frees a source buffer and returns NULL, used when failure occurs before
any other parse state has been allocated.

```luma
      #takes_ownership
fail_early -> fn(
    src: *byte
) *AST::AstNode
```

### `parse_file`

Reads, lexes and parses the file at `path`, returning the module `AstNode`
for the whole file (the caller owns it), or NULL on any failure.

```luma
pub #returns_ownership
parse_file -> fn(
    path: *byte
) *AST::AstNode
```

