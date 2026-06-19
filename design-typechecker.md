# Luma Typechecker: Complete Design Document

## 1. Architecture Overview

The typechecker lives in `src/typechecker/` (8 files, ~6,900 lines of C) and performs semantic analysis on the AST produced by the parser + module combiner. It sits between parsing and LLVM codegen:

```
Lex → Parse → Combine Modules → Typecheck → LLVM Codegen
```

### Entry Point

`typecheck()` in `tc.c:6` dispatches by `node->category`:

| Category | Handler | Description |
|---|---|---|
| `STMT` | `typecheck_statement()` | Returns bool (pass/fail) |
| `EXPR` | `typecheck_expression()` | Returns `AstNode*` (inferred type), or NULL on error |
| `TYPE` | Pass-through | Returns `true` (types are validated structurally) |
| `PREPROCESSOR` | Dispatches to module/use/os/link handlers | Module system |

### File Breakdown

| File | Lines | Role |
|---|---|---|
| `type.h` | 356 | Master header: all structs, all function declarations |
| `tc.c` | 52 | Entry dispatcher (`typecheck()`) |
| `lookup.c` | 296 | `typecheck_statement()` dispatcher, `typecheck_expression()` dispatcher, `typecheck_program_multipass()` (3-pass orchestrator) |
| `expr.c` | 1,885 | All expression type-checking (binary, unary, call, member access, alloc/free, cast, etc.) |
| `stmt.c` | 1,786 | All statement type-checking (var/func/struct/enum decls, if, loops, switch, defer, return) |
| `type.c` | 619 | Type utilities (`types_match`, `is_numeric_type`, `type_to_string`, `is_cast_valid`, struct helpers) |
| `scope.c` | 293 | Scope management (init, create_child, add_symbol, lookup with visibility) |
| `module.c` | 418 | Module system (register, find, import, qualified lookup, dependency graph, `@os`/`@link`) |
| `error.c` | 141 | Three error-reporting functions (`tc_error`, `tc_error_help`, `tc_error_id`) |
| `static_mem_tracker.c` | 372 | Static memory analysis (track alloc/free/alias, use-after-free, double-free, leak detection) |
| `array.c` | 215 | Array type validation, bounds checking, multi-dim array access |

---

## 2. Pipeline Integration

The build pipeline in `src/helper/run.c` calls typechecking at **stage 4**:

```
Stage 1: Lex all files → token arrays
Stage 2: Parse all files → module ASTs
Stage 3: Combine modules → single AST_PROGRAM
Stage 4: Typecheck → semantic errors
Stage 5-10: LLVM codegen, optimize, link
```

The typechecker mutates the AST inline: it attaches `scope` pointers to nodes, injects `&self` as the first argument in method calls, and pads array arguments. The LLVM codegen reads these modified AST fields.

---

## 3. Core Data Structures

### 3.1 Types (Type Representation)

Types are `AstNode` tagged unions with `category = Node_Category_TYPE`. Seven variants:

| `type` enum | Key fields | Example |
|---|---|---|
| `AST_TYPE_BASIC` | `name: str` | `int`, `float`, `bool`, `char`, `str`, `void`, **enum names** |
| `AST_TYPE_POINTER` | `pointee_type: AstNode*` | `int*` |
| `AST_TYPE_ARRAY` | `element_type: AstNode*`, `size: Expr*` | `int[10]`, `char[]` |
| `AST_TYPE_FUNCTION` | `param_types: AstNode**`, `param_count`, `return_type: AstNode*` | `fn(int, int) bool` |
| `AST_TYPE_STRUCT` | `name: str`, `member_types: AstNode**`, `member_names: str*`, `member_count` | `struct Point` |
| `AST_TYPE_ENUM` | Represented as `AST_TYPE_BASIC` with the enum's name | `Color` (acts like `int`) |
| `AST_TYPE_RESOLUTION` | `parts[]`, `part_count` | `Module::Type` (qualified name) |

**Key distinction**: Structs use **nominal typing** (match by name, not structure). Enums are represented as basic types with the enum's name as the type name — they are compatible with `int` but distinguished from other enums for exhaustiveness checking.

### 3.2 Type Match Result (`TypeMatchResult`)

```c
enum { TYPE_MATCH_EXACT, TYPE_MATCH_COMPATIBLE, TYPE_MATCH_NONE }
```

**Implicit conversions** (return `COMPATIBLE`):
- `enum ↔ int`
- `float ↔ double`
- `str ↔ char*`
- Array element match → pointer match (array decay)
- Function ↔ pointer-to-function

