# Luma Typechecker Specification

## 1. Architecture Overview

The typechecker is invoked after parsing and module combination. It performs semantic analysis: type checking, symbol resolution, scope management, and static memory analysis (use-after-free, double-free, leaks).

```
Parsing → Combine Modules → Typecheck (3-pass) → LLVM Codegen
```

### Entry Point

`typecheck()` in `src/typechecker/tc.c` dispatches based on `node->category`:

| Category | Handler |
|---|---|
| `STMT` | `typecheck_statement()` |
| `EXPR` | `typecheck_expression()` → returns inferred `AstNode*` type |
| `TYPE` | Pass-through (types are validated structurally) |
| `PREPROCESSOR` | Dispatches to module/use/os/link handlers |

### 3-Pass Module System (`typecheck_program_multipass`)

```
Pass 1: Register all module scopes (creates empty namespaces)
Pass 2: Resolve all @use statements (import links)
Pass 3: Typecheck module bodies in dependency order (topological sort)
```

Within Pass 3 for each module:
```
Pass 3a: Forward declarations (function prototypes)
Pass 3b: Pre-register @os symbols (functions + constants)
Pass 3c: All other statements
Pass 3d: @os blocks + @link directives
Pass 3e: Static memory leak/double-free reporting
```

## 2. Core Data Structures

### Type Representation

Types are `AstNode` tagged union nodes with `category = Node_Category_TYPE`:

```luma
enum NodeType {
  // Types (7 variants)
  AST_TYPE_BASIC,     // { name: str }              e.g. int, float, bool
  AST_TYPE_POINTER,   // { pointee_type: AstNode* }  e.g. int*
  AST_TYPE_ARRAY,     // { element_type, size }      e.g. int[10]
  AST_TYPE_FUNCTION,  // { param_types[], return_type }  e.g. fn(int, int) int
  AST_TYPE_STRUCT,    // { name, member_types[], member_names[] }
  AST_TYPE_ENUM,      // (represented as basic type with enum name)
  AST_TYPE_RESOLUTION // { parts[], part_count }     e.g. Module::Type
}
```

**Type Match Result**: `TYPE_MATCH_EXACT`, `TYPE_MATCH_COMPATIBLE`, `TYPE_MATCH_NONE`

**Implicit conversions** (return `COMPATIBLE`):
- `enum → int`, `int → enum`
- `float ↔ double`
- `str ↔ char*` (string/pointer compatibility)
- Arrays decay to pointers (element type check)
- Function ↔ pointer-to-function

### Symbol Table

```luma
struct Symbol {
  name: str,
  type: AstNode*,           // Type of the symbol
  is_public: bool,
  is_mutable: bool,
  scope_depth: int,
  returns_ownership: bool,  // For functions: #returns_ownership
  takes_ownership: bool,    // For functions: #takes_ownership
}
```

### Scope Hierarchy

```luma
struct Scope {
  parent: Scope*,
  symbols: GrowableArray<Symbol>,
  children: GrowableArray<Scope*>,
  scope_name: str,
  depth: int,
  is_function_scope: bool,
  associated_node: AstNode*,    // The AST node that created this scope
  is_module_scope: bool,
  module_name: str,
  imported_modules: GrowableArray<ModuleImport>,
  memory_analyzer: StaticMemoryAnalyzer*,
  deferred_frees: GrowableArray<str>,
  link_libs: GrowableArray<str>,
  config: BuildConfig*,
}
```

**Scope lookup algorithm** (`scope_lookup_with_visibility`):
1. Walk up the scope chain (current → parent → ... → global)
2. For each scope, search symbols linearly
3. Check visibility: public symbols always visible; private symbols only within same module
4. Also search imported modules' scopes (recursive through module imports)
5. Return first match or NULL

**Symbol addition** checks for duplicates only in current scope (inner scopes can shadow).

**Key scopes created**:
- `global` — root scope for all modules
- `module` — per-module scope (is_module_scope = true)
- `function` — per-function scope (is_function_scope = true, associated_node = func_decl)
- `block` — for `{ }` blocks
- `then_branch`, `else_branch`, `elif_branch` — for if/elif/else
- `loop` — for infinite/while/for loops
- `switch`, `case`, `default` — for switch/case/default
- `for_loop` — for loop initializers

### Module System

