# Module: codegen

*Source: `src/codegen/codegen.lx`*

Top-level codegen driver for the C backend.

Pass A builds the symbol registries from the AST; Pass B emits the C
translation unit (enums, structs, prototypes, globals, function bodies).
The driver then transpiles the result to a .c file and compiles it with an
external C compiler.

## Table of Contents

- [Functions](#functions)
- [Variables](#variables)


## Functions

### `is_known_libc_name`

Returns true if `name` is one of the libc/libm names in known_libc_names.

```luma
      is_known_libc_name -> fn(
    name: *byte
) bool
```

### `IOWRITE`

Writes string `s` to the output FILE* `out` (an fputs wrapper).

```luma
      IOWRITE -> fn(
    out: *void,
    s: *byte
) void
```

### `write_input_helpers`

Emits the static C helper functions backing `input<T>(...)`: one raw
`__luma_read_byte` primitive plus a type-specific wrapper per supported T,
all reading through that un-buffered primitive.

```luma
      write_input_helpers -> fn(
    out: *void,
    is_windows: bool
) void
```

### `write_preamble`

Emits the generated file's C preamble: `_GNU_SOURCE`, the standard
headers (platform-conditional), the `__luma_range_t` typedef, and the
input helpers.

```luma
      write_preamble -> fn(
    out: *void,
    os: *byte
) void
```

### `register_method`

Registers a struct's method member (a field holding a function value) into
the function registry, skipping generic templates and embedded members.

```luma
      register_method -> fn(
    ctx: *CG::CodegenContext,
    module_name: *byte,
    struct_name: *byte,
    member: *AST::AstNode
) void
```

### `register_generated_method`

Registers a concrete generic-method instantiation (a standalone
FuncDeclNode with a mangled name) as a method, carrying its owner struct
name and static flag.

```luma
      register_generated_method -> fn(
    ctx: *CG::CodegenContext,
    module_name: *byte,
    owner: *byte,
    is_static: i64,
    decl: *AST::AstNode
) void
```

### `register_decl`

Pass A workhorse: registers one module-level declaration (struct, enum,
function, global, `@os` arm, or `@use` alias) into the appropriate
registry, skipping generic templates.

```luma
      register_decl -> fn(
    ctx: *CG::CodegenContext,
    os: *byte,
    module_name: *byte,
    decl: *AST::AstNode
) void
```

### `run_pass_a`

Pass A driver: walks every module in the program (plus the typechecker's
generic instantiations) and registers all top-level declarations into the
context's registries.

```luma
      run_pass_a -> fn(
    ctx: *CG::CodegenContext,
    prog: *AST::ProgramNode,
    os: *byte
) void
```

### `emit_enums`

Emits a `typedef enum {...} Name;` for every registered enum, using mangled
member names.

```luma
      emit_enums -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf
) void
```

### `emit_struct_typedefs`

Emits a forward `typedef struct Name Name;` for every registered struct.

```luma
      emit_struct_typedefs -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf
) void
```

### `emit_struct_body`

Emits one struct's `struct Name {...};` body, first recursively emitting
any structs it embeds by value (cycle-guarded). Skips already-defined
structs.

```luma
      emit_struct_body -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    s: *CG::StructInfo
) void
```

### `emit_all_struct_bodies`

Emits the C body for every registered struct.

```luma
      emit_all_struct_bodies -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf
) void
```

### `emit_func_prototype`

Emits one function's C prototype: main is the fixed
`int main(int, char**)`, methods gain a leading `Struct *self` parameter,
and names are mangled per the FuncInfo. Known-libc and body-less extern
declarations produce no prototype.

```luma
      emit_func_prototype -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    f: *CG::FuncInfo
) void
```

### `emit_all_prototypes`

Emits a C prototype for every registered function that needs one.

```luma
      emit_all_prototypes -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf
) void
```

### `emit_globals`

Emits one mangled C global definition per registered global, with its
initializer if any.

```luma
      emit_globals -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf
) void
```

### `emit_func_body`

Emits one function's C definition: signature with `self`/param bindings,
then the body statements. Skips externs and body-less declarations; main
binds its fixed argc/argv under `__luma_argc`/`__luma_argv`.

```luma
      emit_func_body -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf,
    f: *CG::FuncInfo
) void
```

### `emit_all_func_bodies`

Emits the C body for every non-extern registered function, main last
(cosmetic; prototypes make ordering unnecessary).

```luma
      emit_all_func_bodies -> fn(
    ctx: *CG::CodegenContext,
    buf: *CG::Buf
) void
```

### `basename_of`

Returns a pointer to the final `/`-separated segment of `path` (the
basename), or `path` itself if it has no slash.

```luma
      basename_of -> fn(
    path: *byte
) *byte
```

### `target_cc_prefix`

Prefix for the compile command: host `cc -w` by default, or a mingw-w64
cross compiler when a Windows target was explicitly requested.

```luma
      target_cc_prefix -> fn(
    os: *byte,
    target_explicit: bool
) *byte
```

### `target_output_name`

Returns the output executable name: `.exe` appended on Windows targets
unless already present. Caller owns the result.

```luma
      #returns_ownership
target_output_name -> fn(
    name: *byte,
    os: *byte
) *byte
```

### `codegen_transpile`

Transpiles `program` to C, writes it to output/<name>.c (creating
output/ if needed), and returns the .c path — caller owns it. Returns
null on a null/non-program input or if the file can't be opened.

```luma
pub #returns_ownership
codegen_transpile -> fn(
    program: *AST::AstNode,
    config: CONST::LumaBuildConfig
) *byte
```

### `codegen_compile`

Compiles/links the C at `c_path` per `config` via an external C compiler,
freeing `c_path` either way (kept on disk when config.save). Returns true
on success.

```luma
pub #takes_ownership
codegen_compile -> fn(
    c_path: *byte,
    config: CONST::LumaBuildConfig
) bool
```


## Variables

- **`KNOWN_LIBC_NAME_COUNT`** : i64 *(const)* — Number of names in known_libc_names; must match the array size below by
- **`known_libc_names`** : [*byte; 67] *(const)* — libc/libm symbol names already declared by the preamble's includes;
