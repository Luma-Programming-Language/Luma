# Module: parser_expr

*Source: `src/parser/expr.lx`*

Parses Luma expressions into AST nodes (a Pratt / precedence-climbing parser).

A token `nud`/`led` dispatch table drives parsing of literals, operators,
calls, indexing, member access, struct expressions, and the `__alloc__`,
`__free__`, `__cast__`, `__input__`, `__system__`, `__syscall__`, and
`__sizeof__` builtins.

## Table of Contents

- [Functions](#functions)


## Functions

### `unescape_char`

Resolves a single escape character (the character after a `\`) to its byte
value; unknown escapes pass the character through unchanged.

```luma
      unescape_char -> fn(
    c: byte
) byte
```

### `hex_digit_val`

Returns the value of a hex digit character (`0`-`9`, `a`-`f`, `A`-`F`), or
-1 if `c` is not a hex digit.

```luma
      hex_digit_val -> fn(
    c: byte
) i64
```

### `unescape_str`

Returns a heap-allocated copy of `s` with escape sequences resolved.

Handles `\n`, `\t`, `\r`, `\0`, `\\`, `\'`, `\"`, and `\xHH` hex-byte
escapes; unknown escapes pass through unchanged.


```luma
      #returns_ownership
unescape_str -> fn(
    s: *byte
) *byte
```

**Returns:**
Newly allocated, NUL-terminated string; the caller owns the buffer.

### `enstruct`

```luma
      #returns_ownership
enstruct -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `binary`

```luma
      #returns_ownership
binary -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `is_shift_right`

```luma
      is_shift_right -> fn(
    p: *PARS::Parser
) bool
```

### `current_bp`

```luma
      current_bp -> fn(
    p: *PARS::Parser
) i64
```

### `try_generic_instantiation`

```luma
      #returns_ownership
try_generic_instantiation -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `call`

```luma
      #returns_ownership
call -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `assign`

```luma
      #returns_ownership
assign -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `prefix`

```luma
      #returns_ownership
prefix -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `primary`

```luma
      #returns_ownership
primary -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `unary`

```luma
      #returns_ownership
unary -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `group`

```luma
      #returns_ownership
group -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `array`

```luma
      #returns_ownership
array -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `index`

```luma
      #returns_ownership
index -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `deref`

```luma
      #returns_ownership
deref -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `addr`

```luma
      #returns_ownership
addr -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_alloc`

```luma
      #returns_ownership
_alloc -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_free`

```luma
      #returns_ownership
_free -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_cast`

```luma
      #returns_ownership
_cast -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_input`

```luma
      #returns_ownership
_input -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_system`

```luma
      #returns_ownership
_system -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `syscall`

```luma
      #returns_ownership
syscall -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_sizeof`

```luma
      #returns_ownership
_sizeof -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `estruct`

```luma
      #returns_ownership
estruct -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `nud`

Parses the left-hand operand (the "null denotation") of an expression,
dispatching on the current token. Lexer-detected error tokens are consumed
and reported without an extra diagnostic.


```luma
      #returns_ownership
nud -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

**Returns:**
The parsed node, or null after recording an error.

### `led`

Parses an infix/postfix continuation (the "left denotation") for `left` at
binding power `bp`, dispatching on the current token.


```luma
      #returns_ownership
led -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

**Returns:**
The combined node, or null after recording an error.

### `try_generic_instantiation`

Speculatively parses `left '<' Type (',' Type)* '>'` as a generic
call/struct-literal/member site and attaches the type arguments, provided
the `(')`/`{`/`::` following shape holds.

On failure the parser and error state are rolled back to the savepoint so
the `<` can be parsed as an ordinary comparison.

```luma
      #returns_ownership
try_generic_instantiation -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `parse_expr`

Parses a full expression at binding power `bp` using the `nud`/`led`
loop, stopping once the current token binds tighter than `bp`.


```luma
pub #returns_ownership
parse_expr -> fn(
    p: *PARS::Parser,
    bp: i64
) *AST::AstNode
```

**Returns:**
The expression node, or null on error.

### `enstruct`

Parses `left { name: value, ... }` — a named struct literal — into a
struct-expression AST node.

```luma
      #returns_ownership
enstruct -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `binary`

Parses a binary operator: consumes the operator token (plus a second `>`
when it forms a right-shift) and parses the right operand at `bp`.

```luma
      #returns_ownership
binary -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `is_shift_right`

Returns true when the current and next tokens are both `>` (adjacent `>`
tokens after a comparison could only form a right-shift).

```luma
      is_shift_right -> fn(
    p: *PARS::Parser
) bool
```

### `current_bp`

Returns the binding power of the current token, treating `>>` as a shift.

```luma
      current_bp -> fn(
    p: *PARS::Parser
) i64
```

### `call`

Parses `left(args)` into a call node.

```luma
      #returns_ownership
call -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `assign`

Parses `left = value` into an assignment node.

```luma
      #returns_ownership
assign -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `prefix`

Parses a postfix/prefix expression on `left`: indexing `left[...]`, member
access `left.m` / `left::m`, or postfix `++`/`--`.

```luma
      #returns_ownership
prefix -> fn(
    p: *PARS::Parser,
    left: *AST::AstNode,
    bp: i64
) *AST::AstNode
```

### `primary`

Parses a primary expression: integer, float, string, char, or bool literal
or an identifier. String/char literals are unescaped here.

```luma
      #returns_ownership
primary -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `unary`

Parses a prefix unary expression (`-`, `+`, `!`, `~`, `++`, `--`).

```luma
      #returns_ownership
unary -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `group`

Parses a parenthesized `( ... )` grouping expression.

```luma
      #returns_ownership
group -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `array`

Parses an array literal `[elem, ...]` into an array node.

```luma
      #returns_ownership
array -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `deref`

Parses `*object` into a dereference node.

```luma
      #returns_ownership
deref -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `addr`

Parses `&object` into an address-of node.

```luma
      #returns_ownership
addr -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_alloc`

Parses `__alloc__(size)` into an allocation node.

```luma
      #returns_ownership
_alloc -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_free`

Parses `__free__(ptr)` into a deallocation node.

```luma
      #returns_ownership
_free -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_cast`

Parses `__cast__<Type>(expr)` into a cast node.

```luma
      #returns_ownership
_cast -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_input`

Parses `__input__<Type>(msg)` into an input node.

```luma
      #returns_ownership
_input -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_system`

Parses `__system__(cmd)` into a system-command node.

```luma
      #returns_ownership
_system -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `syscall`

Parses `__syscall__(arg, ...)` into a syscall node.

```luma
      #returns_ownership
syscall -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `_sizeof`

Parses `__sizeof__<Type>` into a sizeof node.

```luma
      #returns_ownership
_sizeof -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `estruct`

Parses an anonymous struct literal `{ name: value, ... }` (struct name and
alias null) into a struct-expression node.

```luma
      #returns_ownership
estruct -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

### `index`

Parses `left[expr]` into an index node.

```luma
      #returns_ownership
index -> fn(
    p: *PARS::Parser
) *AST::AstNode
```

