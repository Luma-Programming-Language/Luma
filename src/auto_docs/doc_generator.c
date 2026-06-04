#include "doc_generator.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

// Helper to create directory
static bool ensure_directory(const char *path) {
  struct stat st = {0};
  if (stat(path, &st) == -1) {
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
      fprintf(stderr, "Failed to create directory: %s\n", path);
      return false;
    }
  }
  return true;
}

// Write formatted doc comment (handles markdown syntax in comments)
static void write_doc_comment(FILE *f, const char *doc, int indent_level) {
  if (!doc || !*doc)
    return;

  const char *line = doc;
  while (*line) {
    // Find end of line
    const char *end = strchr(line, '\n');
    if (!end)
      end = line + strlen(line);

    // Write indentation
    for (int i = 0; i < indent_level; i++) {
      fprintf(f, "  ");
    }

    // Write line content (preserve markdown in doc comments)
    fprintf(f, "%.*s\n", (int)(end - line), line);

    // Move to next line
    if (*end == '\n') {
      line = end + 1;
    } else {
      break;
    }
  }
}

// Helper function to print type information
static void print_type(FILE *f, AstNode *type) {
  if (!type) {
    fprintf(f, "?");
    return;
  }

  switch (type->type) {
  case AST_TYPE_BASIC:
    fprintf(f, "%s", type->type_data.basic.name);
    break;

  case AST_TYPE_POINTER:
    fprintf(f, "*");
    print_type(f, type->type_data.pointer.pointee_type);
    break;

  case AST_TYPE_ARRAY:
    fprintf(f, "[");
    print_type(f, type->type_data.array.element_type);
    fprintf(f, "; ");
    if (type->type_data.array.size) {
      // Try to print array size if it's a literal
      if (type->type_data.array.size->type == AST_EXPR_LITERAL &&
          type->type_data.array.size->expr.literal.lit_type == LITERAL_INT) {
        fprintf(f, "%lld",
                type->type_data.array.size->expr.literal.value.int_val);
      } else {
        fprintf(f, "N");
      }
    }
    fprintf(f, "]");
    break;

  case AST_TYPE_FUNCTION:
    fprintf(f, "fn(");
    for (size_t i = 0; i < type->type_data.function.param_count; i++) {
      if (i > 0)
        fprintf(f, ", ");
      print_type(f, type->type_data.function.param_types[i]);
    }
    fprintf(f, ") ");
    print_type(f, type->type_data.function.return_type);
    break;

  case AST_TYPE_RESOLUTION:
    for (size_t i = 0; i < type->type_data.resolution.part_count; i++) {
      if (i > 0)
        fprintf(f, "::");
      fprintf(f, "%s", type->type_data.resolution.parts[i]);
    }
    break;

  default:
    fprintf(f, "UnknownType");
    break;
  }
}

// Helper to write doc comment, stopping at certain markers
static void write_doc_comment_until_marker(FILE *f, const char *doc, const char *stop_marker) {
  if (!doc || !*doc) return;
  
  const char *marker_pos = stop_marker ? strstr(doc, stop_marker) : NULL;
  const char *end = marker_pos ? marker_pos : (doc + strlen(doc));
  
  const char *line = doc;
  while (line < end) {
    const char *line_end = strchr(line, '\n');
    if (!line_end || line_end >= end) {
      line_end = end;
    }
    
    // Skip empty lines at the end
    if (line_end == end && line == line_end) {
      break;
    }
    
    fprintf(f, "%.*s\n", (int)(line_end - line), line);
    
    if (*line_end == '\n') {
      line = line_end + 1;
    } else {
      break;
    }
  }
}

