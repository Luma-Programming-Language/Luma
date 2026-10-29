;;; luma.el --- Luma language support -*- lexical-binding: t; -*-

(require 'eglot)

(define-derived-mode luma-mode prog-mode "Luma"
  "Major mode for editing Luma source files.")

(add-to-list 'auto-mode-alist '("\\.lx\\'" . luma-mode))

(with-eval-after-load 'eglot
  (add-to-list 'eglot-server-programs
               '(luma-mode . ("luma" "--lsp"))))

(add-hook 'luma-mode-hook #'eglot-ensure)

(provide 'luma)
;;; luma.el ends here