### 3.3 Symbol (`Symbol`)

```c
struct Symbol {
    name: str,
    type: AstNode*,           // the type of this symbol
    is_public: bool,
    is_mutable: bool,
    scope_depth: int,
    returns_ownership: bool,  // for functions: #returns_ownership
    takes_ownership: bool,    // for functions: #takes_ownership
}
```

Ownership flags are stored on the **symbol** (not the type) because they are properties of the function declaration, not the function signature.

### 3.4 Scope (`Scope`)

```c
struct Scope {
    parent: Scope*,
    symbols: GrowableArray<Symbol>,
    children: GrowableArray<Scope*>,
    scope_name: str,
    depth: int,
    is_function_scope: bool,
    associated_node: AstNode*,           // func_decl for function scopes
    is_module_scope: bool,
    module_name: str,
    imported_modules: GrowableArray<ModuleImport>,
    memory_analyzer: StaticMemoryAnalyzer*,
    deferred_frees: GrowableArray<str>,
    link_libs: GrowableArray<str>,
    config: BuildConfig*,
    returns_ownership: bool,            // propagated from func decl
    takes_ownership: bool,
}
```

The memory analyzer is **shared** via pointer from the root scope — all scopes in a module point to the same `StaticMemoryAnalyzer` instance, so allocations and frees in nested scopes are tracked in one flat list.

### 3.5 Module Structures

```c
struct ModuleImport {
    module_name: str,
    alias: str,            // the name used to access this module
    module_scope: Scope*,
}

struct ModuleDependency {
    module_name: str,
    dependencies: GrowableArray<str>,  // module names from @use
    processed: bool,
}
```

Modules are registered with a `__module_<name>` prefixed symbol in the global scope.

### 3.6 Memory Analyzer

```c
struct StaticMemoryAnalyzer {
    allocations: GrowableArray<StaticAllocation>,
    arena: ArenaAllocator*,
    skip_memory_tracking: bool,  // set true during defer blocks
}

struct StaticAllocation {
    line, column: int,
    variable_name: str,
    has_matching_free: bool,         // at least one free seen
    free_count: int,                 // unconditional frees
    conditional_free_count: int,     // frees in branches/loops
    aliases: GrowableArray<str>,     // other variables aliasing this alloc
    reported: bool,                  // already emitted diagnostic
    address_taken: bool,             // &var was taken (ownership escaped)
    function_name: str,
    file_path: str,
}
```

### 3.7 Error Structures

```c
struct TypeError {
    message: str,
    line, column: int,
    context: str,
}
```

The actual runtime error system (`ErrorInformation`) lives in `src/c_libs/error/error.h` and supports line text extraction from tokens, caret positioning, help text, and notes.

---

## 4. Module System (3-Pass)

### Pass 1: Register All Module Scopes

In `typecheck_program_multipass()` (`lookup.c:187`):
- Iterates all AST_PREPROCESSOR_MODULE nodes
- Creates a `Scope` for each via `create_module_scope()`
- Registers with `__module_<name>` symbol in global scope
- **Duplicate module names are errors**

### Pass 2: Resolve All @use Statements

- Iterates all modules again
- Only processes `AST_PREPROCESSOR_USE` nodes
- Calls `typecheck_use_stmt()` which calls `find_module_scope()` + `add_module_import()`
- Must happen before typechecking bodies because code may reference imported symbols

### Pass 3: Typecheck Bodies in Dependency Order

Builds a `ModuleDependency` graph from `@use` statements, then processes modules via `process_module_in_order()` (topological sort — recursive DFS with `processed` flag). Each module gets **5 sub-passes**:

#### Pass 3a: Forward Declarations
Processes all `AST_STMT_FUNCTION` with `forward_declared == true`. These register function signatures in the module scope without checking bodies, enabling mutual recursion.

#### Pass 3b: Pre-register @os Symbols
Scans `@os` blocks, finds the arm matching the target OS config, and pre-registers function and constant symbols from that block so the rest of the module can reference them.

#### Pass 3c: All Other Statements
Everything except `@use`, `@os`, `@link`, and forward declarations. This is the main typechecking pass.

#### Pass 3d: @os Blocks + @link
Actually typechecks the body of the matched `@os` arm and processes `@link` directives.

#### Pass 3e: Static Memory Report
Calls `static_memory_check_and_report()` to report leaks, double-frees (conditioned on `config->check_mem`).

### Dependency Graph (`build_dependency_graph`)