// Generate documentation for a function
static void generate_function_docs(FILE *f, AstNode *func, DocGenConfig config) {
  const char *name = func->stmt.func_decl.name;
  const char *doc = func->stmt.func_decl.doc_comment;
  bool is_public = func->stmt.func_decl.is_public;
  
  if (!is_public && !config.include_private) {
    return;
  }
  
  // Find first doc marker
  const char *first_marker = NULL;
  const char *markers[] = {"# Parameters", "# Returns", "# Example", NULL};
  if (doc) for (int m = 0; markers[m]; m++) {
    const char *p = strstr(doc, markers[m]);
    if (p && (!first_marker || p < first_marker)) first_marker = p;
  }

  // Function header
  fprintf(f, "### `%s`\n\n", name);

  // Documentation comment (stop before first marker)
  if (doc && *doc) {
    write_doc_comment_until_marker(f, doc, first_marker);
    fprintf(f, "\n");
  }

  // Modifiers line
  fprintf(f, "```luma\n");
  if (func->stmt.func_decl.is_dll_import) {
    fprintf(f, "#dll_import(\"%s\"", func->stmt.func_decl.dll_name ? func->stmt.func_decl.dll_name : "");
    if (func->stmt.func_decl.dll_callconv) {
      fprintf(f, ", callconv: \"%s\"", func->stmt.func_decl.dll_callconv);
    }
    fprintf(f, ")\n");
  }
  if (func->stmt.func_decl.is_lib_import) {
    fprintf(f, "#lib_import(\"%s\")\n", func->stmt.func_decl.lib_name ? func->stmt.func_decl.lib_name : "");
  }
  if (func->stmt.func_decl.returns_ownership) {
    fprintf(f, "#returns_ownership\n");
  }
  if (func->stmt.func_decl.takes_ownership) {
    fprintf(f, "#takes_ownership\n");
  }
  fprintf(f, "%s %s -> fn(\n", is_public ? "pub" : "     ", name);

  // Parameters (one per line)
  for (size_t i = 0; i < func->stmt.func_decl.param_count; i++) {
    fprintf(f, "    ");
    fprintf(f, "%s: ", func->stmt.func_decl.param_names[i]);
    if (func->stmt.func_decl.param_types && func->stmt.func_decl.param_types[i]) {
      print_type(f, func->stmt.func_decl.param_types[i]);
    } else {
      fprintf(f, "?");
    }
    if (i < func->stmt.func_decl.param_count - 1)
      fprintf(f, ",");
    fprintf(f, "\n");
  }

  fprintf(f, ") ");
  if (func->stmt.func_decl.return_type) {
    print_type(f, func->stmt.func_decl.return_type);
  } else {
    fprintf(f, "void");
  }
  fprintf(f, "\n```\n\n");

  // Extract and print Parameters/Returns/Example sections
  if (doc) for (int m = 0; markers[m]; m++) {
    const char *section_start = strstr(doc, markers[m]);
    if (!section_start) continue;

    const char *section_end = NULL;
    for (int n = m + 1; markers[n]; n++) {
      const char *p = strstr(section_start + 1, markers[n]);
      if (p && (!section_end || p < section_end)) section_end = p;
    }
    if (!section_end) section_end = section_start + strlen(section_start);

    fprintf(f, "**%s:**\n", markers[m] + 2);
    const char *line = section_start;
    while (line < section_end) {
      const char *line_end = strchr(line, '\n');
      if (!line_end || line_end >= section_end) line_end = section_end;

      if (strncmp(line, markers[m], strlen(markers[m])) != 0) {
        fprintf(f, "%.*s\n", (int)(line_end - line), line);
      }

      if (*line_end == '\n') line = line_end + 1;
      else break;
    }
    fprintf(f, "\n");
  }
}