```luma
struct ModuleImport {
  module_name: str,
  alias: str,           // The name used to access this module
  module_scope: Scope*,
}

struct ModuleDependency {
  module_name: str,
  dependencies: GrowableArray<str>,  // Module names from @use
  processed: bool,
}
```

Modules are registered with `__module_<name>` prefix in global scope. Dependencies are resolved via topological sort.

### Memory Tracker

```luma
struct StaticMemoryAnalyzer {
  allocations: GrowableArray<StaticAllocation>,
  arena: ArenaAllocator*,
  skip_memory_tracking: bool,  // Set true during defer blocks
}

struct StaticAllocation {
  line, column: int,
  variable_name: str,
  has_matching_free: bool,
  free_count: int,
  conditional_free_count: int,
  aliases: GrowableArray<str>,
  reported: bool,
  address_taken: bool,
  function_name: str,
  file_path: str,
}
```

## 3. Type Checking Rules

### Literals

| Literal | Inferred Type |
|---|---|
| `42` | `int` |
| `3.14` | `double` |
| `"hello"` | `str` |
| `true` / `false` | `bool` |
| `'x'` | `char` |
| `null` | `null` |

### Identifiers

Look up in scope chain. Error: "Undefined Identifier" if not found.

### Binary Operations

**Arithmetic** (`+`, `-`, `*`, `/`, `%`, `**`):
- Both operands must be numeric (`int`, `float`, `double`, `char`)
- Modulo (`%`) requires integer types only
- Type promotion: `double > float > int`
- Returns the promoted type

**Comparison** (`==`, `!=`, `<`, `<=`, `>`, `>=`):
- Both operands must have compatible types
- Returns `bool`

**Logical** (`&&`, `||`):
- Both operands must be `bool`
- Returns `bool`

**Range** (`..`):
- Both operands must be `int`
- Returns `int[size]` array type (inclusive range)

**Bitwise** (`&`, `|`, `^`, `<<`, `>>`):
- Both operands must be numeric
- Returns `int`

### Unary Operations

| Op | Constraint | Result Type |
|---|---|---|
| `!` | any | `bool` |
| `-` | numeric | same as operand |
| `~` | numeric | same as operand |
| `++`/`--` (pre/post) | numeric | same as operand |

### Assignment (`=`)

1. Type-check target (must be mutable identifier)
2. Type-check value
3. `types_match(target_type, value_type)` — error on mismatch
4. Handle `#returns_ownership` call tracking for memory analysis
5. Handle pointer alias tracking for memory analysis

### Function Calls

1. Resolve callee:
   - **Simple identifier**: `scope_lookup` by name
   - **Member access with `::`**: compile-time lookup via `lookup_qualified_symbol` (module or enum member)
   - **Member access with `.`**: runtime method call on struct instance
2. Verify callee is function type
3. Match argument count to parameter count
4. Type-check each argument against parameter type
5. Handle array padding for function parameters
6. Handle `#takes_ownership` — track argument frees (including `self` for methods)
7. Return the function's return type

**Method call transformation**:
For `obj.method(args...)`:
- Type-check `obj` and resolve to struct type
- Look up `method` in struct members
- Prepend `self` (address of `obj`) as first argument
- If method expects pointer but obj is value, auto-apply `&`

### Index Expression (`a[i]`)

1. Check use-after-free on the base variable
2. Type-check object — must be array, pointer, or `string`
3. Type-check index — must be integer type
4. Return element type (or `char` for string)

### Member Access (`a.b` / `a::b`)

**Compile-time (`::`)**:
- Module access: `lookup_qualified_symbol`
- Enum member access: look up `Enum.Member` qualified name
- Supports chaining: `Module::Type::Member`

**Runtime (`.`)** :
- Type-check base expression
- Resolve through pointers to underlying struct type
- Look up member name in struct type's member list
- Return member type

### Dereference (`*ptr`)

1. Check use-after-free
2. Verify operand is pointer type
3. Return pointee type

### Address-of (`&expr`)

1. Verify expr is an lvalue (identifier, member, index, deref)
2. Mark address as taken in memory tracker
3. Return `T*` where T is operand type

### Alloc (`alloc(size)`)

1. Verify size is numeric
2. Return `void*` (generic pointer)
3. Memory tracker records this allocation

### Free (`free(ptr)`)

1. Verify ptr is pointer type
2. Memory tracker records the free
3. Within defer blocks, adds to deferred_frees list
4. Return `void`

### Cast (`cast<Type>(expr)`)

