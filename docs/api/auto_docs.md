# Module: auto_docs

*Source: `src/docs-gen/docs.lx`*

Automatic API documentation generator.

Walks a parsed program AST and emits Markdown reference pages: one
`<output_dir>/<module>.md` per module plus an `<output_dir>/README.md`
index. Doc comments on declarations are reused verbatim, with
`# Parameters` / `# Returns` / `# Example` sections extracted into their own
bolded blocks, struct fields rendered into a table, and a table of contents
that only lists sections that actually exist on the page.

Invoked from the CLI with `luma -docs [dir]`, or programmatically through
the entry points in this module.

# Example
```luma
let config: DOCS::DocGenConfig = DOCS::create_doc_config(0, "docs/api");
DOCS::generate_documentation(program, config);
```

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)
- [Variables](#variables)

---

## Structures

### `DocGenConfig`

Controls how documentation is generated.

Created with `create_doc_config`; every field is plain data. Only
`output_dir` is read today — the rest are reserved for future options.


| Field | Type | Description |
|-------|------|-------------|
| `output_dir` | *byte |  |
| `format` | *byte |  |
| `include_private` | bool |  |
| `include_source_links` | bool |  |
| `arena` | *void |  |
| `project_root` | *byte |  |


## Functions

### `create_doc_config`

Builds a `DocGenConfig` with sensible defaults.

When `output_dir` is NULL it falls back to `"docs"`. Private declarations
are included by default so the reference is complete; set
`include_private` to false to restrict the pages to the public API.


```luma
pub create_doc_config -> fn(
    arena: *void,
    output_dir: *byte
) DocGenConfig
```

**Parameters:**
- `arena`: Scratch arena to associate with the config (pass 0 if unused)
- `output_dir`: Destination directory for the generated Markdown; NULL means "docs"


**Returns:**
A `DocGenConfig` value ready to pass to `generate_documentation`.


**Example:**
```luma
let config: DOCS::DocGenConfig = DOCS::create_doc_config(0, "docs/api");
```

### `generate_os_docs`

```luma
      generate_os_docs -> fn(
    f: *CG::Buf,
    os_node: *AST::AstNode,
    config: DocGenConfig
) void
```

### `generate_link_docs`

```luma
      generate_link_docs -> fn(
    f: *CG::Buf,
    link_node: *AST::AstNode
) void
```

### `ensure_directory`

```luma
      ensure_directory -> fn(
    path: *byte
) bool
```

### `write_doc_file`

```luma
      write_doc_file -> fn(
    path: *byte,
    content: *byte
) bool
```

### `first_doc_marker`

```luma
      first_doc_marker -> fn(
    doc: *byte
) *byte
```

### `clean_doc`

```luma
      #returns_ownership
clean_doc -> fn(
    doc: *byte
) *byte
```

### `write_doc_comment`

```luma
      write_doc_comment -> fn(
    f: *CG::Buf,
    doc: *byte,
    indent_level: i64
) void
```

### `write_doc_comment_until_marker`

```luma
      write_doc_comment_until_marker -> fn(
    f: *CG::Buf,
    doc: *byte,
    stop: *byte
) void
```

### `print_doc_sections`

```luma
      print_doc_sections -> fn(
    f: *CG::Buf,
    doc: *byte
) void
```

### `print_type`

```luma
      print_type -> fn(
    f: *CG::Buf,
    type: *AST::AstNode
) void
```

### `write_func_attributes`

```luma
      write_func_attributes -> fn(
    f: *CG::Buf,
    fd: *AST::FuncDeclNode
) void
```

### `write_type_params`

```luma
      write_type_params -> fn(
    f: *CG::Buf,
    params: **byte,
    count: i64
) void
```

### `write_func_signature`

```luma
      write_func_signature -> fn(
    f: *CG::Buf,
    fd: *AST::FuncDeclNode,
    header: *byte
) void
```

### `generate_function_docs`

```luma
      generate_function_docs -> fn(
    f: *CG::Buf,
    func: *AST::AstNode,
    config: DocGenConfig
) void
```

### `write_inline_doc`

```luma
      write_inline_doc -> fn(
    f: *CG::Buf,
    doc: *byte
) void
```

### `generate_struct_docs`

```luma
      generate_struct_docs -> fn(
    f: *CG::Buf,
    strct: *AST::AstNode,
    config: DocGenConfig
) void
```

### `generate_enum_docs`

```luma
      generate_enum_docs -> fn(
    f: *CG::Buf,
    enm: *AST::AstNode,
    config: DocGenConfig
) void
```

### `generate_var_docs`

```luma
      generate_var_docs -> fn(
    f: *CG::Buf,
    var: *AST::AstNode,
    config: DocGenConfig
) void
```

### `generate_block_decls`

```luma
      generate_block_decls -> fn(
    f: *CG::Buf,
    block: *AST::AstNode,
    config: DocGenConfig
) void
```

### `generate_os_docs`

```luma
      generate_os_docs -> fn(
    f: *CG::Buf,
    os_node: *AST::AstNode,
    config: DocGenConfig
) void
```

### `generate_link_docs`

```luma
      generate_link_docs -> fn(
    f: *CG::Buf,
    link_node: *AST::AstNode
) void
```

### `generate_module_docs`

Renders a single module as a Markdown page.

Emits the module heading, its `//!` doc comment, the source path, a table
of contents limited to sections that are actually present, followed by the
module's structs, enums, functions, variables, `@os` blocks, and `@link`
libraries.


```luma
pub generate_module_docs -> fn(
    module: *AST::AstNode,
    config: DocGenConfig,
    f: *CG::Buf
) bool
```

**Parameters:**
- `module`: A `PREPROCESSOR_MODULE` AST node
- `config`: Generation options
- `f`: Buffer that receives the Markdown output


**Returns:**
True when `module` was a module node and was rendered; false otherwise
(a NULL node or the wrong node kind). Writes nothing on failure.


**Example:**
```luma
let out: *CG::Buf = CG::buf_init();
DOCS::generate_module_docs(module, config, out);
```

### `generate_documentation`

Generates the full API reference for a parsed program.

Creates `output_dir` if needed, then writes one `<module>.md` per module
plus a `README.md` index linking to each page with a one-line summary taken
from the module's `//!` doc comment. Only modules under the build's
project root are emitted (default `src/`, overridable through
`config.project_root`) — linked std/lib dependencies (e.g. transitive
`@use`s) are skipped so the reference mirrors the project's own source
tree. Progress is reported through the compiler's `output` builtin.


```luma
pub generate_documentation -> fn(
    program: *AST::AstNode,
    config: DocGenConfig
) bool
```

**Parameters:**
- `program`: A `PROGRAM` AST node, as produced by the parser
- `config`: Generation options; `output_dir` selects the destination


**Returns:**
True when every module was written successfully; false if a module failed
to render or any file could not be created.


**Example:**
```luma
DOCS::generate_documentation(program, DOCS::create_doc_config(0, "docs/api"));
```


## Variables

- **`DOC_MARKERS`** : [*byte; 3] *(const)*
