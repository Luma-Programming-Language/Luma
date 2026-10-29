# Luma for Visual Studio Code

This extension starts `luma --lsp` over stdio for `.lx` files. It provides
language-server diagnostics, hover, completion, go to definition, document
symbols, and semantic highlighting.

## Requirements

- Install Luma and make `luma` available on `PATH`, or set `luma.serverPath`
  in VS Code settings to the executable's full path. The extension also checks
  each open workspace for `bin/luma` (or `bin/luma.exe` on Windows), then falls
  back to `luma` on `PATH`.
- Node.js and npm to package this extension from source.

## Install from source

From the repository root:

```sh
cd editors/vscode
npm install
npx --yes @vscode/vsce package --out luma-vscode-0.1.0.vsix
code --install-extension luma-vscode-0.1.0.vsix
```

Restart VS Code and open a `.lx` file. The extension launches the language
server directly without a shell, so paths containing spaces work. To use a
specific executable, add this to your VS Code settings:

```json
{
  "luma.serverPath": "C:\\path\\to\\luma.exe"
}
```

Use the Command Palette for **Luma: Show Server Path**, **Luma: Restart
Language Server**, and **Luma: Show Language Server Output**. The output
channel reports startup, protocol, and restart errors; the trace channel
is available for LSP protocol tracing.
