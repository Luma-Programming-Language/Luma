# Module: auto_docs

*Source: `src/docs-gen/docs.lx`*

Automatic API documentation generator.

Walks a parsed program AST and emits Markdown: one `<output_dir>/<module>.md`
per module plus an `<output_dir>/README.md` index. Doc comments on decls are
reused verbatim, with `# Parameters` / `# Returns` / `# Example` sections
extracted into their own blocks.

This is a direct port of the C `doc_generator.c` (its `FILE *` sinks are
replaced by `CG::Buf`, flushed with `IO::write_binary`).

## Table of Contents

- [Structures](#structures)
- [Functions](#functions)

---

## Structures

### `DocGenConfig`

| Field | Type | Description |
|-------|------|-------------|
| `output_dir` | *byte |  |
| `format` | *byte |  |
| `include_private` | bool |  |
| `include_source_links` | bool |  |
| `arena` | *void |  |


## Functions

### `create_doc_config`

```luma
pub create_doc_config -> fn(
    arena: *void,
    output_dir: *byte
) DocGenConfig
```

### `generate_module_docs`

```luma
pub generate_module_docs -> fn(
    module: *AST::AstNode,
    config: DocGenConfig,
    f: *CG::Buf
) bool
```

### `generate_documentation`

```luma
pub generate_documentation -> fn(
    program: *AST::AstNode,
    config: DocGenConfig
) bool
```

