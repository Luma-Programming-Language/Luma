# Shared by bootstrap-build.sh and cross-build.sh — one file list, one
# place to keep it in sync with lumix.toml's [run].args.
#
# Deliberately NOT included: std/win32.lx. It's Windows-only (kernel32
# externs with no @os gating of its own — meant to be link-time-conditional,
# not a module that no-ops on other platforms) and nothing in this list
# needs it: the one Windows-specific bit any of these files touches
# (lsp_transport.lx's stdout redirect) declares its own handful of kernel32
# externs locally instead, so it doesn't need std/win32.lx linked at all.
SRC_FILES=(
  src/ast/expr.lx src/ast/module.lx src/ast/type.lx src/ast/stmt.lx
  src/ast/ast.lx src/ast/ast_print.lx
  std/cstring.lx std/vector.lx src/constants.lx std/io.lx std/sys.lx
  src/error/error.lx
  src/lexer/tokens.lx src/lexer/lexer.lx std/memory.lx std/arena.lx
  src/parser/file.lx src/parser/type.lx src/parser/parser.lx
  src/parser/expr.lx src/parser/stmt.lx
  src/typechecker/core.lx src/typechecker/type.lx src/typechecker/scope.lx
  src/typechecker/generics.lx
  src/typechecker/expr.lx src/typechecker/tc_error.lx src/typechecker/tc.lx
  src/typechecker/stmt.lx
  src/codegen/codegen_core.lx src/codegen/codegen_type.lx
  src/codegen/codegen_expr.lx src/codegen/codegen_stmt.lx std/libc.lx
  src/codegen/codegen.lx
  std/args.lx src/commands.lx std/thread.lx lib/progress_bar.lx lib/color.lx lib/json.lx
  src/lsp/lsp_transport.lx src/lsp/lsp_document.lx src/lsp/lsp_dispatch.lx
  src/lsp/lsp_main.lx
)
