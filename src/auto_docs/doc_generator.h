/**
 * @file doc_generator.h
 * @brief Documentation generation system for Luma language
 */

#pragma once

#include "../ast/ast.h"
#include "../c_libs/memory/memory.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Configuration for documentation generation
 */
typedef struct {
  const char *output_dir;    // Directory to write documentation files
  const char *format;        // Output format: "markdown", "html", "json"
  bool include_private;      // Include private members in documentation
  bool include_source_links; // Include links to source code
  ArenaAllocator *arena;     // Arena for memory allocation
} DocGenConfig;

/**
 * @brief Initialize documentation generator configuration
 */
DocGenConfig create_doc_config(ArenaAllocator *arena, const char *output_dir);

/**
 * @brief Generate documentation for an entire program
 *
 * @param program The root AST node (program)
 * @param config Documentation generation configuration
 * @return true if generation succeeded, false otherwise
 */
bool generate_documentation(AstNode *program, DocGenConfig config);

/**
 * @brief Generate documentation for a single module
 *
 * @param module The module AST node
 * @param config Documentation generation configuration
 * @param output_file File to write documentation to
 * @return true if generation succeeded, false otherwise
 */
bool generate_module_docs(AstNode *module, DocGenConfig config,
                          FILE *output_file);