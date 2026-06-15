return {
  -- Filetype mapping for .lx to luma
  {
    "AstroNvim/astrocore",
    opts = {
      filetypes = {
        extension = {
          lx = "luma",
        },
      },
    },
  },

  -- LSP setup for Luma
  {
    "neovim/nvim-lspconfig",
    opts = function(_, opts)
      local luma_path = vim.fn.expand("luma")
      if vim.fn.executable(luma_path) == 0 then
        vim.notify("Luma LSP not found at " .. luma_path, vim.log.levels.WARN)
        return opts
      end

      local lspconfig = require("lspconfig")
      local configs = require("lspconfig.configs")

      if not configs.luma then
        configs.luma = {
          default_config = {
            cmd = { luma_path, "-lsp" },
            filetypes = { "luma" },
            root_dir = function(fname)
              return lspconfig.util.find_git_ancestor(fname)
                or lspconfig.util.path.dirname(fname)
            end,
            single_file_support = true,
            on_attach = function(client, bufnr)
              vim.notify("Luma LSP attached to buffer " .. bufnr, vim.log.levels.INFO)

              -- Start semantic tokens if supported
              if client.server_capabilities.semanticTokensProvider then
                vim.lsp.semantic_tokens.start(bufnr, client.id)
              end

              -- Keybindings for LSP features
              local bufopts = { noremap = true, silent = true, buffer = bufnr }
              vim.keymap.set("n", "gd", vim.lsp.buf.definition, bufopts)
              vim.keymap.set("n", "K", vim.lsp.buf.hover, bufopts)
              vim.keymap.set("n", "gi", vim.lsp.buf.implementation, bufopts)
              vim.keymap.set("n", "gr", vim.lsp.buf.references, bufopts)
              vim.keymap.set("n", "gD", vim.lsp.buf.declaration, bufopts)
              vim.keymap.set("n", "<C-k>", vim.lsp.buf.signature_help, bufopts)
              vim.keymap.set("i", "<C-k>", vim.lsp.buf.signature_help, bufopts)
              vim.keymap.set("n", "<leader>rn", vim.lsp.buf.rename, bufopts)
              vim.keymap.set("n", "<leader>ca", vim.lsp.buf.code_action, bufopts)
              vim.keymap.set("n", "<leader>f", function()
                vim.lsp.buf.format({ async = true })
              end, bufopts)
              vim.keymap.set("n", "gl", vim.diagnostic.open_float, bufopts)
              vim.keymap.set("n", "[d", vim.diagnostic.goto_prev, bufopts)
              vim.keymap.set("n", "]d", vim.diagnostic.goto_next, bufopts)

              -- Highlight references under cursor
              if client.server_capabilities.documentHighlightProvider then
                vim.api.nvim_create_augroup("luma_lsp_document_highlight", { clear = true })
                vim.api.nvim_create_autocmd({ "CursorHold", "CursorHoldI" }, {
                  group = "luma_lsp_document_highlight",
                  buffer = bufnr,
                  callback = vim.lsp.buf.document_highlight,
                })
                vim.api.nvim_create_autocmd("CursorMoved", {
                  group = "luma_lsp_document_highlight",
                  buffer = bufnr,
                  callback = vim.lsp.buf.clear_references,
                })
              end
            end,
            on_exit = function(code, signal, _)
              if code ~= 0 then
                vim.notify(
                  string.format("Luma LSP exited with code %d (signal %d)", code, signal),
                  vim.log.levels.ERROR
                )
              end
            end,
          },
        }
      end

      local capabilities = vim.lsp.protocol.make_client_capabilities()
      local has_cmp, cmp_nvim_lsp = pcall(require, "cmp_nvim_lsp")
      if has_cmp then
        capabilities = cmp_nvim_lsp.default_capabilities(capabilities)
      end

      -- Enhanced semantic tokens support
      capabilities.textDocument.semanticTokens = {
        dynamicRegistration = true,
        tokenTypes = {
          "namespace", "type", "typeParameter", "function", "method",
          "property", "variable", "parameter", "keyword", "modifier",
          "comment", "string", "number", "operator", "struct", "enum",
          "enumMember",
        },
        tokenModifiers = {
          "declaration", "definition", "readonly", "static", "defaultLibrary",
        },
        formats = { "relative" },
        requests = {
          full = true,
          range = false,
        },
        multilineTokenSupport = false,
        overlappingTokenSupport = false,
      }

      -- Signature help capability
      capabilities.textDocument.signatureHelp = {
        dynamicRegistration = true,
        signatureInformation = {
          documentationFormat = { "markdown", "plaintext" },
          parameterInformation = {
            labelOffsetSupport = true,
          },
        },
      }

      -- Code action capability
      capabilities.textDocument.codeAction = {
        dynamicRegistration = true,
        codeActionLiteralSupport = {
          codeActionKind = {
            valueSet = {
              "quickfix",
              "refactor",
              "source",
            },
          },
        },
      }

      -- Rename capability
      capabilities.textDocument.rename = {
        dynamicRegistration = true,
        prepareSupport = true,
      }

      -- Document highlight capability
      capabilities.textDocument.documentHighlight = {
        dynamicRegistration = true,
      }

      -- Formatting capability
      capabilities.textDocument.formatting = {
        dynamicRegistration = true,
      }

      lspconfig.luma.setup({
        capabilities = capabilities,
      })

      -- Diagnostic configuration for Luma
      vim.diagnostic.config({
        virtual_text = { prefix = "●" },
        signs = true,
        underline = true,
        update_in_insert = false,
        severity_sort = true,
        float = {
          source = "always",
          border = "rounded",
          header = "Luma Diagnostic",
        },
      })

      return opts
    end,
  },
}