- Scans each module's body for `AST_PREPROCESSOR_USE` nodes
- Records `imported_module` names as dependencies
- `process_module_in_order()` does recursive DFS: processes dependencies before the module itself
- No cycle detection (would stack overflow on circular imports)

---

## 5. Scope & Symbol Table

### 5.1 Scope Hierarchy

```
global (root, depth 0)
├── module "io" (depth 1, is_module_scope=true)
│   ├── function "println" (depth 2, is_function_scope=true)
│   │   ├── block { ... } (depth 3)
│   │   └── ...
│   └── ...
├── module "math" (depth 1)
│   └── ...
└── ...
```

### 5.2 Key Scopes Created

| Scope Name | Created When | Properties |
|---|---|---|
| `global` | Program start (depth 0) | Root scope, owns all modules |
| `module` | Per `@module` | `is_module_scope`, `module_name` set |
| `function` | Per function decl | `is_function_scope`, `associated_node = func_decl` |
| `block` | `{ }` code blocks | New scope for each block |
| `then/elif/else_branch` | If/elif/else stmts | Separate scopes for each branch |
| `infinite_loop` / `while_loop` / `for_loop` | Loops | Loop body scope |
| `switch` / `case` / `default` | Switch/case/default | Switch body + individual case scopes |
| `"unnamed"` | Fallback | When no name is provided |

### 5.3 Symbol Lookup Algorithm

`scope_lookup_with_visibility()` in `scope.c:81`:
1. Walk up the scope chain (current → parent → ... → global)
2. For each scope, **linear search** through `scope->symbols`
3. **Visibility check**: public symbols always visible; private symbols visible only within the same module (determined by comparing `find_containing_module()` results)
4. After checking current scope's symbols, also search **imported modules** via `scope_lookup_current_only_with_visibility()` on each `ModuleImport.module_scope`
5. First match wins or return NULL

### 5.4 Symbol Addition

`scope_add_symbol()` / `scope_add_symbol_with_ownership()`:
- Checks for duplicates in **current scope only** (inner scopes can shadow outer ones)
- If duplicate found, returns `true` silently (used to handle prototype → implementation)
- Creates a `Symbol` entry in the growable array

### 5.5 Qualified Name Convention

Struct methods and enum members use a **dot-qualified naming convention**:
- `StructName.methodName` — e.g., `Point.translate`
- `EnumName.memberName` — e.g., `Color.Red`

This enables scope lookup to find both struct methods (`Point.translate`) and enum members (`Color.Red`) through normal symbol lookup.

---

## 6. Type System

### 6.1 `types_match()` Algorithm (`type.c:18`)

Returns `TYPE_MATCH_EXACT`, `TYPE_MATCH_COMPATIBLE`, or `TYPE_MATCH_NONE`.

**Checking order:**
1. Null safety: either NULL → `NONE`
2. Pointer identity: same pointer → `EXACT`
3. Category check: both must be `Node_Category_TYPE`
4. **Struct matching**: nominal (by name), exact match only
5. **Basic matching**: by name; then special cases:
   - Non-builtin + "int" → `COMPATIBLE` (enum ↔ int)
   - Float ↔ Double → `COMPATIBLE`
   - "str" ↔ "char*" → `COMPATIBLE`
6. **Pointer matching**: recursive on pointee types
7. **Array matching**: element type + size comparison (literal sizes must match exactly; unsized arrays compatible)
8. **Array ↔ Pointer**: decays array to pointer (element vs pointee)
9. **Function matching**: return type + all param types recursively; parameter count must match
10. **Function ↔ pointer-to-function**: recursive check through pointer

### 6.2 Type Introspection Functions

| Function | Returns true if... |
|---|---|
| `is_numeric_type` | `int`, `float`, `double`, `char` |
| `is_integer_type` | `int`, `char` |
| `is_pointer_type` | `AST_TYPE_POINTER` |
| `is_array_type` | `AST_TYPE_ARRAY` |
| `is_void_type` | basic `"void"` |
| `is_bool_type` | basic `"bool"` |
| `is_struct_type_node` | `AST_TYPE_STRUCT` |
| `is_function_type` | `AST_TYPE_FUNCTION` |
| `is_pointer_to_function_type` | Pointer → function |

### 6.3 `is_cast_valid()` (`type.c:285`)

Valid casts:
- Types that already match (EXACT or COMPATIBLE)
- **Any numeric to any numeric** (including bool)
- **Pointer to pointer**
- **Pointer ↔ numeric** (systems programming idiom)
- **Function ↔ pointer** (function pointers)
- **Function ↔ function** (recursive signature check)
- **Struct to same struct** (identity cast)
- **Any type to void** (discard)
- **NOT**: void to anything else

