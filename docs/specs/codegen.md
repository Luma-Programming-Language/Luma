# Luma LLVM Code Generation Specification

## 1. Architecture Overview

The code generator converts a type-checked AST into LLVM IR using the LLVM C API. It runs after the typechecker succeeds and produces an object file per module.

```
Typechecked AST → Codegen Context → LLVM IR (per module) → Object files → Link
```

### Entry Point

```c
bool generate_program_modules(AstNode *program, CodeGenContext *context);
```

Called from `generate_llvm_code_modules()` in `run.c` after setting up the `CodeGenContext`. Uses `LLVMGetGlobalContext()` and LLVM's C API (`llvm-c/Core.h`, `llvm-c/Analysis.h`, `llvm-c/TargetMachine.h`).

### Build Pipeline (10 stages)

From `run_build()` in `src/helper/run.c`:

1. Lexing source files
2. Parsing into AST modules
3. Combining modules into one program
4. **Typechecking** (3-pass)
5. **LLVM IR generation** (this subsystem)
6. Stack save
7. Optimize
8. Emit object files
9. Link with system linker
10. Finalize output

## 2. Core Data Structures

### CodeGenContext

```c
struct CodeGenContext {
  LLVMContextRef context;       // LLVM context
  LLVMTargetMachineRef tm;      // Target machine
  LLVMTargetDataLayoutRef layout;  // Data layout info
  LLVMModuleRef current_module; // Module being generated
  LLVMBuilderRef builder;       // IR builder
  Scope *global_scope;          // From typechecker
  const char *file_path;
  ArenaAllocator *arena;
  GrowableArray module_units;   // ModuleCompilationUnit[]
  GrowableArray all_modules;    // All generated modules
  GrowableArray object_files;   // Output object file paths
  LLVMValueRef current_function;
  LLVMBasicBlockRef current_block;
  bool has_error;
  bool has_main;
  DeferredInfo deferred;        // Defer statement stack

  // Caches
  CommonTypes common_types;
  GrowableArray struct_cache;    // StructInfo[]
  GrowableArray enum_cache;     // Enum type detection
  GrowableArray symbol_cache;   // LLVM_Symbol[]
  GrowableArray global_strings; // Cached global string constants
  bool struct_cache_ready;
}
```

### ModuleCompilationUnit

```c
struct ModuleCompilationUnit {
  const char *module_name;
  const char *file_path;
  LLVMModuleRef llvm_module;
  AstNode *module_ast;
  Scope *module_scope;
  bool processed;
  GrowableArray dependencies;  // const char* (module names)
  GrowableArray imported_symbols;   // LLVM_Symbol*
  GrowableArray exported_symbols;   // LLVM_Symbol*
  GrowableArray struct_defs;        // StructInfo*
  GrowableArray symbol_table;       // LLVM_Symbol[]
}
```

### LLVM_Symbol

```c
struct LLVM_Symbol {
  const char *name;
  LLVMValueRef value;
  bool is_global;
}
```

### StructInfo

```c
struct StructInfo {
  const char *name;
  LLVMTypeRef llvm_type;
  GrowableArray field_names;  // const char*
  GrowableArray field_types;  // LLVMTypeRef
  size_t field_count;
  bool is_packed;
}
```

### CommonTypes (Cached LLVM Types)

```c
struct CommonTypes {
  LLVMTypeRef i1, i8, i16, i32, i64;   // Integer types
  LLVMTypeRef f32, f64;                 // Float types
  LLVMTypeRef void_type;
  LLVMTypeRef i8_ptr;                   // char* (i8*)
}
```

### DeferredInfo

```c
struct DeferredInfo {
  DeferredEntry stack[64];  // Fixed-size stack
  int count;
}

struct DeferredEntry {
  LLVMBasicBlockRef alloc_block; // block where deferred values are stored
  LLVMValueRef *values;          // live variable snapshots
  int value_count;
  AstNode *defer_stmt;           // the defer statement AST
}
```

## 3. Dispatch Functions

### `codegen_expr()`

Returns `LLVMValueRef`. Dispatches by `expr->type`:

