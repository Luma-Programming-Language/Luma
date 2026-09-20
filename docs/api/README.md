# API Documentation

Generated documentation for the project.

> Auto-generated from doc comments by `luma -docs`. Do not edit by hand.

## Modules

- [main](main.md) — Entry point of the Luma compiler driver.
- [ast_expr](ast_expr.md) — Expression AST node constructors.
- [ast_module](ast_module.md) — Constructors for the preprocessor AST nodes: `@module`, `@use`, `@os`, and
- [ast_type](ast_type.md) — Constructors for the type AST nodes: basic, pointer, array, function, and
- [ast_stmt](ast_stmt.md) — Constructors for the statement AST nodes: the program root, declarations
- [ast](ast.md) — Core AST definitions: the `AstNode` base struct, the `NodeType`,
- [ast_print](ast_print.md) — Pretty-printer for the AST: renders any `*AST::AstNode` as an indented,
- [constants](constants.md) — Compiler-wide constants and shared configuration types.
- [error](error.md) — Compiler error reporting.
- [lexer_tokens](lexer_tokens.md) — Token types, token and error structures, and the keyword and operator
- [lexer](lexer.md) — Scans raw source text into a stream of tokens: whitespace and comments
- [parser_file](parser_file.md) — Entry point for parsing a whole source file: reads and lexes the file,
- [parser_type](parser_type.md) — Parses type syntax - primitive, pointer, array, resolution and function
- [parser](parser.md) — Core parser state and helpers: the Parser struct over the token stream,
- [parser_expr](parser_expr.md) — Parses Luma expressions into AST nodes (a Pratt / precedence-climbing parser).
- [parse_stmt](parse_stmt.md) — Parses Luma statements into AST nodes.
- [tc_core](tc_core.md) — Core shared data types for the type checker.
- [tc_type](tc_type.md) — Type-related helpers for the type checker.
- [tc_scope](tc_scope.md) — Scope management for the type checker.
- [tc_generics](tc_generics.md) — Monomorphization for `fn<T>`/`struct<T>`/`enum<T>` (see docs/docs.md's
- [tc_expr](tc_expr.md) — Typechecking of Luma expressions.
- [tc_error](tc_error.md) — Error reporting for the Luma typechecker.
- [tc](tc.md) — Driver for the Luma typechecker.
- [tc_stmt](tc_stmt.md) — Statement typechecking and declaration registration.
- [cg_core](cg_core.md) — Core code-generation support shared by the C backend.
- [cg_type](cg_type.md) — C type emission for the codegen backend.
- [cg_expr](cg_expr.md) — Expression codegen for the C backend.
- [cg_stmt](cg_stmt.md) — Statement codegen for the C backend.
- [codegen](codegen.md) — Top-level codegen driver for the C backend.
- [auto_docs](auto_docs.md) — Automatic API documentation generator.
- [commands](commands.md) — Command-line argument parsing and driver text for the `luma` tool.
- [lsp_transport](lsp_transport.md) — JSON-RPC-over-stdio transport for the Luma language server.
- [lsp_document](lsp_document.md) — Document tracking + single-file analysis for the Luma language server.
- [lsp_dispatch](lsp_dispatch.md) — JSON-RPC method dispatch for the Luma language server.
- [lsp_main](lsp_main.md) — Entry point for the self-hosted Luma language server.