### 6.4 `type_to_string()` (`type.c:354`)

Pretty-printer:
- Basic: `"int"`, `"bool"`, etc.
- Struct: `"struct Point"`
- Pointer: `"int*"`, `"char**"`
- Array: `"int[]"` (no size in string!)
- Function: `"fn(int, int) bool"`

**Note**: Array size is not included in the string representation, which can make size-mismatch errors confusing.

---

## 7. Expression Type Checking

### Dispatcher (`lookup.c:98`)

`typecheck_expression()` switches on `expr->type` and delegates to specific functions. Every expression handler returns the inferred `AstNode*` type or `NULL` on error.

### 7.1 Literals

| Source | Inferred Type |
|---|---|
| `42` | `int` |
| `3.14` | `double` |
| `"hello"` | `str` |
| `true` / `false` | `bool` |
| `'x'` | `char` |
| `null` | `null` |

### 7.2 Identifiers

`scope_lookup()` by name. Error: "Undefined Identifier" if not found. Returns `symbol->type`.

### 7.3 Grouping (parentheses)

Recurse into `expr->expr.grouping.expr`.

### 7.4 Binary Operations

**Arithmetic** (`+`, `-`, `*`, `/`, `%`, `**`):
- Both operands must be numeric
- `%` requires integer types only
- Type promotion: `double > float > int`
- Returns the promoted type

**Comparison** (`==`, `!=`, `<`, `<=`, `>`, `>=`):
- Operands must be compatible
- Returns `bool`

**Logical** (`&&`, `||`):
- Both must be `bool`
- Returns `bool`

**Range** (`..`):
- Both must be `int`
- Returns `int[size]` (inclusive, computed by `right - left + 1`)
- Handles ascending, descending, and single-element ranges

**Bitwise** (`&`, `|`, `^`, `<<`, `>>`):
- Both must be numeric
- Returns `int`

### 7.5 Unary Operations

| Op | Requires | Returns |
|---|---|---|
| `!` | anything | `bool` |
| `-` | numeric | same as operand |
| `~` | numeric | same as operand |
| `++`/`--` (pre/post) | numeric (mutable lvalue) | same as operand |

### 7.6 Assignment (`=`)

**Complex handler** (`typecheck_assignment_expr`, expr.c:200):
1. Typecheck target → target_type, value → value_type
2. Check target is **mutable** (look up identifier, check `is_mutable`)
3. **Track allocations**: if value contains `alloc()`, record in memory analyzer (unless in `#returns_ownership` function or indexed assignment)
4. **Track ownership transfer**: if value is a `#returns_ownership` function call, record allocation
5. **Track pointer aliasing**: if both sides are pointers and it's a direct variable-to-variable assignment, track alias
6. `types_match()` check; special message for function-type mismatch vs pointer-to-function
7. Returns target_type

### 7.7 Function Calls

**Complex handler** (`typecheck_call_expr`, expr.c:335):

**Callee resolution**:
- **Simple identifier**: `scope_lookup()` by name
- **Member access `::`**: compile-time lookup via `lookup_qualified_symbol()` (module functions, qualified names)
- **Member access `.`**: runtime method call on struct instance

**Method call transformation** (for `obj.method(args...)`):
1. Typecheck `obj` and resolve to struct type (through pointers, basic→struct resolution)
2. Look up `method` in struct's member list
3. If member is a function type → this is a method call:
   - **Prepend `self`** as the first argument (address of `obj` or `obj` directly)
   - If method expects pointer but obj is a value → auto-apply `&`
   - Reallocate argument array with `+1` slots
   - Updates `expr->expr.call.args` and `arg_count` **in-place on the AST**

**Argument checking**:
- Parameter count must match
- Each argument type-checked against parameter type
- **Array padding**: if parameter expects array of size N and argument has array of size M ≤ N, allows the mismatch and sets `expr->array.target_size` for codegen

**Ownership handling**:
- If callee has `takes_ownership`: track each pointer argument as freed
  - In **defer blocks**: add to `deferred_frees` list on the function scope instead
- If method call with `takes_ownership`: track `self` argument as freed

### 7.8 Index Expression (`a[i]`)

1. Check use-after-free on the base variable (if identifier)
2. Typecheck object → must be array, pointer, or `string`
3. Typecheck index → must be integer type
4. Returns element type (or `char` for string)

### 7.9 Member Access (`a.b` / `a::b`)

