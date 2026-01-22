#include "../ast/ast_utils.h"
#include "../auto_docs/doc_generator.h"
#include "../c_libs/error/error.h"
#include "../llvm/llvm.h"
#include "../parser/parser.h"
#include "../typechecker/type.h"
#include "help.h"
#include "std_path.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

// Helper function to create directory if it doesn't exist
// NOTE Replaced with macro
#ifdef _WIN32
#define create_directory(path) (mkdir((path)) == 0 || errno == EEXIST)
#else
#define create_directory(path) (mkdir((path), 0755) == 0 || errno == EEXIST)
#endif

void handle_segfault(int sig) {
  (void)sig;
  fprintf(stderr, "\nSegmentation fault!\n");
  fprintf(stderr, "This likely indicates a problem in LLVM IR generation.\n");
  exit(1);
}

void handle_illegal_instruction(int sig) {
  (void)sig;
  fprintf(stderr, "\nIllegal instruction caught!\n");
  fprintf(stderr, "This suggests LLVM generated invalid machine code.\n");
  fprintf(stderr,
          "Check your target architecture and LLVM version compatibility.\n");
  exit(1);
}

void save_module_output_files(CodeGenContext *ctx, const char *output_dir) {
  // Create output directory if it doesn't exist
  if (!create_directory(output_dir)) {
    fprintf(stderr, "Warning: Failed to create output directory: %s\n",
            output_dir);
  }

  // Save IR and assembly for each module
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    char filename[512];

    // Set current module for legacy functions
    set_current_module(ctx, unit);
    ctx->module = unit->module; // Update legacy field

    // Save readable LLVM IR
    snprintf(filename, sizeof(filename), "%s/%s.ll", output_dir,
             unit->module_name);
    char *ir = print_llvm_ir(ctx);
    if (ir) {
      FILE *f = fopen(filename, "w");
      if (f) {
        fprintf(f, "%s", ir);
        fclose(f);
        // printf("Generated IR file: %s\n", filename);
      }
      LLVMDisposeMessage(ir);
    }

    // Generate assembly file
    snprintf(filename, sizeof(filename), "%s/%s.s", output_dir,
             unit->module_name);
    generate_assembly_file(ctx, filename);
    // printf("Generated assembly file: %s\n", filename);
  }
}

// ADD THIS NEW HELPER FUNCTION
const char *resolve_import_path(const char *path, ArenaAllocator *allocator) {
  // Check if this is a std/ import
  if (strncmp(path, "std/", 4) == 0 || strncmp(path, "std\\", 4) == 0) {
    char resolved_path[1024];
    if (resolve_std_path(path, resolved_path, sizeof(resolved_path))) {
      fprintf(stderr, "[import] %s -> %s\n", path, resolved_path);
      return arena_strdup(allocator, resolved_path);
    } else {
      fprintf(stderr, "Error: Could not find standard library file: %s\n",
              path);
      fprintf(stderr, "\n");
      print_std_search_paths();
      return NULL;
    }
  }

  // Fallback: if path is a bare module name (no directory separators),
  // attempt to resolve from the standard library
#ifndef _WIN32
  bool has_sep = strchr(path, '/') != NULL;
#else
  bool has_sep = strchr(path, '/') != NULL || strchr(path, '\\') != NULL;
#endif

  if (!has_sep) {
    char std_prefixed[512];
    snprintf(std_prefixed, sizeof(std_prefixed), "std/%s", path);

    char resolved_path[1024];
    if (resolve_std_path(std_prefixed, resolved_path, sizeof(resolved_path))) {
      fprintf(stderr, "[import] %s -> %s\n", path, resolved_path);
      return arena_strdup(allocator, resolved_path);
    }
  }

  // Not a std import or fallback failed; return as-is
  return path;
}