1. Type-check the casted expression
2. `is_cast_valid()` checks:
   - Matching/compatible types
   - Numeric ↔ Numeric
   - Pointer ↔ Pointer
   - Pointer ↔ Numeric
   - Function ↔ Pointer (function pointers)
   - Struct to itself (identity)
   - Any type → void
   - NOT: void → anything
3. Return target type

### Struct Literal (`Type { x: 1, y: 2 }`)

Named: Look up struct type, validate each field exists and matches type.
Anonymous: If expected type is known, validate against it; otherwise create anonymous struct type.

### Array Literal (`[1, 2, 3]`)

1. Type-check first element to establish element type
2. All remaining elements must match
3. Return `element_type[count]`

### Input (`input<Type>("prompt")`)

Type must be a supported basic type (int, float, double, string, char, bool). Message must be string.

### System (`system("cmd")`)

Command must be string. Returns `int`.

### Syscall (`syscall(num, args...)`)

First arg is syscall number (numeric). Remaining args should be numeric or pointer. Returns `int`.

### Sizeof (`sizeof(Type)` / `sizeof(expr)`)

Always returns `int`.

## 4. Statement Type Checking

### Variable Declaration

1. If initializer contains `alloc()`: memory tracker records allocation
2. If initializer is a `#returns_ownership` call: track ownership transfer
3. If declared type provided: `types_match(declared, init)`
4. If no declared type: infer from initializer
5. Must have either type annotation or initializer
6. Add to scope

### Function Declaration

1. Validate return type
2. `main` function validation:
   - Must return `int`
   - 0 or 2 parameters (argc: int, argv: **byte)
   - Must be public
3. Create function type and add to scope
4. Support forward declarations (prototype then implementation)
   - Match signature + ownership flags between prototype and implementation
5. Create function scope, add parameters
6. Type-check body
7. Process deferred frees

### Struct Declaration

1. Validate name, check for duplicates
2. Collect data fields (not methods) first
3. Create struct type and add to scope
4. Process methods:
   - Create method scope with implicit `self: StructType*` parameter
   - Type-check method body
   - Register qualified name: `StructName.methodName`
   - Add method to struct's member list

### Enum Declaration

1. Create basic type node with enum name as type
2. Add `EnumName` to scope
3. For each member, add `EnumName.memberName` to scope

### If/Elif/Else

1. Condition must be `bool`
2. Type-check then branch, elif branches, else branch
3. Process deferred_frees in each branch

### Loops

1. **Infinite loop**: No condition, no initializer
2. **While loop**: Has condition (type-checked), body
3. **For loop**: Has initializer(s), condition, body, optional increment

### Switch/Case/Default

1. Type-check condition
2. Detect enum switches (condition is an enum type)
3. Type-check each case:
   - Values must match condition type
   - For enum switches: values must be `EnumName::Member` format
4. Track covered enum members for exhaustiveness checking
5. Check for duplicate cases
6. Default case is optional (but may be required for non-exhaustive switches)

### Return

1. Must be inside a function scope
2. Void function: must not return value
3. Non-void function: must return value matching return type
4. Check transitive ownership: if returning from `#returns_ownership` call, warn if this function doesn't have the annotation

### Defer

1. Run in the same scope (not a child scope)
2. Enable `skip_memory_tracking` during defer body type-checking
3. Free operations in defer block add to `deferred_frees` list
4. When scope exits, deferred frees are processed as conditional frees

## 5. Static Memory Analysis

### Tracking Allocations

Allocations are tracked when:
- `alloc()` is assigned to a variable (in var_decl or assignment)
- A `#returns_ownership` function call is assigned to a variable

NOT tracked when inside a `#returns_ownership` function (ownership transfers to caller).

### Tracking Frees

Frees are tracked when:
- `free(ptr)` is called
- A `#takes_ownership` function receives a pointer argument
- A method with `#takes_ownership` receives `self`

### Alias Tracking

When a pointer variable is assigned from another pointer:
```
ptr2 = ptr1  // ptr2 becomes an alias of ptr1
```
Both variables share the same `StaticAllocation` record. Freeing one affects the other.

### Deferred Frees

In `defer { free(ptr); }`:
- The free is NOT tracked immediately
- The variable is added to `deferred_frees` list on the function scope
- When the containing scope exits, deferred frees are processed as conditional frees

### Violations Detected