// Generate documentation for a struct
static void generate_struct_docs(FILE *f, AstNode *strct, DocGenConfig config) {
  const char *name = strct->stmt.struct_decl.name;
  const char *doc = strct->stmt.struct_decl.doc_comment;
  bool is_public = strct->stmt.struct_decl.is_public;

  if (!is_public && !config.include_private) {
    return;
  }

  // Struct header
  fprintf(f, "### `%s`\n\n", name);

  // Main struct documentation (stop before # Fields marker)
  if (doc && *doc) {
    const char *fields_marker = strstr(doc, "# Fields");
    if (fields_marker) {
      const char *line = doc;
      while (line < fields_marker) {
        const char *line_end = strchr(line, '\n');
        if (!line_end || line_end >= fields_marker) break;
        fprintf(f, "%.*s\n", (int)(line_end - line), line);
        line = line_end + 1;
      }
    } else {
      write_doc_comment(f, doc, 0);
    }
    fprintf(f, "\n");
  }

  // Public data fields (non-method members)
  bool has_public_fields = false;
  for (size_t i = 0; i < strct->stmt.struct_decl.public_count; i++) {
    AstNode *field = strct->stmt.struct_decl.public_members[i];
    if (field->type == AST_STMT_FIELD_DECL && !field->stmt.field_decl.function) {
      has_public_fields = true;
      break;
    }
  }

  if (has_public_fields) {
    fprintf(f, "| Field | Type | Description |\n");
    fprintf(f, "|-------|------|-------------|\n");
    for (size_t i = 0; i < strct->stmt.struct_decl.public_count; i++) {
      AstNode *field = strct->stmt.struct_decl.public_members[i];
      if (field->type == AST_STMT_FIELD_DECL && !field->stmt.field_decl.function) {
        const char *field_name = field->stmt.field_decl.name;
        const char *field_doc = field->stmt.field_decl.doc_comment;
        AstNode *field_type = field->stmt.field_decl.type;

        fprintf(f, "| `%s` | ", field_name);
        if (field_type) print_type(f, field_type);
        else fprintf(f, "?");
        fprintf(f, " | ");
        if (field_doc && *field_doc) {
          const char *end = strchr(field_doc, '\n');
          if (end) fprintf(f, "%.*s", (int)(end - field_doc), field_doc);
          else fprintf(f, "%s", field_doc);
        }
        fprintf(f, " |\n");
      }
    }
    fprintf(f, "\n");
  }

  // Public methods
  bool has_public_methods = false;
  for (size_t i = 0; i < strct->stmt.struct_decl.public_count; i++) {
    AstNode *field = strct->stmt.struct_decl.public_members[i];
    if (field->type == AST_STMT_FIELD_DECL && field->stmt.field_decl.function) {
      has_public_methods = true;
      break;
    }
  }

  if (has_public_methods) {
    fprintf(f, "**Methods:**\n\n");
    for (size_t i = 0; i < strct->stmt.struct_decl.public_count; i++) {
      AstNode *field = strct->stmt.struct_decl.public_members[i];
      if (field->type == AST_STMT_FIELD_DECL && field->stmt.field_decl.function) {
        const char *method_name = field->stmt.field_decl.name;
        const char *method_doc = field->stmt.field_decl.doc_comment;
        AstNode *method_func = field->stmt.field_decl.function;

        // Find first doc marker
        const char *method_markers[] = {"# Parameters", "# Returns", "# Example", NULL};
        const char *method_first = NULL;
        if (method_doc) for (int m = 0; method_markers[m]; m++) {
          const char *p = strstr(method_doc, method_markers[m]);
          if (p && (!method_first || p < method_first)) method_first = p;
        }

        // Method name as subheading
        fprintf(f, "#### `%s()`\n\n", method_name);

        // Main description (stop before first marker)
        if (method_doc && *method_doc) {
          write_doc_comment_until_marker(f, method_doc, method_first);
          fprintf(f, "\n");
        }

        // Method signature
        if (method_func && method_func->type == AST_STMT_FUNCTION) {
          fprintf(f, "```luma\n");
          if (method_func->stmt.func_decl.is_dll_import) {
            fprintf(f, "#dll_import(\"%s\"", method_func->stmt.func_decl.dll_name ? method_func->stmt.func_decl.dll_name : "");
            if (method_func->stmt.func_decl.dll_callconv) {
              fprintf(f, ", callconv: \"%s\"", method_func->stmt.func_decl.dll_callconv);
            }
            fprintf(f, ")\n");
          }
          if (method_func->stmt.func_decl.is_lib_import) {
            fprintf(f, "#lib_import(\"%s\")\n", method_func->stmt.func_decl.lib_name ? method_func->stmt.func_decl.lib_name : "");
          }
          if (method_func->stmt.func_decl.returns_ownership) {
            fprintf(f, "#returns_ownership\n");
          }
          if (method_func->stmt.func_decl.takes_ownership) {
            fprintf(f, "#takes_ownership\n");
          }
          fprintf(f, "%s -> fn(\n", method_name);
          for (size_t j = 0; j < method_func->stmt.func_decl.param_count; j++) {
            fprintf(f, "    %s: ", method_func->stmt.func_decl.param_names[j]);
            if (method_func->stmt.func_decl.param_types[j]) {
              print_type(f, method_func->stmt.func_decl.param_types[j]);
            } else {
              fprintf(f, "?");
            }
            if (j < method_func->stmt.func_decl.param_count - 1) fprintf(f, ",");
            fprintf(f, "\n");
          }
          fprintf(f, ") ");
          if (method_func->stmt.func_decl.return_type) {
            print_type(f, method_func->stmt.func_decl.return_type);
          } else {
            fprintf(f, "void");
          }
          fprintf(f, "\n```\n\n");
        }

        // Extract and print Parameters/Returns/Example sections
        if (method_doc) for (int m = 0; method_markers[m]; m++) {
          const char *sec_start = strstr(method_doc, method_markers[m]);
          if (!sec_start) continue;

          const char *sec_end = NULL;
          for (int n = m + 1; method_markers[n]; n++) {
            const char *p = strstr(sec_start + 1, method_markers[n]);
            if (p && (!sec_end || p < sec_end)) sec_end = p;
          }
          if (!sec_end) sec_end = sec_start + strlen(sec_start);

          fprintf(f, "**%s:**\n", method_markers[m] + 2);
          const char *line = sec_start;
          while (line < sec_end) {
            const char *line_end = strchr(line, '\n');
            if (!line_end || line_end >= sec_end) line_end = sec_end;

            if (strncmp(line, method_markers[m], strlen(method_markers[m])) != 0) {
              fprintf(f, "%.*s\n", (int)(line_end - line), line);
            }

            if (*line_end == '\n') line = line_end + 1;
            else break;
          }
          fprintf(f, "\n");
        }
      }
    }
  }

  // Private members omitted for brevity - add similar formatting if needed
}