**Compile-time (`::`)** — `typecheck_member_expr()`:
1. For simple identifiers: try `lookup_qualified_symbol()`, then `scope_lookup()`
2. For chained access (`ast::ExprKind::EXPR_NUMBER`): recursively resolve, then build `base.member` qualified name, look up
3. Supports module access, enum member access, and chaining

**Runtime (`.`)** :
1. Typecheck base expression (handles complex expressions like `lex.list[i]`)
2. Check use-after-free for pointer-to-struct identifiers
3. Auto-dereference through pointers to underlying struct type
4. Resolve basic type names to struct type via scope lookup (with imported module fallback)
5. Look up member name in struct's member list

### 7.10 Dereference (`*ptr`)

1. Check use-after-free
2. Verify pointer type
3. Return pointee type

### 7.11 Address-of (`&expr`)

1. Verify lvalue (identifier, member, index, deref)
2. Track address-taken in memory analyzer (suppresses leak reports)
3. Return `T*`

### 7.12 Alloc (`alloc(size)`)

1. Verify size is numeric
2. Return `void*`

**Memory tracking** happens at the **assignment site** (var decl or assignment), not in the expression itself.

### 7.13 Free (`free(ptr)`)

1. Verify ptr is pointer type
2. Track the free:
   - In **defer blocks**: add to `deferred_frees` list (non-indexed only)
   - Normal: track as free in memory analyzer
   - `free(&var)` pattern: warn about freeing non-allocated pointer
3. Return `void`

### 7.14 Cast (`cast<Type>(expr)`)

1. Typecheck the cast expression
2. Validate via `is_cast_valid()`
3. Return target type

### 7.15 Input (`input<Type>("prompt")`)

1. Validate type parameter is a supported basic type (`int`, `float`, `double`, `string`, `char`, `bool`)
2. Validate message is a string
3. Return the type parameter

### 7.16 System (`system("cmd")`)

1. Validate command is a string
2. Return `int`

### 7.17 Syscall (`syscall(num, args...)`)

1. At least 1 argument (syscall number)
2. First arg must be numeric
3. Remaining args should be numeric or pointer (warns otherwise)
4. Return `int`

### 7.18 Sizeof (`sizeof(Type)` / `sizeof(expr)`)

Always returns `int`. Does **not** actually compute the size — just validates the operand type.

### 7.19 Array Literal (`[1, 2, 3]`)

1. Typecheck first element to establish element type
2. All remaining elements must match (anonymous structs get expected-type hint)
3. Empty arrays error ("cannot infer type")
4. Return `element_type[count]`

### 7.20 Struct Literal (`Type { x: 1, y: 2 }`)

**Named** (`Point { x: 10, y: 20 }`):
1. Look up struct type by name
2. Validate each field exists
3. Typecheck each field value against member type
4. Methods cannot be initialized
5. Return basic type with struct name

**Anonymous** (`{ x: 10, y: 20 }`):
1. If `expected_type` is provided (from var decl or array context), resolve and validate against it
2. Otherwise create a true anonymous struct type (`__anon_struct_line_col`)
3. Check for duplicate field names
4. Return the struct type

---

## 8. Statement Type Checking

### Dispatcher (`lookup.c:6`)

`typecheck_statement()` switches on `stmt->type`.

### 8.1 Variable Declaration

1. **Track allocation**: if init contains `alloc()` and not in `#returns_ownership` function, record allocation
2. **Track pointer alias**: if init is pointer assignment, track alias
3. **Track `#returns_ownership` call**: if init calls a `#returns_ownership` function and not in such a function, record allocation
4. Typecheck initializer; if declared type exists, `types_match()` check (specialized error messages for function types)
5. If no declared type, infer from initializer
6. Must have either type annotation or initializer
7. `scope_add_symbol()` to register

### 8.2 Function Declaration

1. **Validate return type** (must be a type node)
2. **`#dll_import` functions** must not have a body
3. **`is_lib_import` / `is_dll_import`**: register symbol but skip body
4. **`main` validation**:
   - Must return `int`
   - 0 or 2 parameters: `argc: int`, `argv: **byte` (with name convention warnings)
   - Must be public (auto-fixes)
   - Cannot be forward declared
5. Create function type from params + return type
6. **Forward declaration support**:
   - First occurrence: register symbol (no body check)
   - Second occurrence (implementation): `function_signatures_match()` — checks return type, param count, each param type, and ownership flags
   - Error on duplicate prototypes
7. Create **function scope**, add parameters as local mutable symbols
8. Typecheck body
9. **Process deferred frees**: iterate `func_scope->deferred_frees` and mark each as conditionally freed

