project('luma', 'c',
  version : 'git',
  license : 'MIT',
  default_options : [
    'c_std=c11',
    'warning_level=2',
    'buildtype=release',
  ]
)

add_languages('cpp', native : false, required : true)

cc = meson.get_compiler('c')

llvm_dep = dependency('llvm',
  modules  : ['core', 'support', 'irreader'],
  required : true,
  version  : get_option('llvm'),
  method   : 'config-tool',
)

sources = files(
  'src/main.c',

  # AST
  'src/ast/ast.c',
  'src/ast/ast_definistions/expr.c',
  'src/ast/ast_definistions/preprocessor.c',
  'src/ast/ast_definistions/stmt.c',
  'src/ast/ast_definistions/type.c',
  'src/ast/ast_utils.c',

  # Auto docs
  'src/auto_docs/doc_generator.c',

  # C libs
  'src/c_libs/color/color.c',
  'src/c_libs/error/error.c',
  'src/c_libs/memory/memory.c',

  # Helper
  'src/helper/help.c',
  'src/helper/run.c',
  'src/helper/std_path.c',

  # Lexer
  'src/lexer/lexer.c',

  # LLVM backend
  'src/llvm/core/llvm.c',
  'src/llvm/core/lookup.c',
  'src/llvm/expr/arrays.c',
  'src/llvm/expr/binary_ops.c',
  'src/llvm/expr/defer.c',
  'src/llvm/expr/expr.c',
  'src/llvm/module/member_access.c',
  'src/llvm/module/module_handles.c',
  'src/llvm/stmt/stmt.c',
  'src/llvm/struct/struct_access.c',
  'src/llvm/struct/struct.c',
  'src/llvm/struct/struct_expr.c',
  'src/llvm/struct/struct_helpers.c',
  'src/llvm/types/type.c',
  'src/llvm/types/type_cache.c',
  'src/llvm/util/helpers.c',

  # LSP server
  'src/lsp/formatter/expr.c',
  'src/lsp/formatter/formatter.c',
  'src/lsp/formatter/stmt.c',
  'src/lsp/lsp_diagnostics.c',
  'src/lsp/lsp_document.c',
  'src/lsp/lsp_features.c',
  'src/lsp/lsp_json.c',
  'src/lsp/lsp_message.c',
  'src/lsp/lsp_module.c',
  'src/lsp/lsp_semantic_tokens.c',
  'src/lsp/lsp_server.c',
  'src/lsp/lsp_symbols.c',

  # Parser
  'src/parser/expr.c',
  'src/parser/parser.c',
  'src/parser/parser_utils.c',
  'src/parser/stmt.c',
  'src/parser/type.c',

  # Typechecker
  'src/typechecker/array.c',
  'src/typechecker/error.c',
  'src/typechecker/expr.c',
  'src/typechecker/lookup.c',
  'src/typechecker/module.c',
  'src/typechecker/scope.c',
  'src/typechecker/static_mem_tracker.c',
  'src/typechecker/stmt.c',
  'src/typechecker/tc.c',
  'src/typechecker/type.c',
)

executable('luma',
  sources,
  dependencies : llvm_dep,
  install      : true,
  install_dir  : get_option('bindir'),
)