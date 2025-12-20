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

// Escape markdown special characters
static void write_escaped_markdown(FILE *f, const char *text) {
  if (!text)
    return;

  for (const char *p = text; *p; p++) {
    switch (*p) {
    case '*':
    case '_':
    case '`':
    case '[':
    case ']':
    case '#':
      fputc('\\', f);
      // fallthrough
    default:
      fputc(*p, f);
    }
  }
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
  
  // Function header
  fprintf(f, "### %s `%s`\n\n", is_public ? "public" : "private", name);
  
  // Documentation comment (stop before # Parameters or # Returns if present)
  if (doc && *doc) {
    write_doc_comment_until_marker(f, doc, "# Parameters");
    write_doc_comment_until_marker(f, doc, "# Returns");
    write_doc_comment_until_marker(f, doc, "# Example");
    fprintf(f, "\n");
  }
  
  // Function signature
  fprintf(f, "**Signature:**\n```luma\n");
  
  // Modifiers
  if (func->stmt.func_decl.returns_ownership) {
    fprintf(f, "#returns_ownership ");
  }
  if (func->stmt.func_decl.takes_ownership) {
    fprintf(f, "#takes_ownership ");
  }
  if (is_public) {
    fprintf(f, "pub ");
  }
  
  fprintf(f, "const %s -> fn(", name);
  
  // Parameters
  for (size_t i = 0; i < func->stmt.func_decl.param_count; i++) {
    if (i > 0) fprintf(f, ", ");
    fprintf(f, "%s: ", func->stmt.func_decl.param_names[i]);
    if (func->stmt.func_decl.param_types && func->stmt.func_decl.param_types[i]) {
      print_type(f, func->stmt.func_decl.param_types[i]);
    } else {
      fprintf(f, "?");
    }
  }
  
  fprintf(f, ") ");
  
  // Return type
  if (func->stmt.func_decl.return_type) {
    print_type(f, func->stmt.func_decl.return_type);
  } else {
    fprintf(f, "void");
  }
  
  fprintf(f, ";\n```\n\n");
  
  // Now print Parameters section if it exists
  if (doc && strstr(doc, "# Parameters")) {
    const char *params_start = strstr(doc, "# Parameters");
    const char *params_end = strstr(params_start + 1, "\n#");
    if (!params_end) params_end = params_start + strlen(params_start);
    
    fprintf(f, "**Parameters:**\n");
    const char *line = params_start;
    while (line < params_end) {
      const char *line_end = strchr(line, '\n');
      if (!line_end) line_end = params_end;
      
      // Skip the "# Parameters" header itself
      if (strncmp(line, "# Parameters", 12) != 0) {
        fprintf(f, "%.*s\n", (int)(line_end - line), line);
      }
      
      if (*line_end == '\n') {
        line = line_end + 1;
      } else {
        break;
      }
    }
    fprintf(f, "\n");
  }
  
  // Print Returns section if it exists
  if (doc && strstr(doc, "# Returns")) {
    const char *returns_start = strstr(doc, "# Returns");
    const char *returns_end = strstr(returns_start + 1, "\n#");
    if (!returns_end) returns_end = returns_start + strlen(returns_start);
    
    fprintf(f, "**Returns:**\n");
    const char *line = returns_start;
    while (line < returns_end) {
      const char *line_end = strchr(line, '\n');
      if (!line_end) line_end = returns_end;
      
      // Skip the "# Returns" header itself
      if (strncmp(line, "# Returns", 9) != 0) {
        fprintf(f, "%.*s\n", (int)(line_end - line), line);
      }
      
      if (*line_end == '\n') {
        line = line_end + 1;
      } else {
        break;
      }
    }
    fprintf(f, "\n");
  }
  
  // Print Example section if it exists
  if (doc && strstr(doc, "# Example")) {
    const char *example_start = strstr(doc, "# Example");
    const char *example_end = strstr(example_start + 1, "\n#");
    if (!example_end) example_end = example_start + strlen(example_start);
    
    fprintf(f, "**Example:**\n");
    const char *line = example_start;
    while (line < example_end) {
      const char *line_end = strchr(line, '\n');
      if (!line_end) line_end = example_end;
      
      // Skip the "# Example" header itself
      if (strncmp(line, "# Example", 9) != 0) {
        fprintf(f, "%.*s\n", (int)(line_end - line), line);
      }
      
      if (*line_end == '\n') {
        line = line_end + 1;
      } else {
        break;
      }
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
  fprintf(f, "### %s `%s`\n\n", is_public ? "public" : "private", name);

  // Main struct documentation (stop before # Fields marker)
  if (doc && *doc) {
    // Find where # Fields starts and stop before it
    const char *fields_marker = strstr(doc, "# Fields");
    if (fields_marker) {
      // Print everything before # Fields
      const char *line = doc;
      while (line < fields_marker) {
        const char *line_end = strchr(line, '\n');
        if (!line_end || line_end >= fields_marker) {
          break;
        }
        fprintf(f, "%.*s\n", (int)(line_end - line), line);
        line = line_end + 1;
      }
    } else {
      // No # Fields marker, print everything
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
    fprintf(f, "**Fields:**\n\n");
    for (size_t i = 0; i < strct->stmt.struct_decl.public_count; i++) {
      AstNode *field = strct->stmt.struct_decl.public_members[i];
      if (field->type == AST_STMT_FIELD_DECL && !field->stmt.field_decl.function) {
        const char *field_name = field->stmt.field_decl.name;
        const char *field_doc = field->stmt.field_decl.doc_comment;
        AstNode *field_type = field->stmt.field_decl.type;

        fprintf(f, "- `%s`: ", field_name);
        
        // Print type
        if (field_type) {
          print_type(f, field_type);
        } else {
          fprintf(f, "?");
        }

        // Print doc comment
        if (field_doc && *field_doc) {
          fprintf(f, " — ");
          // Get first line only for brief description
          const char *end = strchr(field_doc, '\n');
          if (end) {
            fprintf(f, "%.*s", (int)(end - field_doc), field_doc);
          } else {
            fprintf(f, "%s", field_doc);
          }
        }
        fprintf(f, "\n");
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

        // Method name as subheading
        fprintf(f, "#### `%s()`\n\n", method_name);

        // Main description (stop before # Parameters, # Returns, # Example)
        if (method_doc && *method_doc) {
          const char *stop_at = strstr(method_doc, "# Parameters");
          if (!stop_at) stop_at = strstr(method_doc, "# Returns");
          if (!stop_at) stop_at = strstr(method_doc, "# Example");
          
          if (stop_at) {
            const char *line = method_doc;
            while (line < stop_at) {
              const char *line_end = strchr(line, '\n');
              if (!line_end || line_end >= stop_at) {
                break;
              }
              fprintf(f, "%.*s\n", (int)(line_end - line), line);
              line = line_end + 1;
            }
          } else {
            write_doc_comment(f, method_doc, 0);
          }
          fprintf(f, "\n");
        }

        // Method signature
        if (method_func && method_func->type == AST_STMT_FUNCTION) {
          fprintf(f, "```luma\n");
          
          // Modifiers
          if (method_func->stmt.func_decl.returns_ownership) {
            fprintf(f, "#returns_ownership ");
          }
          if (method_func->stmt.func_decl.takes_ownership) {
            fprintf(f, "#takes_ownership ");
          }
          
          fprintf(f, "%s -> fn(", method_name);
          
          // Parameters
          for (size_t j = 0; j < method_func->stmt.func_decl.param_count; j++) {
            if (j > 0) fprintf(f, ", ");
            fprintf(f, "%s: ", method_func->stmt.func_decl.param_names[j]);
            if (method_func->stmt.func_decl.param_types[j]) {
              print_type(f, method_func->stmt.func_decl.param_types[j]);
            } else {
              fprintf(f, "?");
            }
          }
          
          fprintf(f, ") ");
          
          // Return type
          if (method_func->stmt.func_decl.return_type) {
            print_type(f, method_func->stmt.func_decl.return_type);
          } else {
            fprintf(f, "void");
          }
          
          fprintf(f, "\n```\n\n");
        }
        
        // Parameters section
        if (method_doc && strstr(method_doc, "# Parameters")) {
          const char *params_start = strstr(method_doc, "# Parameters");
          const char *params_end = strstr(params_start + 1, "\n#");
          if (!params_end) params_end = params_start + strlen(params_start);
          
          fprintf(f, "**Parameters:**\n");
          const char *line = params_start;
          while (line < params_end) {
            const char *line_end = strchr(line, '\n');
            if (!line_end) line_end = params_end;
            
            if (strncmp(line, "# Parameters", 12) != 0) {
              fprintf(f, "%.*s\n", (int)(line_end - line), line);
            }
            
            if (*line_end == '\n') {
              line = line_end + 1;
            } else {
              break;
            }
          }
          fprintf(f, "\n");
        }
        
        // Returns section
        if (method_doc && strstr(method_doc, "# Returns")) {
          const char *returns_start = strstr(method_doc, "# Returns");
          const char *returns_end = strstr(returns_start + 1, "\n#");
          if (!returns_end) returns_end = returns_start + strlen(returns_start);
          
          fprintf(f, "**Returns:**\n");
          const char *line = returns_start;
          while (line < returns_end) {
            const char *line_end = strchr(line, '\n');
            if (!line_end) line_end = returns_end;
            
            if (strncmp(line, "# Returns", 9) != 0) {
              fprintf(f, "%.*s\n", (int)(line_end - line), line);
            }
            
            if (*line_end == '\n') {
              line = line_end + 1;
            } else {
              break;
            }
          }
          fprintf(f, "\n");
        }
        
        // Example section
        if (method_doc && strstr(method_doc, "# Example")) {
          const char *example_start = strstr(method_doc, "# Example");
          const char *example_end = strstr(example_start + 1, "\n#");
          if (!example_end) example_end = example_start + strlen(example_start);
          
          fprintf(f, "**Example:**\n");
          const char *line = example_start;
          while (line < example_end) {
            const char *line_end = strchr(line, '\n');
            if (!line_end) line_end = example_end;
            
            if (strncmp(line, "# Example", 9) != 0) {
              fprintf(f, "%.*s\n", (int)(line_end - line), line);
            }
            
            if (*line_end == '\n') {
              line = line_end + 1;
            } else {
              break;
            }
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

  // Enum header
  fprintf(f, "### %s `%s`\n\n", is_public ? "pub" : "priv", name);

  // Documentation comment
  if (doc && *doc) {
    write_doc_comment(f, doc, 0);
    fprintf(f, "\n");
  }

  // Enum values
  fprintf(f, "**Values:**\n\n");
  for (size_t i = 0; i < enm->stmt.enum_decl.member_count; i++) {
    fprintf(f, "- `%s`\n", enm->stmt.enum_decl.members[i]);
  }
  fprintf(f, "\n");
}

// Variables - simpler formatting
static void generate_var_docs(FILE *f, AstNode *var, DocGenConfig config) {
  const char *name = var->stmt.var_decl.name;
  const char *doc = var->stmt.var_decl.doc_comment;
  bool is_public = var->stmt.var_decl.is_public;
  bool is_mutable = var->stmt.var_decl.is_mutable;
  
  // if (!is_public && !config.include_private) {
  //   return;
  // }
  
  // Variable header
  fprintf(f, "### %s `%s`\n\n", 
          is_public ? "public" : "private", 
          name);
  
  // Type info
  fprintf(f, "**Type:** ");
  if (var->stmt.var_decl.var_type) {
    print_type(f, var->stmt.var_decl.var_type);
  } else {
    fprintf(f, "inferred");
  }
  fprintf(f, " (%s)\n\n", is_mutable ? "mutable" : "constant");
  
  // Documentation
  if (doc && *doc) {
    write_doc_comment(f, doc, 0);
    fprintf(f, "\n");
  }
}

bool generate_module_docs(AstNode *module, DocGenConfig config, FILE *f) {
  if (!module || module->type != AST_PREPROCESSOR_MODULE) {
    return false;
  }

  const char *module_name = module->preprocessor.module.name;
  const char *module_doc = module->preprocessor.module.doc_comment;

  // Module header
  fprintf(f, "# Module: %s\n\n", module_name ? module_name : "unnamed");

  // Module documentation
  if (module_doc && *module_doc) {
    write_doc_comment(f, module_doc, 0);
    fprintf(f, "\n");
  }

  // Table of contents
  fprintf(f, "## Table of Contents\n\n");
  fprintf(f, "- [Structures](#structures)\n");
  fprintf(f, "- [Enumerations](#enumerations)\n");
  fprintf(f, "- [Functions](#functions)\n");
  fprintf(f, "- [Variables](#variables)\n\n");

  fprintf(f, "---\n\n");

  // Process module body
  if (module->preprocessor.module.body) {
    // Collect different declaration types
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

    // Structures section
    if (has_structs) {
      fprintf(f, "## Structures\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_STRUCT) {
          generate_struct_docs(f, node, config);
        }
      }
    }

    // Enumerations section
    if (has_enums) {
      fprintf(f, "## Enumerations\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_ENUM) {
          generate_enum_docs(f, node, config);
        }
      }
    }

    // Functions section
    if (has_functions) {
      fprintf(f, "## Functions\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_FUNCTION) {
          generate_function_docs(f, node, config);
        }
      }
    }

    // Variables section
    if (has_vars) {
      fprintf(f, "## Variables\n\n");
      for (size_t i = 0; i < module->preprocessor.module.body_count; i++) {
        AstNode *node = module->preprocessor.module.body[i];
        if (node && node->type == AST_STMT_VAR_DECL) {
          generate_var_docs(f, node, config);
        }
      }
    }
  }

  return true;
}

bool generate_documentation(AstNode *program, DocGenConfig config) {
  if (!program || program->type != AST_PROGRAM) {
    fprintf(stderr, "Invalid program node for documentation generation\n");
    return false;
  }

  // Ensure output directory exists
  if (!ensure_directory(config.output_dir)) {
    return false;
  }

  printf("Generating documentation in %s/...\n", config.output_dir);

  // Generate index file
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

  // Generate documentation for each module
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

    // Add to index
    fprintf(index_file, "- [%s](%s.md)\n", module_name, module_name);

    // Generate module documentation file
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