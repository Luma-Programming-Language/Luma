# Zed

This is a Zed language extension for `.lx` files. It starts `luma --lsp` and
provides the language server's diagnostics, completion, hover, navigation,
and syntax highlighting. The extension has no Tree-sitter grammar; Luma
highlighting comes entirely from the language server's semantic tokens.

## Install for development

1. Make sure `luma` is available on `PATH`.
2. In Zed, run **zed: install dev extension** from the Command Palette and
   select this `editors/zed` directory.
3. Restart Zed and open a `.lx` file.

Luma's syntax highlighting comes from LSP semantic tokens. Enable them in
Zed's `settings.json`:

```json
{
  "languages": {
    "Luma": {
      "semantic_tokens": "full"
    }
  }
}
```

With `"full"`, Zed uses the tokens from `luma --lsp` exclusively for
highlighting.

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
