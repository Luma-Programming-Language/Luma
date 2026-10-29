# Neovim

This client uses Neovim's built-in LSP support and requires Neovim 0.11 or
newer. Ensure `luma` is available on `PATH`.

Copy `luma.lua` to `~/.config/nvim/lua/luma.lua` (or the equivalent Neovim
configuration directory), then add this to `init.lua`:

```lua
require("luma")
```

Files ending in `.lx` are detected as Luma and start `luma --lsp`. To use a
specific binary, replace `"luma"` in `cmd` with its full path, for example
`"C:/Program Files/luma/bin/luma.exe"` on Windows.
