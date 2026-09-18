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


## Functions

### `create_doc_config`

Builds a `DocGenConfig` with sensible defaults.

When `output_dir` is NULL it falls back to `"docs"`.


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
from the module's `//!` doc comment. Progress is reported through the
compiler's `output` builtin.


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