bool generate_llvm_code_modules(AstNode *root, BuildConfig config,
                                ArenaAllocator *allocator, int *step,
                                CompileTimer *timer) {
  CodeGenContext *ctx = init_codegen_context(allocator);
  if (!ctx) {
    return false;
  }

  const char *base_name = config.name ? config.name : "output";
  const char *output_dir = config.save ? "output" : "obj";

  if (!create_directory(output_dir)) {
    fprintf(stderr, "Failed to create output directory: %s\n", output_dir);
    cleanup_codegen_context(ctx);
    return false;
  }

  signal(SIGSEGV, handle_segfault);
  signal(SIGILL, handle_illegal_instruction);

  bool success = generate_program_modules(ctx, root, output_dir);

  if (!success) {
    fprintf(stderr, "Failed to generate LLVM modules\n");
    cleanup_codegen_context(ctx);
    return false;
  }

  preprocess_all_modules(ctx);

  print_progress_with_time(++(*step), 9, "LLVM IR Generation", timer);

  if (config.save) {
    save_module_output_files(ctx, output_dir);
    debug_object_files(output_dir);
  }

  char exe_file[256];
  snprintf(exe_file, sizeof(exe_file), "%s", base_name);

  if (!link_object_files(ctx, output_dir, exe_file, config.opt_level)) {
    cleanup_codegen_context(ctx);
    return false;
  }

  print_progress_with_time(++(*step), 9, "Linking", timer);

  cleanup_module_caches();
  
  cleanup_codegen_context(ctx);
  return true;
}

bool link_object_files(CodeGenContext *ctx, const char *output_dir, const char *executable_name,
                       int opt_level) {
  char command[4096];
#ifdef __APPLE__
  if (opt_level > 0) {
    snprintf(command, sizeof(command), "cc -O%d -Wl,-dead_strip -o %s", opt_level, executable_name);
  } else {
    snprintf(command, sizeof(command), "cc -Wl,-dead_strip -o %s", executable_name);
  }
#else
  if (opt_level > 0) {
    snprintf(command, sizeof(command), "cc -O%d -pie -o %s", opt_level, executable_name);
  } else {
    snprintf(command, sizeof(command), "cc -pie -o %s", executable_name);
  }
#endif
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    char obj_path[512];
    snprintf(obj_path, sizeof(obj_path), " %s/%s.o", output_dir, unit->module_name);
    strncat(command, obj_path, sizeof(command) - strlen(command) - 1);
  }
  int result = system(command);
  if (result != 0) {
    char alt_command[4096];
#ifdef __APPLE__
    size_t alt_len = snprintf(alt_command, sizeof(alt_command), "gcc -O%d -Wl,-dead_strip -o %s",
                              opt_level, executable_name);
    (void)alt_len;
#else
    size_t alt_len = snprintf(alt_command, sizeof(alt_command), "gcc -O%d -no-pie -o %s",
                              opt_level, executable_name);
    (void)alt_len;
#endif
    for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
      char obj_path[512];
      snprintf(obj_path, sizeof(obj_path), " %s/%s.o", output_dir, unit->module_name);
      strncat(alt_command, obj_path, sizeof(alt_command) - strlen(alt_command) - 1);
    }
    result = system(alt_command);
    if (result != 0) {
      return false;
    }
  }
#ifdef __APPLE__
  {
    char strip_cmd[1024];
    snprintf(strip_cmd, sizeof(strip_cmd), "strip -x %s", executable_name);
    (void)system(strip_cmd);
  }
#endif
  return true;
}

