/**
 * @file parser.c
 * @brief Implementation of the parser module for the programming language
 * compiler
 *
 * This file contains the core parsing functionality that converts a stream of
 * tokens into an Abstract Syntax Tree (AST). The parser uses a Pratt parser
 * approach for handling operator precedence and associativity in expressions.
 *
 * The parser supports:
 * - Statement parsing (variables, functions, control flow, etc.)
 * - Expression parsing with proper operator precedence
 * - Type parsing for type annotations
 * - Error reporting with source location information
 *
 * @author Connor Harris
 * @date 2025
 * @version 1.0
 */

#include <stdalign.h>
#include <stdio.h>

#include "../ast/ast.h"
#include "../c_libs/error/error.h"
#include "../c_libs/memory/memory.h"
#include "../helper/help.h"
#include "parser.h"

/**
 * @brief Reports a parser error with detailed location information
 *
 * Creates and adds an error to the global error system with information about
 * where the error occurred in the source code, including line and column
 * information.
 *
 * @param psr Pointer to the parser instance
 * @param error_type String describing the type of error (e.g., "SyntaxError")
 * @param file Path to the source file where the error occurred
 * @param msg Detailed error message describing what went wrong
 * @param line Line number where the error occurred (1-based)
 * @param col Column number where the error occurred (1-based)
 * @param tk_length Length of the token that caused the error
 *
 * @note This function uses the arena allocator to duplicate the line text
 * @see ErrorInformation, error_add()
 */
void parser_error(Parser *psr, const char *error_type, const char *file,
                  const char *msg, int line, int col, int tk_length) {
  (void)file;
  ErrorInformation err = {
      .error_type = error_type,
      .file_path = psr->file_path,
      .message = msg,
      .line = line,
      .col = col,
      .line_text = generate_line(psr->arena, psr->tks, psr->tk_count, err.line),
      .token_length = tk_length,
      .label = "Parser Error",
      .note = NULL,
      .help = NULL,
  };
  error_add(err);
}

/**
 * @brief Main parsing function that converts tokens into an AST
 *
 * This is the entry point for the parser. It takes a growable array of tokens
 * and converts them into a complete program AST node containing all parsed
 * statements.
 *
 * @param tks Growable array containing all tokens from the lexer
 * @param arena Arena allocator for memory management during parsing
 *
 * @return Pointer to the root AST node (Program node) containing all parsed
 * statements, or NULL if parsing fails
 *
 * @note The function estimates the initial capacity for statements based on
 * token count
 * @note All memory allocations use the provided arena allocator
 *
 * @see Parser, parse_stmt(), create_program_node()
 */

/**
 * @brief Main parsing function that converts tokens into an AST
 *
 * This is the entry point for the parser. It takes a growable array of tokens
 * and converts them into a complete program AST node containing all parsed
 * statements.
 */

Stmt *parse(GrowableArray *tks, ArenaAllocator *arena, BuildConfig *config) {
  // Initialize parser
  Parser parser = {
      .file_path = config->filepath,
      .arena = arena,
      .tks = (Token *)tks->data,
      .tk_count = tks->count,
      .capacity = (tks->count / 4) + 10,
      .pos = 0,
  };

  if (!parser.tks) {
    parser_error(&parser, "SyntaxError", parser.file_path,
                 "Internal error: failed to get tokens from token array", 0, 0,
                 0);
    return NULL;
  }

  // Initialize arrays
  GrowableArray stmts, modules;
  if (!init_parser_arrays(&parser, &stmts, &modules)) {
    return NULL;
  }

  // Parse module declaration
  char *module_doc = NULL;
  Token module_tok = p_current(&parser);
  const char *module_name = parse_module_declaration(&parser, &module_doc);
  if (!module_name) {
    error_report();
    return NULL;
  }

  // Create module node with doc comment
  // Signature: create_module_node(arena, name, doc_comment, potions, body,
  // body_count, line, column)
  Stmt *module_stmt = create_module_node(parser.arena, module_name,
                                         module_doc, // doc_comment parameter
                                         0,          // potions
                                         NULL,       // body (initially NULL)
                                         0,          // body_count
                                         module_tok.line, module_tok.col);

  Stmt **module_slot = (Stmt **)growable_array_push(&modules);
  if (!module_slot) {
    parser_error(&parser, "SyntaxError", parser.file_path,
                 "Internal error: out of memory growing modules array",
                 p_current(&parser).line, p_current(&parser).col, 0);
    return NULL;
  }
  *module_slot = module_stmt;

  // Parse all statements
  while (p_current(&parser).type_ != TOK_EOF) {
    Stmt *stmt = parse_stmt(&parser);
    if (!stmt) {
      // CRITICAL: Report accumulated errors before returning
      error_report();
      return NULL;
    }

    Stmt **slot = (Stmt **)growable_array_push(&stmts);
    if (!slot) {
      parser_error(&parser, "SyntaxError", parser.file_path,
                   "Internal error: out of memory growing statements array",
                   p_current(&parser).line, p_current(&parser).col, 0);
      error_report();
      return NULL;
    }
    *slot = stmt;
  }

  // Update module with parsed statements
  *module_slot = create_module_node(parser.arena, module_name,
                                    module_doc, // doc_comment parameter
                                    0,          // potions
                                    (Stmt **)stmts.data, // body
                                    stmts.count,         // body_count
                                    module_tok.line, module_tok.col);

  // Create and return program node
  return create_program_node(parser.arena, (AstNode **)modules.data,
                             modules.count, 0, 0);
}