static void generate_enum_docs(FILE *f, AstNode *enm, DocGenConfig config) {
  const char *name = enm->stmt.enum_decl.name;
  const char *doc = enm->stmt.enum_decl.doc_comment;
  bool is_public = enm->stmt.enum_decl.is_public;

  if (!is_public && !config.include_private) {
    return;
  }

  fprintf(f, "### %s `%s`\n\n", is_public ? "pub" : "priv", name);

  if (doc && *doc) {
    write_doc_comment(f, doc, 0);
    fprintf(f, "\n");
  }

  fprintf(f, "**Values:**\n\n");
  for (size_t i = 0; i < enm->stmt.enum_decl.member_count; i++) {
    fprintf(f, "- `%s`\n", enm->stmt.enum_decl.members[i]);
  }
  fprintf(f, "\n");
}

static void generate_var_docs(FILE *f, AstNode *var, DocGenConfig config) {
  (void)config;
  const char *name = var->stmt.var_decl.name;
  const char *doc = var->stmt.var_decl.doc_comment;
  bool is_mutable = var->stmt.var_decl.is_mutable;

  fprintf(f, "- **`%s`** : ", name);
  if (var->stmt.var_decl.var_type) {
    print_type(f, var->stmt.var_decl.var_type);
  } else {
    fprintf(f, "inferred");
  }
  fprintf(f, " *(%s)*", is_mutable ? "mutable" : "constant");
  if (doc && *doc) {
    const char *end = strchr(doc, '\n');
    fprintf(f, " — ");
    if (end) fprintf(f, "%.*s", (int)(end - doc), doc);
    else fprintf(f, "%s", doc);
  }
  fprintf(f, "\n");
}

