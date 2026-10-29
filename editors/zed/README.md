# Zed

This is a Zed language extension for `.lx` files. It starts `luma --lsp` and
provides the language server's diagnostics, completion, hover, and navigation.
The extension uses the pinned
[tree-sitter-luma grammar](https://github.com/APConduct/tree-sitter-luma) for
syntax parsing and basic highlighting.

## Install for development

1. Make sure `luma` is available on `PATH`.
2. In Zed, run **zed: install dev extension** from the Command Palette and
   select this `editors/zed` directory.
3. Restart Zed and open a `.lx` file.

To select a specific Luma executable, add this to Zed's `settings.json`:

```json
{
  "lsp": {
    "luma": {
      "binary": {
        "path": "C:\\Program Files\\luma\\bin\\luma.exe",
        "arguments": ["--lsp"]
      }
    }
  }
}
```