/**
 * @brief Gets the binding power (precedence) for a given token type
 *
 * This function is crucial for the Pratt parser implementation. It returns
 * the binding power (precedence level) for different operators, which
 * determines the order of operations during expression parsing.
 *
 * Higher binding power values indicate higher precedence operators.
 *
 * @param kind The token type to get binding power for
 *
 * @return BindingPower enumeration value representing the precedence level
 *         Returns BP_NONE for tokens that don't have binding power
 *
 * @note Precedence levels (highest to lowest):
 *       - BP_CALL: Function calls, member access, indexing
 *       - BP_POSTFIX: Postfix increment/decrement
 *       - BP_PRODUCT: Multiplication, division
 *       - BP_SUM: Addition, subtraction
 *       - BP_RELATIONAL: Comparison operators
 *       - BP_EQUALITY: Equality and inequality
 *       - BP_BITWISE_AND, BP_BITWISE_XOR, BP_BITWISE_OR: Bitwise operations
 *       - BP_LOGICAL_AND, BP_LOGICAL_OR: Logical operations
 *       - BP_TERNARY: Ternary conditional operator
 *       - BP_ASSIGN: Assignment operators
 *
 * @see BindingPower, LumaTokenType
 */
BindingPower get_bp(LumaTokenType kind) {
  switch (kind) {
  // Assignment
  case TOK_EQUAL:
    return BP_ASSIGN;

  // Ternary
  case TOK_QUESTION:
    return BP_TERNARY;

  // Logical
  case TOK_OR:
    return BP_LOGICAL_OR;
  case TOK_AND:
    return BP_LOGICAL_AND;

  // Bitwise
  case TOK_PIPE:
    return BP_BITWISE_OR;
  case TOK_CARET:
    return BP_BITWISE_XOR;
  case TOK_AMP:
    return BP_BITWISE_AND;

  // Equality
  case TOK_EQEQ:
  case TOK_NEQ:
    return BP_EQUALITY;

  // Relational
  case TOK_LT:
  case TOK_LE:
  case TOK_GT:
  case TOK_GE:
    return BP_RELATIONAL;

  // Shift operators (ADD THIS SECTION)
  case TOK_SHIFT_LEFT:
  case TOK_SHIFT_RIGHT:
    return BP_SHIFT;

  // Arithmetic
  case TOK_PLUS:
  case TOK_MINUS:
    return BP_SUM;
  case TOK_STAR:
  case TOK_SLASH:
  case TOK_MODL:
    return BP_PRODUCT;

  // Postfix
  case TOK_PLUSPLUS:
  case TOK_MINUSMINUS:
    return BP_POSTFIX;

  // Call/indexing/member access
  case TOK_LPAREN:
  case TOK_LBRACKET:
  case TOK_DOT:
  case TOK_RESOLVE:
  case TOK_LBRACE:
    return BP_CALL;

  case TOK_RANGE:
    return BP_RANGE;

  default:
    return BP_NONE;
  }
}

/**
 * @brief Null Denotation - handles prefix expressions and primary expressions
 *
 * This is part of the Pratt parser implementation. The "nud" function handles
 * tokens that can appear at the beginning of an expression (prefix operators
 * and primary expressions like literals and identifiers).
 *
 * @param parser Pointer to the parser instance
 *
 * @return Pointer to the parsed expression AST node, or NULL if parsing fails
 *
 * @note Handles:
 *       - Primary expressions: numbers, strings, identifiers
 *       - Prefix unary operators: -, +, !, ++, --
 *       - Grouped expressions: (expression)
 *       - Array literals: [expression, ...]
 *
 * @see led(), parse_expr(), primary(), unary(), grouping(), array_expr()
 */
