vim.filetype.add({
  extension = {
    lx = "luma",
  },
})

vim.lsp.config("luma", {
  cmd = { "luma", "--lsp" },
  filetypes = { "luma" },
  root_markers = { ".git" },
})

vim.lsp.enable("luma")