// Forward declarations
static void generate_os_docs(FILE *f, AstNode *os_node, DocGenConfig config);
static void generate_link_docs(FILE *f, AstNode *link_node);

// Generate documentation for declarations inside a block
static void generate_block_decls(FILE *f, AstNode *block, DocGenConfig config) {
  if (!block || block->type != AST_STMT_BLOCK)
    return;

  for (size_t i = 0; i < block->stmt.block.stmt_count; i++) {
    AstNode *node = block->stmt.block.statements[i];
    if (!node)
      continue;

    switch (node->type) {
    case AST_STMT_STRUCT:
      generate_struct_docs(f, node, config);
      break;
    case AST_STMT_ENUM:
      generate_enum_docs(f, node, config);
      break;
    case AST_STMT_FUNCTION:
      generate_function_docs(f, node, config);
      break;
    case AST_STMT_VAR_DECL:
      generate_var_docs(f, node, config);
      break;
    case AST_PREPROCESSOR_OS:
      generate_os_docs(f, node, config);
      break;
    case AST_PREPROCESSOR_LINK:
      generate_link_docs(f, node);
      break;
    default:
      break;
    }
  }
}

// Generate documentation for an @os block
static void generate_os_docs(FILE *f, AstNode *os_node, DocGenConfig config) {
  for (size_t i = 0; i < os_node->preprocessor.os.arm_count; i++) {
    const char *platform = os_node->preprocessor.os.platforms[i];
    fprintf(f, "### `\"%s\"`\n\n", platform);
    generate_block_decls(f, os_node->preprocessor.os.bodies[i], config);
    fprintf(f, "\n");
  }

  if (os_node->preprocessor.os.has_default &&
      os_node->preprocessor.os.default_body) {
    fprintf(f, "### Default `_`\n\n");
    generate_block_decls(f, os_node->preprocessor.os.default_body, config);
    fprintf(f, "\n");
  }
}

// Generate documentation for @link directives
static void generate_link_docs(FILE *f, AstNode *link_node) {
  fprintf(f, "> **FFI library:** `%s`\n>\n",
          link_node->preprocessor.link.lib_name
              ? link_node->preprocessor.link.lib_name
              : "unknown");
}