Expr *nud(Parser *parser) {
  switch (p_current(parser).type_) {
  case TOK_NUMBER:
  case TOK_NUM_FLOAT:
  case TOK_STRING:
  case TOK_CHAR_LITERAL:
  case TOK_IDENTIFIER:
  case TOK_TRUE:
  case TOK_FALSE:
    return primary(parser);
  case TOK_MINUS:
  case TOK_PLUS:
  case TOK_BANG:
  case TOK_TILDE:
  case TOK_PLUSPLUS:
  case TOK_MINUSMINUS:
    return unary(parser);
  case TOK_LPAREN:
    return grouping(parser);
  case TOK_LBRACKET:
    return array_expr(parser);
  case TOK_LBRACE:
    return struct_expr(parser);
  case TOK_STAR:
    return deref_expr(parser);
  case TOK_AMP:
    return addr_expr(parser);
  case TOK_ALLOC:
    return alloc_expr(parser);
  case TOK_FREE:
    return free_expr(parser);
  case TOK_CAST:
    return cast_expr(parser);
  case TOK_INPUT:
    return input_expr(parser);
  case TOK_SYSTEM:
    return system_expr(parser);
  case TOK_SYSCALL:
    return syscall_expr(parser);

  // Compile time
  case TOK_SIZE_OF:
    return sizeof_expr(parser);
  default:
    p_advance(parser);
    return NULL;
  }
}

/**
 * @brief Left Denotation - handles binary and postfix expressions
 *
 * This is part of the Pratt parser implementation. The "led" function handles
 * tokens that can appear after an expression has been parsed (binary operators
 * and postfix operators).
 *
 * @param parser Pointer to the parser instance
 * @param left The left operand expression (already parsed)
 * @param bp The current binding power context
 *
 * @return Pointer to the parsed expression AST node incorporating the left
 * operand, or the original left expression if no valid LED is found
 *
 * @note Handles:
 *       - Binary arithmetic and logical operators: +, -, *, /, ==, !=, etc.
 *       - Function calls: function(args)
 *       - Assignment: variable = value
 *       - Member access: object.member
 *       - Postfix operators: variable++, variable--
 *       - Array indexing: array[index]
 *
 * @see nud(), parse_expr(), binary(), call_expr(), assign_expr(), prefix_expr()
 */
Expr *led(Parser *parser, Expr *left, BindingPower bp) {
  switch (p_current(parser).type_) {
  case TOK_PLUS:
  case TOK_MINUS:
  case TOK_STAR:
  case TOK_SLASH:
  case TOK_MODL:
  case TOK_EQEQ:
  case TOK_NEQ:
  case TOK_LT:
  case TOK_LE:
  case TOK_GT:
  case TOK_GE:
  case TOK_AMP:
  case TOK_PIPE:
  case TOK_CARET:
  case TOK_AND:
  case TOK_OR:
  case TOK_RANGE:
  case TOK_SHIFT_LEFT:
  case TOK_SHIFT_RIGHT:
    return binary(parser, left, bp);
  case TOK_LPAREN:
    return call_expr(parser, left, bp);
  case TOK_EQUAL:
    return assign_expr(parser, left, bp);
  case TOK_DOT:
  case TOK_RESOLVE:
  case TOK_PLUSPLUS:
  case TOK_MINUSMINUS:
  case TOK_LBRACKET:
    return prefix_expr(parser, left, bp);
  case TOK_LBRACE:
    return named_struct_expr(parser, left, bp);
  default:
    p_advance(parser);
    return left;
  }
}

/**
 * @brief Parses an expression using the Pratt parsing algorithm
 *
 * This is the core expression parsing function that implements the Pratt parser
 * algorithm. It handles operator precedence and associativity automatically
 * through the binding power mechanism.
 *
 * @param parser Pointer to the parser instance
 * @param bp Minimum binding power - only operators with higher binding power
 *           will be consumed by this call
 *
 * @return Pointer to the parsed expression AST node, or NULL if parsing fails
 *
 * @note The algorithm works by:
 *       1. Getting the left expression using nud()
 *       2. While the next operator has higher binding power than bp:
 *          - Use led() to extend the expression with the operator
 *       3. Return the final expression
 *
 * @see nud(), led(), get_bp(), BindingPower
 */
