# Emacs

This client uses Eglot, included with Emacs 29 and newer. On older Emacs
versions, install Eglot from GNU ELPA. Ensure `luma` is available on `PATH`.

Load `luma.el` from your Emacs configuration:

```elisp
(load "/path/to/Luma/editors/emacs/luma.el")
```

Opening a `.lx` file selects `luma-mode` and starts `luma --lsp`. To use a
specific executable, change `"luma"` in `eglot-server-programs` to its full
path.