| AST Node | Handler |
|---|---|
| `AST_EXPR_LITERAL` | `codegen_literal()` |
| `AST_EXPR_IDENTIFIER` | `codegen_identifier()` |
| `AST_EXPR_BINARY` | `codegen_binary_expr()` |
| `AST_EXPR_UNARY` | `codegen_unary_expr()` |
| `AST_EXPR_CALL` | `codegen_call_expr()` |
| `AST_EXPR_ASSIGNMENT` | `codegen_assignment_expr()` |
| `AST_EXPR_MEMBER` | `codegen_member_expr()` |
| `AST_EXPR_INDEX` | `codegen_index_expr()` |
| `AST_EXPR_DEREF` | `codegen_deref_expr()` |
| `AST_EXPR_ADDR` | `codegen_addr_expr()` |
| `AST_EXPR_CAST` | `codegen_cast_expr()` |
| `AST_EXPR_ALLOC` | `codegen_alloc_expr()` |
| `AST_EXPR_FREE` | `codegen_free_expr()` |
| `AST_EXPR_INPUT` | `codegen_input_expr()` |
| `AST_EXPR_SYSTEM` | `codegen_system_expr()` |
| `AST_EXPR_SYSCALL` | `codegen_syscall_expr()` |
| `AST_EXPR_SIZEOF` | `codegen_sizeof_expr()` |
| `AST_EXPR_ARRAY` | `codegen_array_expr()` |
| `AST_EXPR_STRUCT` | `codegen_struct_expr()` |
| `AST_EXPR_TERNARY` | `codegen_ternary_expr()` |
| `AST_EXPR_RANGE` | `codegen_range_expr()` |

### `codegen_stmt()`

Returns `bool`. Dispatches by `stmt->type`:

| AST Node | Handler |
|---|---|
| `AST_PROGRAM` | `codegen_program_modules()` |
| `AST_STMT_VAR_DECL` | `codegen_var_decl()` |
| `AST_STMT_FUNCTION` | `codegen_func_decl()` |
| `AST_STMT_STRUCT` | `codegen_struct_decl()` |
| `AST_STMT_ENUM` | `codegen_enum_decl()` |
| `AST_STMT_IF` | `codegen_if_decl()` |
| `AST_STMT_LOOP` | `codegen_loop_decl()` |
| `AST_STMT_RETURN` | `codegen_return_stmt()` |
| `AST_STMT_EXPRESSION` | `codegen_expr()` |
| `AST_STMT_BLOCK` | `codegen_block()` |
| `AST_STMT_PRINT` | `codegen_print_stmt()` |
| `AST_STMT_SWITCH` | `codegen_switch_stmt()` |
| `AST_STMT_DEFER` | `codegen_defer_stmt()` |
| `AST_STMT_BREAK_CONTINUE` | `codegen_break_continue()` |

### `codegen_type()`

Returns `LLVMTypeRef`. Dispatches by type AST node:

| AST Type | LLVM Type |
|---|---|
| `AST_TYPE_BASIC("int")` | `i64` (or `i32` on some platforms) |
| `AST_TYPE_BASIC("float")` | `float` |
| `AST_TYPE_BASIC("double")` | `double` |
| `AST_TYPE_BASIC("bool")` | `i1` |
| `AST_TYPE_BASIC("char")` | `i8` |
| `AST_TYPE_BASIC("void")` | `void` |
| `AST_TYPE_BASIC("str")` | `i8*` |
| `AST_TYPE_POINTER` | pointer to pointee type |
| `AST_TYPE_ARRAY` | array type with element count |
| `AST_TYPE_FUNCTION` | function pointer type |
| `AST_TYPE_STRUCT` | named struct type (from cache) |

## 4. Expression Code Generation

### Literals

| Literal | LLVM Value |
|---|---|
| `LITERAL_INT` | `LLVMConstInt(i64, value)` |
| `LITERAL_FLOAT` | `LLVMConstReal(double, value)` |
| `LITERAL_BOOL` | `LLVMConstInt(i1, value)` |
| `LITERAL_CHAR` | `LLVMConstInt(i8, value)` |
| `LITERAL_STRING` | global string constant via `build_global_string()` |
| `LITERAL_NULL` | `LLVMConstNull(i8*)` |