### 8.3 Struct Declaration

**Dual-pass design** (separate data fields from methods):

1. Validate name, check for duplicates in current scope
2. **First pass**: collect all **data fields** (non-method `field_decl` nodes), check for duplicate field names
3. Create struct type with data fields only
4. **Add struct type to scope** BEFORE processing methods (so method bodies can reference the struct type for `self`)
5. **Second pass**: process methods:
   - Create method scope with implicit `self: StructType*` parameter
   - Add method's declared parameters
   - Typecheck method body
   - Register with **qualified name**: `StructName.methodName`
   - Create method type that **includes `self`** as the first parameter
   - Append method to struct's `member_types` and `member_names`

**Visibility**: both public and private members processed similarly; public methods registered with `is_public=true`, private with `is_public=false`.

### 8.4 Enum Declaration

1. Create basic type node with `name = enum_name` (this IS the enum type)
2. Add `EnumName` to scope
3. For each member, add `EnumName.memberName` with the enum type

Enum values are **not given integer values** — the typechecker treats them as the enum type. Compatibility with `int` is handled by `types_match()`.

### 8.5 If/Elif/Else

1. Condition must be `bool`
2. Create child scopes for then/elif/else branches
3. Typecheck each branch body
4. **Deferred free processing**: after each branch, process `deferred_frees` as conditional frees (so `if { free(x); }` tracks as a conditional free)

### 8.6 Loops

Three variants dispatched by `typecheck_loop_decl()`:

| Type | Condition | Initializer | Handler |
|---|---|---|---|
| Infinite loop | NULL | NULL | `typecheck_infinite_loop_decl()` |
| While loop | Not NULL | NULL | `typecheck_while_loop_decl()` |
| For loop | Not NULL | Not NULL | `typecheck_for_loop_decl()` |

All create a child scope for the loop body. For loops also process initializer statements in the loop scope.

### 8.7 Switch/Case/Default

1. Typecheck condition
2. **Enum detection**: if condition type is a non-builtin basic type → it's an enum switch
3. Create switch scope
4. Typecheck each case:
   - Values must match condition type
   - For enum switches: member access must reference the same enum
   - Track covered enum members for exhaustiveness
   - **Duplicate case detection** for enum members
5. Typecheck default case (if present)
6. **Exhaustiveness analysis**:
   - Find all enum members via `find_enum_members()` (scans scope for `EnumName.*` patterns)
   - If all members covered AND default exists → warn "unnecessary default"
   - If not all members covered AND no default → error "non-exhaustive switch"

### 8.8 Return

1. Find enclosing function scope → get return type
2. **Void function**: must not return value
3. **Non-void function**: must return value matching return type
4. **Transitive ownership check**: if returning a `#returns_ownership` call result from a function that doesn't have `#returns_ownership`, warn
5. **`#returns_ownership` function**: if returning a pointer parameter, track it as freed (ownership passes through)
6. **Leak suppression**: only track as freed if the returned variable is a parameter (not a local allocation)

### 8.9 Defer

1. **Don't create a new scope** — use the same scope (so `deferred_frees` goes to the correct function scope)
2. Set `analyzer->skip_memory_tracking = true` while typechecking the defer body
3. `free()` calls inside defer add to the function scope's `deferred_frees` list (instead of tracking immediately)
4. Restore `skip_memory_tracking` afterward
5. When the **enclosing function scope exits**, all deferred frees are processed as unconditional frees (in `typecheck_func_decl`)

### 8.10 Block Statements

1. Create child scope
2. Typecheck all statements in the block scope
3. Process deferred frees as conditional (scope exit = conditional free)

### 8.11 Print / Expression Statements

Print: typecheck all arguments (no return type validation).
Expression statement: just typecheck the expression, discard result.

### 8.12 Break/Continue

No typechecking — trivially true.

---

## 9. Static Memory Analysis

### 9.1 Tracker Architecture

A single `StaticMemoryAnalyzer` is shared across all scopes in a module (attached to the root scope, inherited by children). This means allocations from all scopes are tracked in one flat list, disambiguated by `variable_name + function_name`.

### 9.2 Allocation Tracking

Triggered in three places:
1. **Variable declaration** with `alloc()` in initializer (`stmt.c:148`)
2. **Variable declaration** with `#returns_ownership` call initializer (`stmt.c:175`)
3. **Assignment** of `alloc()` to variable (`expr.c:233`)
4. **Assignment** of `#returns_ownership` call result (`expr.c:256`)