Expr *parse_expr(Parser *parser, BindingPower bp) {
  Expr *left = nud(parser);

  while (p_has_tokens(parser) && get_bp(p_current(parser).type_) > bp) {
    BindingPower current_bp = get_bp(p_current(parser).type_);
    left = led(parser, left, current_bp);
  }

  return left;
}

/**
 * @brief Parses a single statement
 *
 * This function dispatches to the appropriate statement parsing function based
 * on the current token. It also handles visibility modifiers (public/private)
 * that can appear before certain statement types.
 *
 * @param parser Pointer to the parser instance
 *
 * @return Pointer to the parsed statement AST node, or NULL if parsing fails
 *
 * @note Handles:
 *       - Variable declarations: const, var
 *       - Control flow: return, if, loop, break, continue
 *       - Block statements: { ... }
 *       - Print statements: print, println
 *       - Expression statements: any expression followed by semicolon
 *       - Visibility modifiers: public, private (applied to declarations)
 *
 * @see const_stmt(), var_stmt(), return_stmt(), block_stmt(), if_stmt(),
 *      loop_stmt(), print_stmt(), break_continue_stmt(), expr_stmt()
 */
Stmt *parse_stmt(Parser *parser) {
  consume_doc_comments(parser);

  bool returns_ownership = false;
  bool takes_ownership = false;
  bool is_public = false;
  bool is_dll_import = false;
  bool is_lib_import = false;
  const char *lib_name = NULL;
  const char *dll_name = NULL;
  const char *dll_callconv = NULL;

  // Check for attribute modifiers (#returns_ownership, #takes_ownership,
  // #dll_import)
  while (p_current(parser).type_ == TOK_RETURNES_OWNERSHIP ||
         p_current(parser).type_ == TOK_TAKES_OWNERSHIP ||
         p_current(parser).type_ == TOK_DLL_IMPORT ||
         p_current(parser).type_ == TOK_LIB_IMPORT) {

    if (p_current(parser).type_ == TOK_RETURNES_OWNERSHIP) {
      returns_ownership = true;
      p_advance(parser);

    } else if (p_current(parser).type_ == TOK_TAKES_OWNERSHIP) {
      takes_ownership = true;
      p_advance(parser);

    } else if (p_current(parser).type_ == TOK_DLL_IMPORT) {
      is_dll_import = true;
      p_advance(parser); // consume #dll_import

      p_consume(parser, TOK_LPAREN, "Expected '(' after #dll_import");

      if (p_current(parser).type_ != TOK_STRING) {
        parser_error(
            parser, "SyntaxError", parser->file_path,
            "Expected DLL name string as first argument to #dll_import",
            p_current(parser).line, p_current(parser).col,
            p_current(parser).length);
        return NULL;
      }
      dll_name = arena_strdup(parser->arena, get_name(parser));
      p_advance(parser); // consume dll name string

      // Optional: , callconv: "stdcall"
      if (p_current(parser).type_ == TOK_COMMA) {
        p_advance(parser); // consume ','

        if (p_current(parser).type_ != TOK_IDENTIFIER) {
          parser_error(parser, "SyntaxError", parser->file_path,
                       "Expected 'callconv' option in #dll_import",
                       p_current(parser).line, p_current(parser).col,
                       p_current(parser).length);
          return NULL;
        }

        char *kw = get_name(parser);
        p_advance(parser); // consume option name

        if (strcmp(kw, "callconv") != 0) {
          parser_error(parser, "SyntaxError", parser->file_path,
                       "Unknown #dll_import option, expected 'callconv'",
                       p_current(parser).line, p_current(parser).col,
                       p_current(parser).length);
          return NULL;
        }

        p_consume(parser, TOK_COLON, "Expected ':' after 'callconv'");

        if (p_current(parser).type_ != TOK_STRING) {
          parser_error(parser, "SyntaxError", parser->file_path,
                       "Expected calling convention string after 'callconv:'",
                       p_current(parser).line, p_current(parser).col,
                       p_current(parser).length);
          return NULL;
        }
        dll_callconv = arena_strdup(parser->arena, get_name(parser));
        p_advance(parser); // consume callconv string
      }

      p_consume(parser, TOK_RPAREN, "Expected ')' to close #dll_import");
    } else if (p_current(parser).type_ == TOK_LIB_IMPORT) {
      is_lib_import = true;
      p_advance(parser); // consume #lib_import

      p_consume(parser, TOK_LPAREN, "Expected '(' after #lib_import");

      if (p_current(parser).type_ != TOK_STRING) {
        parser_error(parser, "SyntaxError", parser->file_path,
                     "Expected library name string as argument to #lib_import",
                     p_current(parser).line, p_current(parser).col,
                     p_current(parser).length);
        return NULL;
      }
      lib_name = arena_strdup(parser->arena, get_name(parser));
      p_advance(parser);

      p_consume(parser, TOK_RPAREN, "Expected ')' to close #lib_import");
    }
  }

  // Check for visibility modifiers
  if (p_current(parser).type_ == TOK_PUBLIC) {
    is_public = true;
    p_advance(parser);
  } else if (p_current(parser).type_ == TOK_PRIVATE) {
    is_public = false;
    p_advance(parser);
  }

  Stmt *node = NULL;

  switch (p_current(parser).type_) {
  case TOK_USE:
    node = use_stmt(parser);
    break;
  case TOK_OS:
    node = os_stmt(parser);
    break;
  case TOK_LINK:
    node = link_stmt(parser);
    break;
  case TOK_CONST:
    node = const_stmt(parser, is_public, returns_ownership, takes_ownership);
    break;
  case TOK_VAR:
    node = var_stmt(parser, is_public);
    break;
  case TOK_RETURN:
    node = return_stmt(parser);
    break;
  case TOK_LBRACE:
    node = block_stmt(parser);
    break;
  case TOK_IF:
    node = if_stmt(parser);
    break;
  case TOK_LOOP:
    node = loop_stmt(parser);
    break;
  case TOK_PRINT:
    node = print_stmt(parser, false);
    break;
  case TOK_PRINTLN:
    node = print_stmt(parser, true);
    break;
  case TOK_CONTINUE:
  case TOK_BREAK:
    node = break_continue_stmt(parser, p_current(parser).type_ == TOK_CONTINUE);
    break;
  case TOK_DEFER:
    node = defer_stmt(parser);
    break;
  case TOK_SWITCH:
    node = switch_stmt(parser);
    break;
  case TOK_IMPL:
    node = impl_stmt(parser);
    break;
  default:
    node = expr_stmt(parser);
    break;
  }

  // Stamp dll_import metadata onto the resulting function node
  if (node && is_dll_import) {
    if (node->type != AST_STMT_FUNCTION) {
      parser_error(parser, "SyntaxError", parser->file_path,
                   "#dll_import can only be applied to function declarations",
                   node->line, node->column, 0);
      return NULL;
    }
    apply_dll_import(node, dll_name, dll_callconv);
  }

  if (node && is_lib_import) {
    if (node->type != AST_STMT_FUNCTION) {
      parser_error(parser, "SyntaxError", parser->file_path,
                   "#lib_import can only be applied to function declarations",
                   node->line, node->column, 0);
      return NULL;
    }
    apply_lib_import(node, lib_name);
  }

  return node;
}

