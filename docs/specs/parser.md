# Luma Parser Specification

## 1. Architecture Overview

The parser converts a stream of tokens into an Abstract Syntax Tree (AST). It uses a **Pratt parsing** approach for expressions (with binding power/precedence) and recursive descent for statements and types.

```
Source → Lexer → Tokens → Parser → AST (program node with modules)
```

### Entry Point

```c
Stmt *parse(GrowableArray *tks, ArenaAllocator *arena, BuildConfig *config);
```

Called from `run_build()` after lexing. Returns an `AST_PROGRAM` node containing an array of `AST_PREPROCESSOR_MODULE` nodes.

### Parser State

```c
struct Parser {
  file_path: const char*,
  arena: ArenaAllocator*,
  tks: Token[],          // Token stream
  tk_count: size_t,
  capacity: size_t,
  pos: size_t,           // Current position
  pending_doc_comment: char*,  // /// doc comment to attach to next node
}
```

## 2. Token Types (from Lexer)

The lexer produces tokens with type, value, line, column, and length. Key token categories:

| Category | Tokens |
|---|---|
| **Literals** | `TOK_NUMBER`, `TOK_NUM_FLOAT`, `TOK_STRING`, `TOK_CHAR_LITERAL`, `TOK_TRUE`, `TOK_FALSE`, `TOK_NULL` |
| **Identifiers** | `TOK_IDENTIFIER` |
| **Keywords** | `TOK_VAR`, `TOK_CONST`, `TOK_IF`, `TOK_ELSE`, `TOK_LOOP`, `TOK_FOR`, `TOK_WHILE`, `TOK_RETURN`, `TOK_BREAK`, `TOK_CONTINUE`, `TOK_PRINT`, `TOK_PRINTLN`, `TOK_SWITCH`, `TOK_CASE`, `TOK_DEFAULT`, `TOK_DEFER`, `TOK_PUBLIC`, `TOK_PRIVATE`, `TOK_USE`, `TOK_OS`, `TOK_LINK`, `TOK_IMPL`, `TOK_FN` |
| **Types** | `TOK_INT`, `TOK_UINT`, `TOK_FLOAT`, `TOK_DOUBLE`, `TOK_BOOL`, `TOK_STRINGT`, `TOK_VOID`, `TOK_CHAR` |
| **Builtins** | `TOK_ALLOC`, `TOK_FREE`, `TOK_MEMCPY`, `TOK_CAST`, `TOK_INPUT`, `TOK_SYSTEM`, `TOK_SYSCALL`, `TOK_SIZE_OF` |
| **Operators** | `TOK_PLUS`, `TOK_MINUS`, `TOK_STAR`, `TOK_SLASH`, `TOK_MODL`, `TOK_EQEQ`, `TOK_NEQ`, `TOK_LT`, `TOK_LE`, `TOK_GT`, `TOK_GE`, `TOK_AND`, `TOK_OR`, `TOK_AMP`, `TOK_PIPE`, `TOK_CARET`, `TOK_SHIFT_LEFT`, `TOK_SHIFT_RIGHT`, `TOK_RANGE`, `TOK_BANG`, `TOK_TILDE`, `TOK_PLUSPLUS`, `TOK_MINUSMINUS` |
| **Delimiters** | `TOK_LPAREN`, `TOK_RPAREN`, `TOK_LBRACKET`, `TOK_RBRACKET`, `TOK_LBRACE`, `TOK_RBRACE`, `TOK_COMMA`, `TOK_SEMICOLON`, `TOK_COLON`, `TOK_DOT`, `TOK_RESOLVE`, `TOK_EQUAL`, `TOK_QUESTION` |
| **Attributes** | `TOK_RETURNES_OWNERSHIP`, `TOK_TAKES_OWNERSHIP`, `TOK_DLL_IMPORT`, `TOK_LIB_IMPORT` |

## 3. Pratt Expression Parsing

### Binding Power / Precedence