### Identifiers

1. Look up variable in LLVM symbol table
2. If it's an alloca (stack variable): load it
3. If it's a global: load it
4. If it's a function: return the function value

### Binary Operations

| Op Category | Implementation |
|---|---|
| **Arithmetic** (`+`, `-`, `*`, `/`) | `LLVMBuildAdd/Sub/Mul/SDiv/FAdd/FSub/FMul/FDiv` with type promotion |
| **Modulo** (`%`) | `LLVMBuildSRem` (signed) for int, error for float |
| **Comparison** (`==`, `!=`, `<`, `<=`, `>`, `>=`) | `LLVMBuildICmp/FCmp` with appropriate predicates |
| **Logical** (`&&`, `||`) | short-circuit with `LLVMBuildAnd/Or` on `i1` |
| **Bitwise** (`&`, `|`, `^`, `<<`, `>>`) | `LLVMBuildAnd/Or/Xor/Shl/AShr` |
| **Range** (`..`) | allocates array on stack, fills with sequential int values via loop |

Type promotion follows: `int < float < double`. If either operand is double, both become double.

### Unary Operations

| Op | Implementation |
|---|---|
| `-` (neg) | `LLVMBuildNeg/FNeg` |
| `!` (not) | `LLVMBuildNot` on `i1` |
| `~` (bit not) | `LLVMBuildNot` |
| `++` / `--` (pre/post) | load, add/sub 1, store, return new (pre) or old (post) value |

### Function Calls

1. If method call (callee is `AST_EXPR_MEMBER` with `.` access):
   - Prepend `self` argument (pointer to struct instance)
   - Handle `self` auto-address-of if needed
   - Resolve function through struct member lookup
2. If compile-time call (callee is `AST_EXPR_MEMBER` with `::` access):
   - Look up function in module scope via `lookup_codegen_symbol_in_module()`
3. Otherwise resolve by function name
4. Handle `#dll_import` / `#lib_import` through LLVM `build_call` with proper calling convention
5. Handle struct return convention (sret): if return type is struct, use hidden pointer parameter
6. Build call via `LLVMBuildCall2()`
7. Handle `#takes_ownership` / `#returns_ownership` by inserting `free()` calls

### Assignment

| Target Type | Implementation |
|---|---|
| Identifier (stack var) | evaluate value, store via `LLVMBuildStore` |
| Identifier (global) | evaluate value, store to global |
| Pointer deref (`*ptr = val`) | store to dereferenced pointer |
| Array index (`arr[i] = val`) | GEP to element, store |
| Struct member (`obj.field = val`) | GEP to field, store |

Returns the stored value.

### Array Expressions

1. Alloca array of appropriate size on stack
2. For range expressions (`0..5`): fill with loop
3. Otherwise: copy element values from array literal
4. Handle padding to match target size for function calls

### Index Expressions

1. GEP into array or pointer with bounds checking (LLVM `inbounds`)
2. For nested arrays (multi-dimensional): chain GEPs
3. For pointer + index: pointer arithmetic
4. For member + index (`arr[i].field`): GEP then member access

### Member Expressions

**Runtime (`.`):**
1. GEP into struct by field index
2. Load the field value
3. Support chaining (`a.b.c`) via recursive GEP

**Compile-time (`::`):**
- For module functions: look up in module symbol table
- For enum members: get the integer constant value

### Dereference (`*ptr`)

1. Load via `LLVMBuildLoad2` with pointee type
2. Handle `_Deref_memcpy` marker nodes (synthesized deref + memcpy)

### Address-of (`&expr`)

1. For identifiers: return the alloca/global pointer
2. For array subscript: return GEP result

### Cast