/**
 * @brief Parses a type annotation
 *
 * This function parses type expressions used in variable declarations,
 * function parameters, return types, etc. It handles primitive types,
 * pointer types, array types, and user-defined types.
 *
 * @param parser Pointer to the parser instance
 *
 * @return Pointer to the parsed Type AST node, or NULL if parsing fails
 *
 * @note Handles:
 *       - Primitive types: int, uint, float, bool, string, void, char
 *       - Pointer types: *type
 *       - Array types: [size]type or []type
 *       - User-defined types: identified by TOK_IDENTIFIER
 *
 * @warning Prints error message to stderr for unexpected tokens
 *
 * @see tnud(), tled(), LumaTokenType
 */
Type *parse_type(Parser *parser) {
  LumaTokenType tok = p_current(parser).type_;

  switch (tok) {
  case TOK_INT:
  case TOK_UINT:
  case TOK_DOUBLE:
  case TOK_FLOAT:
  case TOK_BOOL:
  case TOK_STRINGT:
  case TOK_VOID:
  case TOK_CHAR:
  case TOK_STAR:       // Pointer type
  case TOK_LBRACKET:   // Array type
  case TOK_IDENTIFIER: // Could be simple type or namespace::Type
    return tnud(parser);

  default:
    parser_error(parser, "TypeError", parser->file_path,
                 "Expected a type name here", p_current(parser).line,
                 p_current(parser).col, p_current(parser).length);
    return NULL;
  }
}