| Level | Operators | Assoc |
|---|---|---|
| `BP_PRIMARY` (17) | primary expressions | — |
| `BP_CALL` (16) | `()`, `[]`, `.`, `::`, `{}` | left |
| `BP_POSTFIX` (15) | `++`, `--` (postfix) | left |
| `BP_UNARY` (14) | prefix `!`, `~`, `+`, `-`, `++`, `--` | right |
| `BP_EXPONENT` (13) | `**` | right |
| `BP_PRODUCT` (12) | `*`, `/`, `%` | left |
| `BP_SUM` (11) | `+`, `-` | left |
| `BP_SHIFT` (10) | `<<`, `>>` | left |
| `BP_RANGE` (9) | `..` | left |
| `BP_RELATIONAL` (8) | `<`, `<=`, `>`, `>=` | left |
| `BP_EQUALITY` (7) | `==`, `!=` | left |
| `BP_BITWISE_AND` (6) | `&` | left |
| `BP_BITWISE_XOR` (5) | `^` | left |
| `BP_BITWISE_OR` (4) | `\|` | left |
| `BP_LOGICAL_AND` (3) | `&&` | left |
| `BP_LOGICAL_OR` (2) | `\|\|` | left |
| `BP_TERNARY` (1.5) | `? :` | right |
| `BP_ASSIGN` (1) | `=` | right |

### Pratt Algorithm

```c
Expr *parse_expr(Parser *parser, BindingPower bp) {
  Expr *left = nud(parser);                        // prefix or primary
  while (get_bp(current_token) > bp) {
    left = led(parser, left, current_bp);          // infix or postfix
  }
  return left;
}
```

**nud (null denotation)** — handles tokens at expression start:

| Token | Handler |
|---|---|
| `TOK_NUMBER`, `TOK_STRING`, `TOK_TRUE`, `TOK_FALSE`, `TOK_CHAR_LITERAL`, `TOK_IDENTIFIER` | `primary()` — literal or identifier |
| `TOK_PLUS`, `TOK_MINUS`, `TOK_BANG`, `TOK_TILDE`, `TOK_PLUSPLUS`, `TOK_MINUSMINUS` | `unary()` — prefix operator |
| `TOK_LPAREN` | `grouping()` — parenthesized expression |
| `TOK_LBRACKET` | `array_expr()` — array literal `[a, b, c]` |
| `TOK_LBRACE` | `struct_expr()` — anonymous struct `{ x: 1, y: 2 }` |
| `TOK_STAR` | `deref_expr()` — pointer dereference |
| `TOK_AMP` | `addr_expr()` — address-of |
| `TOK_ALLOC`, `TOK_FREE`, `TOK_CAST`, `TOK_INPUT`, `TOK_SYSTEM`, `TOK_SYSCALL`, `TOK_SIZE_OF` | respective builtin handler |

**led (left denotation)** — handles infix and postfix:

| Token | Handler | Produces |
|---|---|---|
| Arithmetic/comparison/logical/bitwise/range | `binary()` | `AST_EXPR_BINARY` |
| `TOK_LPAREN` | `call_expr()` | `AST_EXPR_CALL` |
| `TOK_EQUAL` | `assign_expr()` | `AST_EXPR_ASSIGNMENT` |
| `TOK_DOT` / `TOK_RESOLVE` | `prefix_expr()` → member | `AST_EXPR_MEMBER` |
| `TOK_LBRACKET` | `prefix_expr()` → index | `AST_EXPR_INDEX` |
| `TOK_PLUSPLUS` / `TOK_MINUSMINUS` | `prefix_expr()` → postfix | `AST_EXPR_UNARY` (post_inc/dec) |
| `TOK_LBRACE` | `named_struct_expr()` | `AST_EXPR_STRUCT` (named) |

## 4. Statement Parsing

The top-level dispatch in `parse_stmt()`:

1. Collect doc comments (`///` lines)
2. Collect attribute modifiers (`#returns_ownership`, `#takes_ownership`, `#dll_import`, `#lib_import`)
3. Check visibility modifier (`public`/`private`)
4. Dispatch by current token:

| Token | Handler | AST Node Type |
|---|---|---|
| `TOK_USE` | `use_stmt()` | `AST_PREPROCESSOR_USE` |
| `TOK_OS` | `os_stmt()` | `AST_PREPROCESSOR_OS` |
| `TOK_LINK` | `link_stmt()` | `AST_PREPROCESSOR_LINK` |
| `TOK_CONST` | `const_stmt()` | `AST_STMT_VAR_DECL` (immutable) |
| `TOK_VAR` | `var_stmt()` | `AST_STMT_VAR_DECL` |
| `TOK_RETURN` | `return_stmt()` | `AST_STMT_RETURN` |
| `TOK_LBRACE` | `block_stmt()` | `AST_STMT_BLOCK` |
| `TOK_IF` | `if_stmt()` | `AST_STMT_IF` |
| `TOK_LOOP` | `loop_stmt()` | `AST_STMT_LOOP` |
| `TOK_PRINT` | `print_stmt(ln=false)` | `AST_STMT_PRINT` |
| `TOK_PRINTLN` | `print_stmt(ln=true)` | `AST_STMT_PRINT` |
| `TOK_BREAK` / `TOK_CONTINUE` | `break_continue_stmt()` | `AST_STMT_BREAK_CONTINUE` |
| `TOK_DEFER` | `defer_stmt()` | `AST_STMT_DEFER` |
| `TOK_SWITCH` | `switch_stmt()` | `AST_STMT_SWITCH` |
| `TOK_IMPL` | `impl_stmt()` | `AST_STMT_IMPL` |
| `default` | `expr_stmt()` — parse expression then expect `;` | `AST_STMT_EXPRESSION` |