Uses LLVM instructions based on source/target types:
- Int-to-int: `LLVMBuildIntCast2` (sign-extend or truncate)
- Float-to-float: `LLVMBuildFPCast`
- Int-to-float: `LLVMBuildSIToFP` / `LLVMBuildUIToFP`
- Float-to-int: `LLVMBuildFPToSI`
- Pointer-to-int: `LLVMBuildPtrToInt`
- Int-to-pointer: `LLVMBuildIntToPtr`
- Pointer-to-pointer: `LLVMBuildBitCast`

### Alloc (`alloc(size)`)

`LLVMBuildCall` to `malloc` (or `calloc` on some platforms). Returns `i8*`.

### Free (`free(ptr)`)

`LLVMBuildCall` to `free`. Casts pointer to `i8*` if needed.

### Input (`input<Type>("msg")`)

Generates `printf` for the prompt, then `scanf` with appropriate format specifier:
- `int` → `scanf("%lld", ...)` or `scanf("%d", ...)`
- `float` → `scanf("%f", ...)`
- `double` → `scanf("%lf", ...)`
- `string` / `char` → `scanf("%s", ...)` / `scanf(" %c", ...)`

### System (`system("cmd")`)

`LLVMBuildCall` to libc `system()`. Returns `int`.

### Syscall

- **Linux**: inline LLVM IR `call i64 asm "syscall" ...` with register constraints
- **macOS**: wraps `syscall()` from libc

### Sizeof

Returns constant integer matching the size in bytes (via LLVM target data layout).

### Ternary (`? :`)

`select i1 cond, then_val, else_val` or branch-based if either side has side effects.

### Struct Expressions

1. Alloca temporary for the struct
2. For each named field: GEP to field, store value with type conversion
3. Load the full struct value

## 5. Statement Code Generation

### Variable Declaration

1. If global scope: `LLVMAddGlobal()` with initializer
2. If function scope: `LLVMBuildAlloca()` for stack space
3. If has initializer: evaluate and store
4. Register in LLVM symbol table
5. Track pointer-to-array element types for codegen

### Function Declaration

1. Create LLVM function type from AST function type
2. Handle struct return (sret): add hidden pointer parameter
3. Handle `#dll_import`: set DLL storage class + calling convention
4. Handle `#lib_import`: declare as external with proper linkage
5. Create function scope LLVM basic block
6. Allocate/decompose struct parameters
7. Store parameters via alloca + store (to have mutable stack slots)
8. Handle forward declarations (body is NULL or separate)
9. Generate body:
   - Create entry block + allocate space for all variables
   - Set up defer stack
   - Generate all statements
   - For `#returns_ownership` returning pointer: insert `noalias` attribute
10. Apply function attributes (always_inline for tiny functions, etc.)

### Enum Declaration

Each enum member is added as a global constant (`LLVMAddGlobal`):
- Member `Name.MEMBER` → global `i64` with value = member index
- Used as immutable constants

### Struct Declaration

1. Create LLVM named struct type (`LLVMStructCreateNamed`)
2. Set body with member types (`LLVMStructSetBody`)
3. Register in struct cache for member access
4. Generate method functions with implicit `self: *StructType` first parameter

### If / Elif / Else

1. Evaluate condition (must be `i1` or convertible)
2. Create then/else/merge basic blocks
3. `LLVMBuildCondBr` to then/else
4. Generate then branch body
5. Generate else branch (or elif chain recursively)
6. Branch to merge block
7. Set current block to merge

### Loops

**Infinite loop:**
1. Create loop header block + after block
2. Branch to header
3. Generate body
4. Branch back to header
5. Set current block to after

**While loop:**
1. Create cond/body/after blocks
2. Branch to cond
3. Evaluate condition, `LLVMBuildCondBr` to body or after
4. Generate body, branch back to cond
5. Set current block to after

**For loop:**
1. Generate initializer statements
2. Same as while for condition + body
3. Generate optional (increment) after body, before back-edge

### Break / Continue

- `break`: branch to after-loop block (tracked in loop context)
- `continue`: branch to loop-cond block (tracked in loop context)

### Return

1. Execute deferred statements (defer stack unwind)
2. If returning value: `LLVMBuildRet(value)`
3. If void: `LLVMBuildRetVoid()`
4. Handle `#returns_ownership`: mark returned pointer with `noalias`

