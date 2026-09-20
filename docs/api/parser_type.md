# Module: parser_type

*Source: `src/parser/type.lx`*

Parses type syntax - primitive, pointer, array, resolution and function
types, plus generic type-argument lists - into AST type nodes.

## Table of Contents

- [Functions](#functions)


## Functions

### `pointer_type`

```luma
      #returns_ownership
pointer_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `array_type`

```luma
      #returns_ownership
array_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `resolution_type`

```luma
      #returns_ownership
resolution_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `function_type`

```luma
      #returns_ownership
function_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `make_simple_type`

Parses a primitive/basic type name from the current token into a
BasicTypeNode.

```luma
      make_simple_type -> fn(
    p: *PARS::Parser,
    line: i64,
    col: i64
) *AST::AstNode
```

### `tnud`

Token-nud type parser: dispatches on the current token to the matching
simple, pointer, array, function or resolution type parser.

```luma
      #returns_ownership
tnud -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `parse_type`

Public entry point: parses a type at the current token position and
returns an AST type node, or NULL if the token cannot start a type.

```luma
pub #returns_ownership
parse_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `array_type`

Parses a sized array type `[elem_type; size]` (size is an integer literal
or identifier) into an ArrayTypeNode.

```luma
      #returns_ownership
array_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `pointer_type`

Parses a `*type` pointer type into a PointerTypeNode.

```luma
      #returns_ownership
pointer_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `parse_type_arg_list`

Parses a `<Type (',' Type)*>` type-argument list after a caller-confirmed
'<' token and returns the parsed type nodes in a vector. In type position
the '<' is unambiguous, so no speculative parsing is needed.

```luma
pub #returns_ownership
parse_type_arg_list -> fn(
    p: *PARS::Parser
) VEC::Vector
```

### `resolution_type`

Parses a (possibly qualified `A::B::`) type reference, attaching generic
type arguments when a '<' follows the last name segment.

```luma
      #returns_ownership
resolution_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `function_type`

Parses a function type: a parenthesised parameter-type list followed by a
return type, into a FuncTypeNode.

```luma
      #returns_ownership
function_type -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