bool generate_module_docs(AstNode *module, DocGenConfig config, FILE *f) {
  if (!module || module->type != AST_PREPROCESSOR_MODULE) {
    return false;
  }

  const char *module_name = module->preprocessor.module.name;
  const char *module_doc = module->preprocessor.module.doc_comment;

  fprintf(f, "# Module: %s\n\n", module_name ? module_name : "unnamed");

  if (module_doc && *module_doc) {
    write_doc_comment(f, module_doc, 0);
    fprintf(f, "\n");
  }

  fprintf(f, "## Table of Contents\n\n");
  fprintf(f, "- [Structures](#structures)\n");
  fprintf(f, "- [Enumerations](#enumerations)\n");
  fprintf(f, "- [Functions](#functions)\n");
  fprintf(f, "- [Variables](#variables)\n");

  bool has_os = false;
  bool has_links = false;
  for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
    AstNode *node = module->preprocessor.module.body[i];
    if (!node) continue;
    if (node->type == AST_PREPROCESSOR_OS) has_os = true;
    if (node->type == AST_PREPROCESSOR_LINK) has_links = true;
  }
  if (has_os) fprintf(f, "- [OS-Specific](#os-specific)\n");
  if (has_links) fprintf(f, "- [Linked Libraries](#linked-libraries)\n");
  fprintf(f, "\n");

  if (module->preprocessor.module.body) {
    bool has_structs = false;
    bool has_enums = false;
    bool has_functions = false;
    bool has_vars = false;

    // First pass: check what we have
    for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
      AstNode *node = module->preprocessor.module.body[i];
      if (!node)
        continue;

      switch (node->type) {
      case AST_STMT_STRUCT:
        has_structs = true;
        break;
      case AST_STMT_ENUM:
        has_enums = true;
        break;
      case AST_STMT_FUNCTION:
        has_functions = true;
        break;
      case AST_STMT_VAR_DECL:
        if (node->stmt.var_decl.doc_comment) {
          has_vars = true;
        }
        break;
      default:
        break;
      }
    }

    if (has_structs) {
      fprintf(f, "---\n\n## Structures\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_STRUCT) {
          generate_struct_docs(f, node, config);
        }
      }
    }

    if (has_enums) {
      fprintf(f, "\n## Enumerations\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_ENUM) {
          generate_enum_docs(f, node, config);
        }
      }
    }

    if (has_functions) {
      fprintf(f, "\n## Functions\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_FUNCTION) {
          generate_function_docs(f, node, config);
        }
      }
    }

    if (has_vars) {
      fprintf(f, "\n## Variables\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_VAR_DECL) {
          generate_var_docs(f, node, config);
        }
      }
    }

    if (has_os) {
      fprintf(f, "\n## OS-Specific\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_PREPROCESSOR_OS) {
          generate_os_docs(f, node, config);
        }
      }
    }

    if (has_links) {
      fprintf(f, "\n## Linked Libraries\n\n");
      fprintf(f, "External native libraries linked by this module.\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_PREPROCESSOR_LINK) {
          generate_link_docs(f, node);
        }
      }
      fprintf(f, "\n");
    }
  }

  return true;
}

bool generate_documentation(AstNode *program, DocGenConfig config) {
  if (!program || program->type != AST_PROGRAM) {
    fprintf(stderr, "Invalid program node for documentation generation\n");
    return false;
  }

  if (!ensure_directory(config.output_dir)) {
    return false;
  }

  printf("Generating documentation in %s/...\n", config.output_dir);

  char index_path[512];
  snprintf(index_path, sizeof(index_path), "%s/README.md", config.output_dir);

  FILE *index_file = fopen(index_path, "w");
  if (!index_file) {
    fprintf(stderr, "Failed to create index file: %s\n", index_path);
    return false;
  }

  fprintf(index_file, "# API Documentation\n\n");
  fprintf(index_file, "Generated documentation for the project.\n\n");
  fprintf(index_file, "## Modules\n\n");

  bool success = true;
  for (size_t i = 0; i < program->stmt.program.module_count; i++) {
    AstNode *module = program->stmt.program.modules[i];

    if (!module || module->type != AST_PREPROCESSOR_MODULE) {
      continue;
    }

    const char *module_name = module->preprocessor.module.name;
    if (!module_name) {
      module_name = "unnamed";
    }

    fprintf(index_file, "- [%s](%s.md)\n", module_name, module_name);

    char doc_path[512];
    snprintf(doc_path, sizeof(doc_path), "%s/%s.md", config.output_dir,
             module_name);

    FILE *doc_file = fopen(doc_path, "w");
    if (!doc_file) {
      fprintf(stderr, "Failed to create documentation file: %s\n", doc_path);
      success = false;
      continue;
    }

    if (!generate_module_docs(module, config, doc_file)) {
      fprintf(stderr, "Failed to generate documentation for module: %s\n",
              module_name);
      success = false;
    }

    fclose(doc_file);
    printf("  Generated: %s\n", doc_path);
  }

  fclose(index_file);

  if (success) {
    printf("✓ Documentation generated successfully in %s/\n",
           config.output_dir);
  }

  return success;
}

DocGenConfig create_doc_config(ArenaAllocator *arena, const char *output_dir) {
  DocGenConfig config = {
      .output_dir = output_dir ? output_dir : "docs",
      .format = "markdown",
      .include_private = false,
      .include_source_links = false,
      .arena = arena,
  };
  return config;
}