// UPDATED parse_file_to_module
Stmt *parse_file_to_module(const char *path, size_t position,
                           ArenaAllocator *allocator, BuildConfig *config) {
  // Resolve the path if it's a std/ import
  const char *resolved_path = resolve_import_path(path, allocator);
  if (!resolved_path) {
    return NULL;
  }

  const char *source = read_file(resolved_path);
  if (!source) {
    fprintf(stderr, "Failed to read source file: %s\n", resolved_path);
    return NULL;
  }

  Lexer lexer;
  init_lexer(&lexer, source, allocator);

  GrowableArray tokens;
  if (!growable_array_init(&tokens, allocator, MAX_TOKENS, sizeof(Token))) {
    fprintf(stderr, "Failed to initialize token array for %s.\n",
            resolved_path);
    free((void *)source);
    return NULL;
  }

  Token tk;
  while ((tk = next_token(&lexer)).type_ != TOK_EOF) {
    Token *slot = (Token *)growable_array_push(&tokens);
    if (!slot) {
      fprintf(stderr, "Out of memory while growing token array\n");
      free((void *)source);
      return NULL;
    }
    *slot = tk;
  }

  if (error_report()) {
    free((void *)source);
    return NULL;
  }

  // Temporarily update config for parsing
  GrowableArray old_tokens = config->tokens;
  size_t old_count = config->token_count;

  config->tokens = tokens;
  config->token_count = tokens.count;

  AstNode *program_root = parse(&tokens, allocator, config);
  free((void *)source);

  if (!program_root) {
    return NULL;
  }

  // Extract the module
  if (program_root->type == AST_PROGRAM &&
      program_root->stmt.program.module_count > 0) {
    Stmt *module = (Stmt *)program_root->stmt.program.modules[0];

    if (module && module->type == AST_PREPROCESSOR_MODULE) {
      module->preprocessor.module.potions = position;
      module->preprocessor.module.tokens = (Token *)tokens.data;
      module->preprocessor.module.token_count = tokens.count;
    }

    char abs_path[4096];
    const char *file_path_to_store = resolved_path;

#ifndef _WIN32
    if (realpath(resolved_path, abs_path)) {
      file_path_to_store = arena_strdup(allocator, abs_path);
    }
#else
    if (_fullpath(abs_path, resolved_path, sizeof(abs_path))) {
      file_path_to_store = arena_strdup(allocator, abs_path);
    }
#endif

    module->preprocessor.module.file_path = file_path_to_store;

    // Restore config
    config->tokens = old_tokens;
    config->token_count = old_count;

    return module;
  }

  config->tokens = old_tokens;
  config->token_count = old_count;
  return NULL;
}

// UPDATED lex_and_parse_file
AstNode *lex_and_parse_file(const char *path, ArenaAllocator *allocator,
                            BuildConfig *config) {
  // Resolve the path if it's a std/ import
  const char *resolved_path = resolve_import_path(path, allocator);
  if (!resolved_path) {
    return NULL;
  }

  const char *source = read_file(resolved_path);
  if (!source) {
    fprintf(stderr, "Failed to read source file: %s\n", resolved_path);
    return NULL;
  }

  Lexer lexer;
  init_lexer(&lexer, source, allocator);

  GrowableArray tokens;
  if (!growable_array_init(&tokens, allocator, MAX_TOKENS, sizeof(Token))) {
    fprintf(stderr, "Failed to initialize token array for %s.\n",
            resolved_path);
    free((void *)source);
    return NULL;
  }

  Token tk;
  while ((tk = next_token(&lexer)).type_ != TOK_EOF) {
    Token *slot = (Token *)growable_array_push(&tokens);
    if (!slot) {
      fprintf(stderr, "Out of memory while growing token array\n");
      free((void *)source);
      return NULL;
    }
    *slot = tk;
  }

  if (error_report()) {
    free((void *)source);
    return NULL;
  }

  AstNode *root = parse(&tokens, allocator, config);
  print_ast(root, "", false, false);

  free((void *)source);
  return root;
}

