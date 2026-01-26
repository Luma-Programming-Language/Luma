// Enhanced llvm.c - Module system implementation
#include "../llvm.h"
#include <llvm-c/Linker.h>
#include <llvm-c/TargetMachine.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>

// Platform-specific includes for CPU detection
#ifdef _WIN32
#include <windows.h>
#elif defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/sysctl.h>
#include <sys/types.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#define DEFAULT_COMPILE_THREADS 4
#define MAX_COMPILE_THREADS 64
#define MAX_PATH_LENGTH 512

typedef struct {
  ModuleCompilationUnit *module;
  const char *output_dir;
  bool success;
  double compile_time;
} ModuleCompileTask;

static size_t detect_cpu_count(void) {
#ifdef _WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return (size_t)sysinfo.dwNumberOfProcessors;
#elif defined(__APPLE__) || defined(__FreeBSD__)
  int mib[2] = {CTL_HW, HW_NCPU};
  int cpu_count;
  size_t len = sizeof(cpu_count);
  if (sysctl(mib, 2, &cpu_count, &len, NULL, 0) == 0 && cpu_count > 0) {
    return (size_t)cpu_count;
  }
#elif defined(__linux__)
  long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
  if (cpu_count > 0) {
    return (size_t)cpu_count;
  }
#endif
  return 0; // Detection failed
}

static size_t get_compile_thread_count(void) {
  // Check environment variable first
  const char *env_threads = getenv("LUMA_COMPILE_THREADS");
  if (env_threads) {
    int threads = atoi(env_threads);
    if (threads > 0 && threads <= MAX_COMPILE_THREADS) {
      return (size_t)threads;
    }
  }

  // Try to detect CPU count
  size_t cpu_count = detect_cpu_count();
  if (cpu_count > 0) {
    return cpu_count;
  }

  // Fallback to default
  return DEFAULT_COMPILE_THREADS;
}

static bool create_output_directory(const char *path) {
  struct stat st = {0};
  if (stat(path, &st) == -1) {
#ifdef _WIN32
    return mkdir(path) == 0;
#else
    return mkdir(path, 0755) == 0;
#endif
  }
  return true; // Already exists
}

static LLVMTargetMachineRef create_target_machine(void) {
  char *target_triple = LLVMGetDefaultTargetTriple();
  LLVMTargetRef target;
  char *error = NULL;

  if (LLVMGetTargetFromTriple(target_triple, &target, &error)) {
    fprintf(stderr, "Failed to get target: %s\n", error);
    LLVMDisposeMessage(error);
    LLVMDisposeMessage(target_triple);
    return NULL;
  }

  char *host_cpu = LLVMGetHostCPUName();
  char *host_features = LLVMGetHostCPUFeatures();

  LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      target, target_triple, host_cpu, host_features,
      LLVMCodeGenLevelNone, // Fast compilation, no optimization
      LLVMRelocPIC, LLVMCodeModelSmall);

  LLVMDisposeMessage(host_cpu);
  LLVMDisposeMessage(host_features);
  LLVMDisposeMessage(target_triple);

  return machine;
}

static bool set_module_target(LLVMModuleRef module,
                              LLVMTargetMachineRef machine) {
  char *target_triple = LLVMGetDefaultTargetTriple();
  LLVMSetTarget(module, target_triple);

  LLVMTargetDataRef target_data = LLVMCreateTargetDataLayout(machine);
  char *data_layout = LLVMCopyStringRepOfTargetData(target_data);
  LLVMSetDataLayout(module, data_layout);

  LLVMDisposeTargetData(target_data);
  LLVMDisposeMessage(data_layout);
  LLVMDisposeMessage(target_triple);

  return true;
}

#ifdef DEBUG_BUILD
static bool verify_module(LLVMModuleRef module, const char *module_name) {
  char *error = NULL;
  if (LLVMVerifyModule(module, LLVMAbortProcessAction, &error)) {
    fprintf(stderr, "Module verification failed for %s: %s\n", module_name,
            error);
    LLVMDisposeMessage(error);
    return false;
  }
  if (error) {
    LLVMDisposeMessage(error);
  }
  return true;
}
#endif