### Print

Generates `printf` calls with format strings built from argument types:
- `int` → `"%lld "` or `"%d "`
- `float` → `"%f "`
- `double` → `"%lf "`
- `string` → `"%s "`
- `char` → `"%c "`
- `bool` → `"true"` / `"false"`
- Range unpacking: iterates range and prints each element

### Switch / Case / Default

1. Evaluate switch condition
2. For each case: build `LLVMBuildSwitch` with case values
3. Each case generates its body, then branches to merge
4. Default case is the otherwise branch of the switch instruction
5. Uses LLVM's `switch` instruction for integer types

### Defer

Uses a fixed-size stack of `DeferredEntry`:

1. **Push**: Save current basic block + live variable pointers, store the defer AST
2. **Execute**: Before return or scope exit, iterate stack in reverse, generate bodies
3. **Clear / Inline**: Pop entries after execution

Deferred statements are generated with their own LLVM basic blocks that execute before function exit (or scope exit for block-scoped defers).

### Block

Creates a new LLVM basic block. Generates each statement sequentially. If the block contains deferred frees, processes them at block exit. The containing function's main block continues after the block.

## 6. Module System

### Multi-Pass Module Codegen

1. **Create module units**: For each AST module, create an `LLVMModuleRef` + `ModuleCompilationUnit`
2. **Process @use**: Resolve imports between module units (populate dependency graph)
3. **Build struct types**: First pass to create LLVM struct types (forward declarations)
4. **Codegen in dependency order**: Topological sort, generate each module:
   - Forward declarations (function prototypes)
   - All statements
   - Export symbols for importing modules
5. **Write object files**: `LLVMTargetMachineEmitToFile` for each module

### Symbol Resolution Across Modules

- `exported_symbols` / `imported_symbols` on each `ModuleCompilationUnit`
- `lookup_codegen_symbol_in_module()` walks imported modules
- Symbols referenced with `module_alias` prefix in generated IR

### @os Conditional Compilation

At codegen time, selects the matching OS arm based on `BuildConfig.target_os` and only generates code for that arm.

## 7. Struct System

### Struct Type Creation

1. `find_or_add_struct_type()` in struct cache by name
2. `LLVMStructCreateNamed()` for the LLVM type
3. Set body after resolving all member types (handling forward references via deferred struct body setting)
4. Register field names → index mapping

### Member Access

- `codegen_struct_member_access()`: GEP to field by name (looks up field index)
- `codegen_chained_struct_member_access()`: handles `a.b.c.d` via repeated GEP
- Supports `arr[i].field`: first index, then GEP

### Struct Methods

Methods registered with qualified name `StructName.methodName`. At call site:
1. `self` is automatically prepended (with `&` if value, pass-through if pointer)
2. Method is looked up via qualified name in scope

### Struct Literals

1. Create alloca for result
2. For each named field: GEP + store (with type conversion if needed)
3. Load the full struct value

### Struct Return Convention

If function returns a struct by value:
1. Add `sret` pointer parameter (hidden first param)
2. Caller allocates space, passes pointer
3. Callee stores to pointer, returns void

## 8. Type Code Generation

### Basic Type Mapping

| Type | LLVM Type | Notes |
|---|---|---|
| `int` | `i64` | Signed 64-bit |
| `uint` | `i64` | (treated same as int) |
| `float` | `float` | 32-bit IEEE 754 |
| `double` | `double` | 64-bit IEEE 754 |
| `bool` | `i1` | 1-bit integer |
| `char` | `i8` | 8-bit integer |
| `void` | `void` | No value |
| `str` | `i8*` | Pointer to char |

### Pointer Types

`T*` → pointer to the LLVM type of `T`.

### Array Types

`T[N]` → `[N x T]` (LLVM array type).
Size `N` must be a compile-time constant.

### Function Types

`fn(T1, T2) R` → `R (T1, T2)*` (LLVM function pointer type).

### Type Cache

Common types are cached in `CommonTypes` struct. `get_int_type(size)` returns appropriate integer type. `types_are_equal()` compares LLVM types structurally.

## 9. Utility Functions