bool run_build(BuildConfig config, ArenaAllocator *allocator) {
  bool success = false;
  int total_stages =
      config.is_document ? 4 : 10; // Fewer stages for doc generation
  int step = 0;

  // START TIMER
  CompileTimer timer;
  timer_start(&timer);

  GrowableArray modules;
  if (!growable_array_init(&modules, allocator, 16, sizeof(AstNode *))) {
    return false;
  }

  // Stage 1: Lexing
  print_progress_with_time(++step, total_stages, "Lexing", &timer);

  for (size_t i = 0; i < config.file_count; i++) {
    char **files_array = (char **)config.files.data;

    Stmt *module = parse_file_to_module(files_array[i], i, allocator, &config);
    if (!module || error_report())
      goto cleanup;

    AstNode **slot = (AstNode **)growable_array_push(&modules);
    if (!slot)
      goto cleanup;
    *slot = (AstNode *)module;
  }

  // Stage 2: Parsing
  print_progress_with_time(++step, total_stages, "Parsing", &timer);

  Stmt *main_module = parse_file_to_module(config.filepath, config.file_count,
                                           allocator, &config);
  if (!main_module || error_report())
    goto cleanup;

  AstNode **main_slot = (AstNode **)growable_array_push(&modules);
  if (!main_slot)
    goto cleanup;
  *main_slot = (AstNode *)main_module;

  // Automatically parse and add imported modules (@use) from the main module
  if (main_module && main_module->type == AST_PREPROCESSOR_MODULE) {
    AstNode **body = main_module->preprocessor.module.body;
    int body_count = main_module->preprocessor.module.body_count;

    for (int j = 0; j < body_count; j++) {
      AstNode *stmt = body[j];
      if (!stmt)
        continue;

      if (stmt->type == AST_PREPROCESSOR_USE) {
        const char *import_name = stmt->preprocessor.use.module_name;
        if (!import_name || !*import_name)
          continue;

        bool already_added = false;
        for (size_t k = 0; k < modules.count; k++) {
          AstNode *existing =
              ((AstNode **)modules.data)[k];
          if (existing && existing->type == AST_PREPROCESSOR_MODULE) {
            const char *existing_name =
                existing->preprocessor.module.name;
            if (existing_name && strcmp(existing_name, import_name) == 0) {
              already_added = true;
              break;
            }
          }
        }

        if (!already_added) {
          Stmt *import_module =
              parse_file_to_module(import_name, modules.count, allocator,
                                   &config);
          if (!import_module || error_report())
            goto cleanup;

          AstNode **slot = (AstNode **)growable_array_push(&modules);
          if (!slot)
            goto cleanup;
          *slot = (AstNode *)import_module;
        }
      }
    }
  }

  // Stage 3: Combining modules
  print_progress_with_time(++step, total_stages, "Module Combination", &timer);

  AstNode *combined_program = create_program_node(
      allocator, (AstNode **)modules.data, modules.count, 0, 0);
  if (!combined_program)
    goto cleanup;

  // CHECK FOR DOCUMENTATION MODE
  if (config.is_document) {
    print_progress_with_time(++step, total_stages, "Generating Documentation",
                             &timer);

    // Create documentation configuration
    DocGenConfig doc_config = create_doc_config(allocator, "docs");
    doc_config.include_private = false; // Only document public APIs

    // Generate documentation
    success = generate_documentation(combined_program, doc_config);

    if (success) {
      print_progress_with_time(++step, total_stages, "Completed", &timer);
      timer_stop(&timer);

      if (timer.elapsed_ms < 1000.0) {
        printf("Documentation generated successfully! (%.0fms)\n",
               timer.elapsed_ms);
      } else {
        printf("Documentation generated successfully! (%.2fs)\n",
               timer.elapsed_ms / 1000.0);
      }
    } else {
      fprintf(stderr, "Failed to generate documentation\n");
    }

    goto cleanup;
  }

  // NORMAL BUILD PROCESS (if not in documentation mode)

  // Stage 4: Typechecking
  print_progress_with_time(++step, total_stages, "Typechecking", &timer);

  Scope root_scope;
  init_scope(&root_scope, NULL, "global", allocator);
  bool tc = typecheck(combined_program, &root_scope, allocator, &config);
  if (error_report()) {
    goto cleanup;
  } else {
    ++step;
  }

  if (tc) {
    // Stage 6: LLVM IR
    print_progress_with_time(++step, total_stages, "LLVM IR", &timer);
    if (!combined_program || combined_program->type != AST_PROGRAM) {
      fprintf(stderr, "ERROR: Invalid program node before codegen\n");
      goto cleanup;
    }
    success = generate_llvm_code_modules(combined_program, config, allocator,
                                         &step, &timer);
  }

  // Stage 7: Finalizing
  print_progress_with_time(++step, total_stages, "Finalizing", &timer);
  print_progress_with_time(++step, total_stages, "Completed", &timer);

  // STOP TIMER AND PRINT FINAL TIME
  timer_stop(&timer);

  if (timer.elapsed_ms < 1000.0) {
    printf("Build succeeded! Written to '%s' (%.0fms)\n",
           config.name ? config.name : "output", timer.elapsed_ms);
  } else {
    printf("Build succeeded! Written to '%s' (%.2fs)\n",
           config.name ? config.name : "output", timer.elapsed_ms / 1000.0);
  }

cleanup:
  return success;
}
