# Module: codegen

*Source: `src/codegen/codegen.lx`*

## Table of Contents

- [Functions](#functions)


## Functions

### `codegen_transpile`

```luma
pub #returns_ownership
codegen_transpile -> fn(
    program: *AST::AstNode,
    config: CONST::LumaBuildConfig
) *byte
```

### `codegen_compile`

```luma
pub #takes_ownership
codegen_compile -> fn(
    c_path: *byte,
    config: CONST::LumaBuildConfig
) bool
```