bool generate_module_object_file(ModuleCompilationUnit *module,
                                 const char *output_path) {
  // Create target machine
  LLVMTargetMachineRef target_machine = create_target_machine();
  if (!target_machine) {
    fprintf(stderr, "Failed to create target machine for module %s\n",
            module->module_name);
    return false;
  }

  // Set target and data layout
  if (!set_module_target(module->module, target_machine)) {
    LLVMDisposeTargetMachine(target_machine);
    return false;
  }

#ifdef DEBUG_BUILD
  // Verify module only in debug builds
  if (!verify_module(module->module, module->module_name)) {
    LLVMDisposeTargetMachine(target_machine);
    return false;
  }
#endif

  // Generate object file
  char *error = NULL;
  bool success = true;

  if (LLVMTargetMachineEmitToFile(target_machine, module->module,
                                  (char *)output_path, LLVMObjectFile,
                                  &error)) {
    fprintf(stderr, "Failed to emit object file for module %s: %s\n",
            module->module_name, error);
    LLVMDisposeMessage(error);
    success = false;
  }

  LLVMDisposeTargetMachine(target_machine);
  return success;
}

static void *compile_module_worker(void *arg) {
  ModuleCompileTask *task = (ModuleCompileTask *)arg;
  clock_t start = clock();

  char output_path[MAX_PATH_LENGTH];
  snprintf(output_path, sizeof(output_path), "%s/%s.o", task->output_dir,
           task->module->module_name);

  task->success = generate_module_object_file(task->module, output_path);

  clock_t end = clock();
  task->compile_time = (double)(end - start) / CLOCKS_PER_SEC;

  return NULL;
}

bool compile_modules_to_objects(CodeGenContext *ctx, const char *output_dir) {
  // Create output directory
  if (!create_output_directory(output_dir)) {
    fprintf(stderr, "Failed to create output directory: %s\n", output_dir);
    return false;
  }

  clock_t total_start = clock();

  // Count modules
  size_t module_count = 0;
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    module_count++;
  }

  if (module_count == 0) {
    fprintf(stderr, "No modules to compile\n");
    return false;
  }

  // Determine thread count
  size_t thread_count = get_compile_thread_count();
  if (thread_count > module_count) {
    thread_count = module_count;
  }

  // Allocate resources
  ModuleCompileTask *tasks = malloc(sizeof(ModuleCompileTask) * module_count);
  pthread_t *threads = malloc(sizeof(pthread_t) * module_count);

  if (!tasks || !threads) {
    fprintf(stderr, "Failed to allocate memory for compilation tasks\n");
    free(tasks);
    free(threads);
    return false;
  }

  // Initialize tasks
  size_t i = 0;
  for (ModuleCompilationUnit *unit = ctx->modules; unit;
       unit = unit->next, i++) {
    tasks[i].module = unit;
    tasks[i].output_dir = output_dir;
    tasks[i].success = false;
    tasks[i].compile_time = 0.0;
  }

  // Compile in batches
  bool overall_success = true;

  for (size_t batch_start = 0; batch_start < module_count;
       batch_start += thread_count) {
    size_t batch_end = batch_start + thread_count;
    if (batch_end > module_count) {
      batch_end = module_count;
    }

    // Launch threads for this batch
    for (i = batch_start; i < batch_end; i++) {
      if (pthread_create(&threads[i], NULL, compile_module_worker, &tasks[i]) !=
          0) {
        fprintf(stderr, "Failed to create thread for module: %s\n",
                tasks[i].module->module_name);
        tasks[i].success = false;
        overall_success = false;
      }
    }

    // Wait for batch completion
    for (i = batch_start; i < batch_end; i++) {
      pthread_join(threads[i], NULL);

      if (!tasks[i].success) {
        fprintf(stderr, "Failed to compile module: %s\n",
                tasks[i].module->module_name);
        overall_success = false;
      }
    }
  }

  // Cleanup
  clock_t total_end = clock();
  double total_time = (double)(total_end - total_start) / CLOCKS_PER_SEC;

  free(tasks);
  free(threads);

  return overall_success;
}

ModuleCompilationUnit *create_module_unit(CodeGenContext *ctx,
                                          const char *module_name) {
  ModuleCompilationUnit *unit = (ModuleCompilationUnit *)arena_alloc(
      ctx->arena, sizeof(ModuleCompilationUnit),
      alignof(ModuleCompilationUnit));

  unit->module_name = arena_strdup(ctx->arena, module_name);
  unit->module = LLVMModuleCreateWithNameInContext(module_name, ctx->context);
  unit->symbols = NULL;
  unit->is_main_module = (strcmp(module_name, "main") == 0);
  unit->next = ctx->modules;

  ctx->modules = unit;
  return unit;
}

ModuleCompilationUnit *find_module(CodeGenContext *ctx,
                                   const char *module_name) {
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    if (strcmp(unit->module_name, module_name) == 0) {
      return unit;
    }
  }
  return NULL;
}