### Function Declaration Parsing

Triggered by identifier token (function name) followed by `(`. Steps:
1. Parse function name (identifier)
2. Parse `(param1: Type1, param2: Type2, ...)`
3. Parse optional `-> ReturnType`
4. If `{` follows → parse body block
5. If `;` follows → forward declaration
6. Wraps in `AST_STMT_FUNCTION`

### Struct Declaration Parsing

Triggered by `struct` keyword:
1. Parse struct name
2. Parse body `{ ... }`
3. Inside body, parse fields as `name: Type` or methods as `fn name(params) -> Ret { body }`
4. Public/private sections with `public { }` / `private { }`
5. Wraps in `AST_STMT_STRUCT` with `AST_STMT_FIELD_DECL` children

### Enum Declaration Parsing

Triggered by `enum` keyword:
1. Parse enum name
2. Parse `{ Member1, Member2, ... }`
3. Wraps in `AST_STMT_ENUM`

### If / Elif / Else

Parsing:
1. Parse `if condition { body }`
2. Parse zero or more `elif condition { body }` blocks
3. Parse optional `else { body }`
4. Nests elif blocks as `AST_STMT_IF` nodes inside `elif_stmts[]`

### Loop Parsing

Three forms distinguished by what follows `loop`:
- **Infinite**: `loop { body }` — no condition/initializer
- **While**: `loop condition { body }` — condition present, no initializer
- **For**: `loop init; condition; optional { body }` — all three present

### Switch / Case / Default

1. Parse `switch condition { ... }`
2. Parse cases: `case value1, value2 => { body }`
3. Parse optional default: `default => { body }`

### Impl Statement

Used to declare implementations for structs:
```
impl [func_list...] -> [struct_list...] { ... }
```
Still in early stages.

## 5. Type Parsing

`parse_type()` dispatches via `tnud()` + `tled()` (also Pratt-style):

| Token | Type AST |
|---|---|
| `TOK_INT` | `AST_TYPE_BASIC("int")` |
| `TOK_UINT` | `AST_TYPE_BASIC("uint")` |
| `TOK_FLOAT` | `AST_TYPE_BASIC("float")` |
| `TOK_DOUBLE` | `AST_TYPE_BASIC("double")` |
| `TOK_BOOL` | `AST_TYPE_BASIC("bool")` |
| `TOK_STRINGT` | `AST_TYPE_BASIC("str")` |
| `TOK_VOID` | `AST_TYPE_BASIC("void")` |
| `TOK_CHAR` | `AST_TYPE_BASIC("char")` |
| `TOK_IDENTIFIER` | `AST_TYPE_BASIC(name)` (user-defined) |
| `TOK_STAR` | `AST_TYPE_POINTER(inner)` |
| `TOK_LBRACKET` | `AST_TYPE_ARRAY(element, size)` — `[size]type` or `[]type` |
| `TOK_FN` | `AST_TYPE_FUNCTION(params, return)` — `fn(T1, T2) R` |
| `TOK_RESOLVE` followed by identifiers | `AST_TYPE_RESOLUTION(parts)` — `Module::Type` |

## 6. Expression Details

### Primary Expressions

**Literals**: Convert token value to appropriate C type and wrap in `AST_EXPR_LITERAL`:
- `TOK_NUMBER` → `LITERAL_INT` with `long long` value (via `strtoll`)
- `TOK_NUM_FLOAT` → `LITERAL_FLOAT` with `double` value (via `strtod`)
- `TOK_STRING` → `LITERAL_STRING` with arena-copied string
- `TOK_CHAR_LITERAL` → `LITERAL_CHAR` with `char` value
- `TOK_TRUE`/`TOK_FALSE` → `LITERAL_BOOL` with `bool` value

**Identifiers**: `AST_EXPR_IDENTIFIER` with arena-copied name.

### Binary Expressions

Map the token type to a `BinaryOp` via `TOKEN_TO_BINOP_MAP`, then recurse with current binding power to get right operand.

### Unary Expressions

Map token to `UnaryOp` via `TOKEN_TO_UNOP_MAP`, parse operand at `BP_UNARY` binding power.