| Function | Purpose |
|---|---|
| `alloca_and_store(ctx, name, value)` | Alloca + store idiom |
| `struct_gep_load(ctx, ptr, index, name)` | GEP into struct + load |
| `struct_gep_store(ctx, ptr, index, value)` | GEP into struct + store |
| `array_gep(ctx, ptr, index, name)` | GEP into array + load |
| `block_has_terminator(block)` | Check if block already branches |
| `branch_if_no_terminator(builder, block)` | Conditional branch |
| `build_global_string(ctx, str, name)` | Create global string constant with caching |
| `convert_value_to_type(ctx, value, from_type, to_type)` | Insert implicit conversion casts |
| `get_default_value(type)` | Zero-initializer for LLVM type |
| `needs_conversion(from, to)` | Check if implicit conversion needed |

## 10. Implementation Checklist for Self-Hosted Port

### Phase 1: LLVM Bindings
- [ ] LLVM C API function declarations (Core, Analysis, TargetMachine)
- [ ] `LLVMContextRef`, `LLVMModuleRef`, `LLVMBuilderRef` wrappers
- [ ] Type creation: `LLVMInt32Type`, `LLVMPointerType`, `LLVMStructType`, etc.
- [ ] Instruction building: `LLVMBuildAdd`, `LLVMBuildLoad`, `LLVMBuildStore`, etc.

### Phase 2: Core Infrastructure
- [ ] `CodeGenContext` struct with all state
- [ ] `LLVM_Symbol` table (per-scope variable lookup)
- [ ] `CommonTypes` initialization and caching
- [ ] `StructInfo` cache (name → LLVM type + field mapping)
- [ ] `DeferredInfo` stack for defer statements
- [ ] `ModuleCompilationUnit` for multi-module codegen

### Phase 3: Dispatch
- [ ] `codegen_expr()` — expression dispatcher (21+ cases)
- [ ] `codegen_stmt()` — statement dispatcher (14+ cases)
- [ ] `codegen_type()` — type-to-LLVM-type converter

### Phase 4: Expressions
- [ ] Literals: int, float, bool, char, string, null
- [ ] Identifiers (stack, global, function)
- [ ] Binary ops: arithmetic, comparison, logical, bitwise, range
- [ ] Unary ops: neg, not, bitnot, inc, dec (pre/post)
- [ ] Function calls (direct, method, module-qualified)
- [ ] Assignments (variable, deref, index, member)
- [ ] Index expressions (array, pointer, multi-dim)
- [ ] Member expressions (runtime `.`, compiletime `::`)
- [ ] Dereference and address-of
- [ ] Cast (all numeric/pointer conversions)
- [ ] Alloc / Free (malloc/free calls)
- [ ] Input / System / Syscall (libc wrappers)
- [ ] Sizeof (target data layout)
- [ ] Array literals, struct literals, ternary

### Phase 5: Statements
- [ ] Variable declarations (local alloca + global)
- [ ] Function declarations (params, body, attributes)
- [ ] Struct declarations (LLVM named struct + methods)
- [ ] Enum declarations (global constants)
- [ ] If/elif/else (condbr + phi)
- [ ] Loops (infinite, while, for)
- [ ] Switch/case/default (LLVM switch instruction)
- [ ] Return (with defer unwind)
- [ ] Print (printf format strings)
- [ ] Break/continue
- [ ] Defer (stack + unwind before return)
- [ ] Blocks

### Phase 6: Module System
- [ ] Multi-module compilation units
- [ ] Dependency-ordered code generation
- [ ] Cross-module symbol resolution
- [ ] Per-module object file emission

### Phase 7: Struct System
- [ ] Struct type creation with field index cache
- [ ] Member access via GEP
- [ ] Struct literal codegen
- [ ] Struct return convention (sret)
- [ ] Method generation with implicit self

### Phase 8: Attributes & ABI
- [ ] `#dll_import` (DLL storage class + calling convention)
- [ ] `#lib_import` (external linkage)
- [ ] `#returns_ownership` (noalias on returned pointer)
- [ ] Target triple + data layout configuration