void set_current_module(CodeGenContext *ctx, ModuleCompilationUnit *module) {
  ctx->current_module = module;
}

void add_symbol_to_module(ModuleCompilationUnit *module, const char *name,
                          LLVMValueRef value, LLVMTypeRef type,
                          bool is_function) {
  LLVM_Symbol *sym = (LLVM_Symbol *)malloc(sizeof(LLVM_Symbol));
  sym->name = strdup(name);
  sym->value = value;
  sym->type = type;
  sym->is_function = is_function;
  sym->next = module->symbols;
  module->symbols = sym;
}

LLVM_Symbol *find_symbol_in_module(ModuleCompilationUnit *module,
                                   const char *name) {
  for (LLVM_Symbol *sym = module->symbols; sym; sym = sym->next) {
    if (strcmp(sym->name, name) == 0) {
      return sym;
    }
  }
  return NULL;
}

LLVM_Symbol *find_symbol_global(CodeGenContext *ctx, const char *name,
                                const char *module_name) {
  if (module_name) {
    ModuleCompilationUnit *module = find_module(ctx, module_name);
    if (module) {
      return find_symbol_in_module(module, name);
    }
    return NULL;
  }

  // Search current module first
  if (ctx->current_module) {
    LLVM_Symbol *sym = find_symbol_in_module(ctx->current_module, name);
    if (sym)
      return sym;
  }

  // Search all other modules
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    if (unit == ctx->current_module)
      continue;

    LLVM_Symbol *sym = find_symbol_in_module(unit, name);
    if (sym)
      return sym;
  }

  return NULL;
}

// Compatibility wrappers
void add_symbol(CodeGenContext *ctx, const char *name, LLVMValueRef value,
                LLVMTypeRef type, bool is_function) {
  if (ctx->current_module) {
    add_symbol_to_module(ctx->current_module, name, value, type, is_function);
  }
}

LLVM_Symbol *find_symbol(CodeGenContext *ctx, const char *name) {
  return find_symbol_global(ctx, name, NULL);
}

void generate_external_declarations(CodeGenContext *ctx,
                                    ModuleCompilationUnit *target_module) {
  for (ModuleCompilationUnit *source_module = ctx->modules; source_module;
       source_module = source_module->next) {
    if (source_module == target_module)
      continue;

    LLVMValueRef func = LLVMGetFirstFunction(source_module->module);
    while (func) {
      if (LLVMGetLinkage(func) == LLVMExternalLinkage) {
        const char *func_name = LLVMGetValueName(func);

        // Skip if already declared
        if (LLVMGetNamedFunction(target_module->module, func_name)) {
          func = LLVMGetNextFunction(func);
          continue;
        }

        // Create external declaration
        LLVMTypeRef func_type = LLVMGlobalGetValueType(func);
        LLVMValueRef external_func =
            LLVMAddFunction(target_module->module, func_name, func_type);
        LLVMSetLinkage(external_func, LLVMExternalLinkage);

        // Copy calling convention for struct returns
        LLVMTypeRef return_type = LLVMGetReturnType(func_type);
        if (LLVMGetTypeKind(return_type) == LLVMStructTypeKind) {
          LLVMCallConv cc = LLVMGetFunctionCallConv(func);
          LLVMSetFunctionCallConv(external_func, cc);

          // Copy parameter attributes
          unsigned param_count = LLVMCountParams(func);
          for (unsigned i = 0; i < param_count; i++) {
            LLVMValueRef src_param = LLVMGetParam(func, i);
            LLVMValueRef dst_param = LLVMGetParam(external_func, i);

            unsigned alignment = LLVMGetAlignment(src_param);
            if (alignment > 0) {
              LLVMSetAlignment(dst_param, alignment);
            }
          }
        }
      }
      func = LLVMGetNextFunction(func);
    }
  }
}

CodeGenContext *init_codegen_context(ArenaAllocator *arena) {
  CodeGenContext *ctx = (CodeGenContext *)arena_alloc(
      arena, sizeof(CodeGenContext), alignof(CodeGenContext));

  // Initialize LLVM targets
  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmParsers();
  LLVMInitializeAllAsmPrinters();

  ctx->context = LLVMContextCreate();
  ctx->builder = LLVMCreateBuilderInContext(ctx->context);

  // Initialize type cache
  init_type_cache(ctx);

  // Initialize module and symbol state
  ctx->modules = NULL;
  ctx->current_module = NULL;
  ctx->current_function = NULL;
  ctx->loop_continue_block = NULL;
  ctx->loop_break_block = NULL;
  ctx->struct_types = NULL;
  ctx->arena = arena;
  ctx->module = NULL;
  ctx->deferred_statements = NULL;
  ctx->deferred_count = 0;

  // Initialize caches
  init_symbol_cache();
  init_struct_cache();

  return ctx;
}