### Function Calls

Consume `(`, parse comma-separated expression list, consume `)`. Wrap callee + args in `AST_EXPR_CALL`.

### Array Literals

Consume `[`, parse comma-separated expressions, consume `]`. Wrap in `AST_EXPR_ARRAY`.

### Struct Literals

**Anonymous** (`nud` path): Parse `{ name1: expr1, name2: expr2, ... }`.
**Named** (`led` path, after identifier): Parse `Point { x: 1, y: 2 }`.

Both produce `AST_EXPR_STRUCT` — anonymous has `name = NULL`, named has the struct type name.

### Cast

`cast<Type>(expr)` — parse type between `< >`, expression in `( )`.

### Input

`input<Type>("msg")` — parse type between `< >`, expression in `( )`.

### Deref / Addr

`*expr` → `AST_EXPR_DEREF`, `&expr` → `AST_EXPR_ADDR`. Both parse at `BP_UNARY` binding power.

## 7. Statement Details

### Variable Declaration

```
var x: Type = value;
const x: Type = value;  // immutable
```

Parses: name, optional `: Type` annotation, optional `= value`. Always consumes trailing `;`.

### Function Declaration

```
fn name(p1: T1, p2: T2) -> RetType { body }
fn name(p1: T1) -> RetType;  // forward declaration
```

Supports ownership annotations (`#returns_ownership`, `#takes_ownership`), DLL import (`#dll_import("lib.dll", callconv: "stdcall")`), and lib import (`#lib_import("lib.so")`).

### Struct Declaration

```
struct Name {
  public {
    x: int,
    fn method(self: *Name, ...) -> Ret { body },
  }
  private {
    y: int,
  }
}
```

### Enum Declaration

```
enum Name { Member1, Member2, Member3 }
```

### Print

```
print(expr1, expr2, ...);
println(expr1, expr2, ...);   // with newline
```

### Return

```
return value;   // or just return; for void functions
```

### Block

```
{ stmt1; stmt2; ... }
```

Creates `AST_STMT_BLOCK` with child statement array.

### If / Elif / Else

```
if condition { body }
elif condition { body }
else { body }
```

### Loop

```
loop { body }                          // infinite
loop condition { body }                // while
loop init; condition; post { body }    // for
```

### Defer

```
defer { statement }
```

Used for cleanup — the deferred statement runs when the enclosing scope exits.

### Switch

```
switch condition {
  case val1, val2 => { body }
  default => { body }
}
```

## 8. Preprocessor

Modules are parsed as a single declaration at the top of each file:
```
module name;
@use "other_module" as alias;
@os { "linux" => { ... }, "windows" => { ... } }
@link "mylib"
```

The `parse()` function creates one module per file with all statements as the module body. The program node wraps all modules.

## 9. Doc Comments

Lines starting with `///` before a declaration are collected into `parser->pending_doc_comment` and attached to the next parsed statement as `doc_comment` (via `collect_doc_comments()` / `consume_doc_comments()`).

## 10. Error Handling

`parser_error()` creates an `ErrorInformation` struct with:
- Error type (e.g., `"SyntaxError"`)
- File path, line, column
- Line text extracted from tokens
- Token length for underlining

Errors are collected via `error_add()` and reported after parsing.

## 11. Implementation Checklist for Self-Hosted Port

- [ ] `Parser` struct with position tracking, doc comment buffer
- [ ] Token utilities: `p_peek()`, `p_current()`, `p_advance()`, `p_consume()`
- [ ] Binding power system (`BindingPower` enum, `get_bp()`)
- [ ] Pratt algorithm: `parse_expr()`, `nud()`, `led()`
- [ ] Primary expressions: literals, identifiers
- [ ] Binary, unary, assignment operators
- [ ] Function calls, member access (`./::`), indexing
- [ ] Array literals, struct literals (named + anonymous)
- [ ] Builtins: alloc, free, cast, input, system, syscall, sizeof
- [ ] Dereference and address-of
- [ ] Variable declarations (var + const)
- [ ] Function declarations (with attributes, forward decls)
- [ ] Struct declarations (fields, methods, public/private)
- [ ] Enum declarations
- [ ] If/elif/else
- [ ] Loops (infinite, while, for)
- [ ] Switch/case/default
- [ ] Defer, return, break/continue, print
- [ ] Impl statement
- [ ] Type parsing: basic types, pointers, arrays, function types, resolution types
- [ ] Preprocessor: module, use, os, link
- [ ] Doc comment collection
- [ ] Error reporting
- [ ] Module aggregation into program node