**NOT tracked** when inside a `#returns_ownership` function (ownership transfers to caller).

### 9.3 Free Tracking

Triggered in:
1. **`free(ptr)` expression** (`expr.c:1256`) — both defer and non-defer
2. **`#takes_ownership` function calls** — each pointer argument marked as freed (`expr.c:679`)
3. **Method calls with `#takes_ownership`** — `self` argument marked as freed (`expr.c:729`)

**Deferred frees**: in defer blocks, instead of tracking immediately, the variable name is added to `func_scope->deferred_frees`. When the function scope exits, these are processed as unconditional frees. When a block/if/loop scope exits before the function, they're processed as conditional frees.

### 9.4 Alias Tracking

When a pointer variable is assigned from another pointer (`ptr2 = ptr1`):
- `static_memory_track_alias()` adds `ptr2` as an alias of `ptr1`'s allocation record
- Only tracks **direct variable-to-variable** assignments (not struct member accesses like `node.next = other`)
- Both variables share the same `StaticAllocation` record
- Freeing one alias affects the others (use-after-free/double-free checks apply to all aliases)

### 9.5 Violations Detected

| Violation | Detection | Reported When |
|---|---|---|
| **Use-after-free** | `static_memory_check_use_after_free()` called on identifier, deref, index, member access | Variable access after it's been freed |
| **Double free** | `static_memory_check_and_report()` checks `alloc->free_count > 1` | At end of module (Pass 3e) |
| **Memory leak** | `static_memory_check_and_report()` checks `!alloc->has_matching_free && !alloc->address_taken` | At end of module (Pass 3e) |
| **Freeing non-allocated** | `static_memory_check_free_nonalloc()` called on `free(&var)` pattern | At the free call site |

### 9.6 Ownership Annotations

| Annotation | Meaning | Effect |
|---|---|---|
| `#returns_ownership` | Function returns a newly allocated pointer | Allocations created inside are NOT tracked locally; the caller tracks them |
| `#takes_ownership` | Function takes ownership of pointer arguments | Pointer arguments are tracked as freed (ownership transferred to callee) |

---

## 10. Error Reporting

### 10.1 Three Error Functions

All in `error.c`, using the global error system in `src/c_libs/error/`:

| Function | Signature | Use Case |
|---|---|---|
| `tc_error` | `(node, type, fmt, ...)` | Basic errors |
| `tc_error_help` | `(node, type, help, fmt, ...)` | Errors with guidance text |
| `tc_error_id` | `(node, id, type, fmt, ...)` | Errors highlighting a specific identifier |

### 10.2 Error Information

The underlying `ErrorInformation` struct captures:
- `error_type` (e.g., "Type Error", "Call Error", "Use After Free")
- `file_path`, `line`, `col`, `token_length`
- `message` (formatted)
- `help` (guidance text)
- `note` (additional context)
- `line_text` (source line with caret, extracted from tokens via `generate_line()`)

### 10.3 Global State

Three globals hold the current module context:
```c
Token *g_tokens;          // current module's token array
int g_token_count;
const char *g_file_path;  // current source file path
```

These are set by `process_module_in_order()` before each module is processed.

---

## 11. Control Flow Analysis (Current Limitations)

The typechecker has **limited** control flow awareness:

| Feature | Support Status |
|---|---|
| Early returns | ✅ Conditional free tracking prevents false double-free on `if { free(x); return; } free(x);` |
| Conditional branches | ✅ Deferred frees in branches are tracked as conditional |
| Loops | ✅ Loop bodies create child scopes, deferred frees on exit |
| **Unreachable code detection** | ❌ Not implemented |
| **Definite initialization** | ❌ Not implemented |
| **Nested defer** | ❌ Only the innermost defer's `skip_memory_tracking` is active |

---

## 12. Edge Cases & Known Issues

### 12.1 Known Limitations

1. **No cycle detection in module dependency graph** — circular `@use` causes stack overflow
2. **`sizeof` doesn't compute actual size** — returns `int` without evaluating
3. **`memcpy` is a stub** — `typecheck_memcpy_expr` returns NULL
4. **No generics/polymorphism** — all types are monomorphic
5. **No trait/interface system** — struct methods are just functions with `self` prepended
6. **Growable arrays use memcpy-based push** — pointers to elements invalidate on reallocation (the `Symbol*` returned by `scope_lookup` may become stale after a push)
7. **Linear symbol lookup** — O(n) per lookup, no hash table
8. **No bounds checking for non-constant indices** — only literal indices are checked
9. **`type_to_string` omits array sizes** — error messages for array size mismatches show `int[]` on both sides
10. **Memory analyzer uses flat list** — allocations from different functions distinguished only by name matching; if two functions allocate into the same variable name, tracking may cross-contaminate
11. **No `break`/`continue` validation** — doesn't verify they're inside a loop
12. **AST mutation** — method calls modify `expr->expr.call.args` and `arg_count` in-place, which could cause issues if the AST is reused