void cleanup_codegen_context(CodeGenContext *ctx) {
  if (!ctx)
    return;

  // Cleanup modules and symbols
  ModuleCompilationUnit *unit = ctx->modules;
  while (unit) {
    ModuleCompilationUnit *next = unit->next;

    // Free symbols
    LLVM_Symbol *sym = unit->symbols;
    while (sym) {
      LLVM_Symbol *next_sym = sym->next;
      free(sym->name);
      free(sym);
      sym = next_sym;
    }

    LLVMDisposeModule(unit->module);
    unit = next;
  }

  // Cleanup LLVM resources
  LLVMDisposeBuilder(ctx->builder);
  LLVMContextDispose(ctx->context);
  LLVMShutdown();
}

bool generate_program_modules(CodeGenContext *ctx, AstNode *ast_root,
                              const char *output_dir) {
  if (!ast_root || ast_root->type != AST_PROGRAM) {
    return false;
  }

  // Generate code for all modules
  codegen_stmt_program_multi_module(ctx, ast_root);

  // Compile all modules to separate object files
  return compile_modules_to_objects(ctx, output_dir);
}

char *print_llvm_ir(CodeGenContext *ctx) {
  if (ctx->current_module) {
    return LLVMPrintModuleToString(ctx->current_module->module);
  }
  return NULL;
}

bool generate_object_file(CodeGenContext *ctx, const char *object_filename) {
  if (ctx->current_module) {
    return generate_module_object_file(ctx->current_module, object_filename);
  }
  return false;
}

bool generate_assembly_file(CodeGenContext *ctx, const char *asm_filename) {
  if (!ctx->current_module)
    return false;

  LLVMTargetMachineRef target_machine = create_target_machine();
  if (!target_machine) {
    fprintf(stderr, "Failed to create target machine\n");
    return false;
  }

  set_module_target(ctx->current_module->module, target_machine);

  char *error = NULL;
  bool success = true;

  if (LLVMTargetMachineEmitToFile(target_machine, ctx->current_module->module,
                                  (char *)asm_filename, LLVMAssemblyFile,
                                  &error)) {
    fprintf(stderr, "Failed to emit assembly file: %s\n", error);
    LLVMDisposeMessage(error);
    success = false;
  }

  LLVMDisposeTargetMachine(target_machine);
  return success;
}

LLVMLinkage get_function_linkage(AstNode *node) {
  const char *name = node->stmt.func_decl.name;

  // Main function must always be external
  if (strcmp(name, "main") == 0) {
    return LLVMExternalLinkage;
  }

  // Use the is_public flag for other functions
  return node->stmt.func_decl.is_public ? LLVMExternalLinkage
                                        : LLVMInternalLinkage;
}

char *process_escape_sequences(const char *input) {
  size_t len = strlen(input);
  char *output = malloc(len + 1);
  size_t out_idx = 0;

  for (size_t i = 0; i < len; i++) {
    if (input[i] == '\\' && i + 1 < len) {
      switch (input[i + 1]) {
      case 'n':
        output[out_idx++] = '\n';
        i++;
        break;
      case 'r':
        output[out_idx++] = '\r';
        i++;
        break;
      case 't':
        output[out_idx++] = '\t';
        i++;
        break;
      case '\\':
        output[out_idx++] = '\\';
        i++;
        break;
      case '"':
        output[out_idx++] = '"';
        i++;
        break;
      case '0':
        output[out_idx++] = '\0';
        i++;
        break;
      case 'x':
        if (i + 3 < len) {
          char hex_str[3] = {input[i + 2], input[i + 3], '\0'};
          char *endptr;
          long hex_val = strtol(hex_str, &endptr, 16);
          if (endptr == hex_str + 2) {
            output[out_idx++] = (char)hex_val;
            i += 3;
          } else {
            output[out_idx++] = input[i];
          }
        } else {
          output[out_idx++] = input[i];
        }
        break;
      default:
        output[out_idx++] = input[i];
        break;
      }
    } else {
      output[out_idx++] = input[i];
    }
  }

  output[out_idx] = '\0';
  return output;
}