| Check | When |
|---|---|
| Use-after-free | Variable accessed after free (identifier, deref, index, member) |
| Double free | Variable freed multiple times (unconditional frees) |
| Memory leak | Allocation with no matching free (unless address was taken) |
| Freeing non-allocated pointer | `free(&stack_var)` detected |

## 6. Error Reporting

Three error functions, all using the global error system:

| Function | Usage |
|---|---|
| `tc_error(node, type, fmt, ...)` | Basic error |
| `tc_error_help(node, type, help, fmt, ...)` | Error with help text |
| `tc_error_id(node, id, type, fmt, ...)` | Error highlighting an identifier |

The error system extracts source line from tokens and displays it with caret pointing to the error location. The `error_add()` function collects errors and the compiler reports them at the end of typechecking.

## 7. Type Utilities

| Function | Behavior |
|---|---|
| `types_match(t1, t2)` | Returns EXACT/COMPATIBLE/NONE. Recursively checks pointers, arrays, functions. Struct matching is nominal (by name). |
| `is_numeric_type(t)` | `int`, `float`, `double`, `char` |
| `is_integer_type(t)` | `int`, `char` |
| `is_pointer_type(t)` | `AST_TYPE_POINTER` |
| `is_array_type(t)` | `AST_TYPE_ARRAY` |
| `is_void_type(t)` | `basic("void")` |
| `is_bool_type(t)` | `basic("bool")` |
| `is_struct_type_node(t)` | `AST_TYPE_STRUCT` |
| `is_function_type(t)` | `AST_TYPE_FUNCTION` |
| `is_cast_valid(from, to)` | Checks all valid cast conversions |
| `is_pointer_to_function_type(t)` | Pointer → function type |
| `get_element_type(array_or_ptr)` | Decays array/ptr to element type |
| `type_to_string(t)` | Pretty-prints type (e.g., `fn(int, int) int`) |

## 8. Implementation Checklist for Self-Hosted Port

### Phase 1: Core Infrastructure
- [ ] GrowableArray equivalent
- [ ] ArenaAllocator equivalent
- [ ] Error reporting system
- [ ] String manipulation utilities (strdup, comparison)

### Phase 2: Type System
- [ ] `TypeMatchResult` enum and `types_match()`
- [ ] Type introspection functions (`is_numeric_type`, etc.)
- [ ] `type_to_string()` pretty printer
- [ ] Type node creation functions (basic, pointer, array, function, struct)

### Phase 3: Scope Management
- [ ] `Scope` struct with parent/children/symbols
- [ ] `init_scope()` / `create_child_scope()`
- [ ] `scope_add_symbol()` with duplicate detection
- [ ] `scope_lookup()` with visibility rules
- [ ] `find_containing_module()`

### Phase 4: Expression Type Checking
- [ ] `typecheck_expression()` dispatcher
- [ ] Literals and identifiers
- [ ] Binary, unary, assignment
- [ ] Function calls (including method calls with self insertion)
- [ ] Member access, index, deref, address-of
- [ ] Alloc, free, memcpy, cast
- [ ] Input, system, syscall, sizeof
- [ ] Array literals, struct literals

### Phase 5: Statement Type Checking
- [ ] `typecheck_statement()` dispatcher
- [ ] Variable and constant declarations
- [ ] Function declarations (including main validation, forward decls)
- [ ] Struct declarations (fields + methods)
- [ ] Enum declarations
- [ ] If/elif/else, loops, switch/case/default
- [ ] Return with ownership checking
- [ ] Defer blocks

### Phase 6: Module System
- [ ] Module/use/os/link preprocessor handling
- [ ] 3-pass multi-module typechecking
- [ ] Dependency graph and topological sort
- [ ] Qualified symbol lookup across modules
- [ ] Visibility rules (public/private)

### Phase 7: Static Memory Analysis
- [ ] `StaticMemoryAnalyzer` with allocation tracking
- [ ] `track_alloc()` / `track_free()` / `track_alias()`
- [ ] Use-after-free detection
- [ ] Double-free detection
- [ ] Memory leak detection
- [ ] Deferred free handling
- [ ] Alias tracking for pointer assignments
- [ ] `#returns_ownership` / `#takes_ownership` support

### Phase 8: Edge Cases & Verification
- [ ] Error test cases from `tests/errors/` all pass
- [ ] Self-hosting: typechecker compiles valid Luma programs correctly
- [ ] Self-hosting: typechecker rejects all invalid programs with proper errors