### 12.2 Design Quirks

- **Enums as basic types**: Enum types are `AST_TYPE_BASIC` with the enum name, not a separate type variant. This means enum ↔ int compatibility falls out of the basic-type matching logic.
- **Struct methods use qualified names**: Methods are stored as `StructName.methodName` global symbols, not scoped within the struct. This is flat, not hierarchical.
- **`associated_node` on scopes**: The function scope's `associated_node` points to the `func_decl` AST node. Many helpers walk the scope chain to find it (e.g., `get_current_function_name`, `get_enclosing_function_return_type`).
- **Config propagation**: `scope->config` is set by `typecheck()` and propagated down to child scopes during initialization. The `@os` blocks re-walk the scope chain to find config if not set.
- **Token re-lexing**: The memory leak/double-free reporter re-lexes the source file to generate line text (since the original tokens are for a different module context at that point).
- **`#dll_import` handling**: Body-less function declarations with `is_dll_import` skip body typechecking.
- **`create_function_type` and `create_struct_type`**: These are defined in `type.c` but declared in `type.h` — they create `AstNode` type nodes programmatically.

---

## 13. Key Design Decisions for the Self-Hosted Port

### 13.1 What to Keep

- **3-pass module system** — registering scopes first, then imports, then bodies in dependency order is clean and handles mutual references
- **5 sub-passes per module** — forward decls → OS symbols → main body → OS bodies → memory analysis is well-proven
- **Scope chain with parent pointers** — simple and effective for lexical scoping
- **Nominal struct typing** — match by name, fits the language design
- **Separate data fields from methods** in struct declarations — enables pre-registering the struct type before method bodies are checked
- **Static memory analysis** — alloc/free tracking, use-after-free, double-free, leak detection are valuable features
- **`skip_memory_tracking` flag for defer** — simple but effective mechanism

### 13.2 What to Improve

1. **Use a hash map for symbol lookup** instead of linear search through growable arrays
2. **Add generics** — the type system needs parameterized types
3. **Add proper enum type variant** instead of overloading `AST_TYPE_BASIC`
4. **Track types more precisely** — `sizeof` should be a compile-time constant, not just type-checked
5. **Add control flow analysis** — definite initialization, unreachable code
6. **Improve memory analysis**:
   - Track allocations per-function properly (not flat by name)
   - Handle `realloc()`, `calloc()` patterns
   - Track pointer arithmetic (not just direct aliases)
7. **Add cycle detection** in module dependency graph
8. **Separate type representation from AST nodes** — types should be a clean, immutable IR rather than reusing the AST node structure
9. **Don't mutate the AST** during typechecking — use a separate type environment or annotations
10. **Better error messages** — include `help` text on more errors, show type information
11. **Handle `break`/`continue` nesting validation**
12. **Incremental typechecking** support for the LSP

### 13.3 Self-Hosted Implementation Strategy (Phase Order)

1. **Core infrastructure**: Arena allocator, growable arrays/vectors, string interning, error system
2. **Type representation**: `Type` enum or struct (not tied to AST nodes); `types_match()`, type introspection, `type_to_string()`
3. **Scope management**: `Scope` struct, symbol table (hash map), lookup with visibility, child scope creation
4. **Module system**: Module registration, imports, dependency graph with cycle detection, topological sort
5. **Expression type-checking**: Literals → identifiers → binary/unary → calls → member/index/deref/addr → alloc/free/cast → arrays/structs → special forms
6. **Statement type-checking**: Var decl → func decl → struct decl → enum decl → if/loops/switch → return → defer → blocks
7. **Memory analysis**: Allocation tracking → free tracking → alias tracking → use-after-free → double-free → leak detection → deferred frees → ownership annotations
8. **Edge cases & verification**: All existing error tests pass; self-hosted compiler typechecks valid programs correctly; rejects all invalid programs with proper errors

### 13.4 Testing Strategy

The tests in `tests/errors/` are the definitive test suite for error detection. The chess engine, tetris, rotating cube, and VM tests are integration tests for correct programs. Use both as the benchmark for the self-hosted typechecker.
