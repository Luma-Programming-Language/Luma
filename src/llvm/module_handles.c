#include "llvm.h"
#include <stdlib.h>

// =============================================================================
// OPTIMIZATION LAYER - HASH TABLE CACHING
// =============================================================================

static FieldToStructEntry *field_to_struct_buckets[SYMBOL_HASH_SIZE] = {0};
static SymbolHashTable *global_symbol_cache = NULL;
static StructHashTable *global_struct_cache = NULL;

// Simple hash function
unsigned int hash_string(const char *str) {
  unsigned int hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash % SYMBOL_HASH_SIZE;
}

// Symbol cache operations
void init_symbol_cache(void) {
  if (!global_symbol_cache) {
    global_symbol_cache = (SymbolHashTable *)calloc(1, sizeof(SymbolHashTable));
  }
}

void cache_symbol(const char *module_name, const char *symbol_name,
                  LLVM_Symbol *symbol) {
  init_symbol_cache();

  char key[512];
  snprintf(key, sizeof(key), "%s:%s", module_name, symbol_name);

  unsigned int bucket = hash_string(key);

  // Check if already cached
  for (SymbolHashEntry *entry = global_symbol_cache->buckets[bucket]; entry;
       entry = entry->next) {
    if (strcmp(entry->key, key) == 0) {
      entry->symbol = symbol;
      return;
    }
  }

  // Add new entry
  SymbolHashEntry *new_entry =
      (SymbolHashEntry *)malloc(sizeof(SymbolHashEntry));
  new_entry->key = strdup(key);
  new_entry->symbol = symbol;
  new_entry->next = global_symbol_cache->buckets[bucket];
  global_symbol_cache->buckets[bucket] = new_entry;
}

LLVM_Symbol *lookup_cached_symbol(const char *module_name,
                                  const char *symbol_name) {
  if (!global_symbol_cache)
    return NULL;

  char key[512];
  snprintf(key, sizeof(key), "%s:%s", module_name, symbol_name);

  unsigned int bucket = hash_string(key);

  for (SymbolHashEntry *entry = global_symbol_cache->buckets[bucket]; entry;
       entry = entry->next) {
    if (strcmp(entry->key, key) == 0) {
      return entry->symbol;
    }
  }

  return NULL;
}

// Struct cache operations
void init_struct_cache(void) {
  if (!global_struct_cache) {
    global_struct_cache = (StructHashTable *)calloc(1, sizeof(StructHashTable));
  }
}

void cache_struct(const char *name, StructInfo *info) {
  init_struct_cache();

  unsigned int bucket = hash_string(name);

  for (StructHashEntry *entry = global_struct_cache->buckets[bucket]; entry;
       entry = entry->next) {
    if (strcmp(entry->name, name) == 0) {
      entry->info = info;
      return;
    }
  }

  StructHashEntry *new_entry =
      (StructHashEntry *)malloc(sizeof(StructHashEntry));
  new_entry->name = strdup(name);
  new_entry->info = info;
  new_entry->next = global_struct_cache->buckets[bucket];
  global_struct_cache->buckets[bucket] = new_entry;
}

StructInfo *lookup_cached_struct(const char *name) {
  if (!global_struct_cache)
    return NULL;

  unsigned int bucket = hash_string(name);

  for (StructHashEntry *entry = global_struct_cache->buckets[bucket]; entry;
       entry = entry->next) {
    if (strcmp(entry->name, name) == 0) {
      return entry->info;
    }
  }

  return NULL;
}

StructInfo *find_struct_type_fast(CodeGenContext *ctx, const char *name) {
  (void)ctx; // Not needed with cache

  // Direct cache lookup - O(1) average
  StructInfo *cached = lookup_cached_struct(name);
  if (cached) {
    return cached;
  }

  return NULL;
}

void cache_struct_field(const char *field_name, StructInfo *info) {
  unsigned int bucket = hash_string(field_name);

  // Check if already cached
  for (FieldToStructEntry *entry = field_to_struct_buckets[bucket]; entry;
       entry = entry->next) {
    if (strcmp(entry->field_name, field_name) == 0) {
      return; // Already cached
    }
  }

  FieldToStructEntry *new_entry = malloc(sizeof(FieldToStructEntry));
  new_entry->field_name = strdup(field_name);
  new_entry->struct_info = info;
  new_entry->next = field_to_struct_buckets[bucket];
  field_to_struct_buckets[bucket] = new_entry;
}

StructInfo *find_struct_by_field_cached(CodeGenContext *ctx,
                                        const char *field_name) {
  unsigned int bucket = hash_string(field_name);

  // Try cache first
  for (FieldToStructEntry *entry = field_to_struct_buckets[bucket]; entry;
       entry = entry->next) {
    if (strcmp(entry->field_name, field_name) == 0) {
      return entry->struct_info;
    }
  }

  // Fallback: linear search and cache result
  for (StructInfo *info = ctx->struct_types; info; info = info->next) {
    int field_idx = get_field_index(info, field_name);
    if (field_idx >= 0) {
      cache_struct_field(field_name, info);
      return info;
    }
  }

  return NULL;
}

// =============================================================================
// PUBLIC API - BATCH PREPROCESSING
// =============================================================================

void preprocess_all_modules(CodeGenContext *ctx) {
  // Pre-cache all symbols from all modules
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    for (LLVM_Symbol *sym = unit->symbols; sym; sym = sym->next) {
      cache_symbol(unit->module_name, sym->name, sym);
    }
  }

  // Pre-cache all struct types
  for (StructInfo *info = ctx->struct_types; info; info = info->next) {
    cache_struct(info->name, info);

    for (size_t i = 0; i < info->field_count; i++) {
      cache_struct_field(info->field_names[i], info);
    }
  }
}

void cleanup_module_caches(void) {
  // Clean symbol cache
  if (global_symbol_cache) {
    for (int i = 0; i < SYMBOL_HASH_SIZE; i++) {
      SymbolHashEntry *entry = global_symbol_cache->buckets[i];
      while (entry) {
        SymbolHashEntry *next = entry->next;
        free((void *)entry->key);
        free(entry);
        entry = next;
      }
    }
    free(global_symbol_cache);
    global_symbol_cache = NULL;
  }

  // Clean struct cache
  if (global_struct_cache) {
    for (int i = 0; i < SYMBOL_HASH_SIZE; i++) {
      StructHashEntry *entry = global_struct_cache->buckets[i];
      while (entry) {
        StructHashEntry *next = entry->next;
        free((void *)entry->name);
        free(entry);
        entry = next;
      }
    }
    free(global_struct_cache);
    global_struct_cache = NULL;
  }

  for (int i = 0; i < SYMBOL_HASH_SIZE; i++) {
    FieldToStructEntry *entry = field_to_struct_buckets[i];
    while (entry) {
      FieldToStructEntry *next = entry->next;
      free((void *)entry->field_name);
      free(entry);
      entry = next;
    }
    field_to_struct_buckets[i] = NULL;
  }
}

// =============================================================================
// ORIGINAL CODE - DEPENDENCY MANAGEMENT
// =============================================================================

ModuleDependencyInfo *build_codegen_dependency_info(AstNode **modules,
                                                    size_t module_count,
                                                    ArenaAllocator *arena) {

  ModuleDependencyInfo *dep_info = (ModuleDependencyInfo *)arena_alloc(
      arena, sizeof(ModuleDependencyInfo) * module_count,
      alignof(ModuleDependencyInfo));

  for (size_t i = 0; i < module_count; i++) {
    AstNode *module = modules[i];
    if (!module || module->type != AST_PREPROCESSOR_MODULE)
      continue;

    dep_info[i].module_name = module->preprocessor.module.name;
    dep_info[i].processed = false;
    dep_info[i].dep_count = 0;

    AstNode **body = module->preprocessor.module.body;
    int body_count = module->preprocessor.module.body_count;

    size_t use_count = 0;
    for (int j = 0; j < body_count; j++) {
      if (body[j] && body[j]->type == AST_PREPROCESSOR_USE) {
        use_count++;
      }
    }

    dep_info[i].dependencies = (char **)arena_alloc(
        arena, sizeof(char *) * use_count, alignof(char *));

    size_t dep_idx = 0;
    for (int j = 0; j < body_count; j++) {
      if (body[j] && body[j]->type == AST_PREPROCESSOR_USE) {
        dep_info[i].dependencies[dep_idx++] =
            (char *)body[j]->preprocessor.use.module_name;
      }
    }
    dep_info[i].dep_count = use_count;
  }

  return dep_info;
}

static bool process_module_codegen_recursive(CodeGenContext *ctx,
                                             const char *module_name,
                                             AstNode **modules,
                                             size_t module_count,
                                             ModuleDependencyInfo *dep_info) {

  ModuleDependencyInfo *current_dep = NULL;
  size_t current_idx = 0;
  for (size_t i = 0; i < module_count; i++) {
    if (strcmp(dep_info[i].module_name, module_name) == 0) {
      current_dep = &dep_info[i];
      current_idx = i;
      break;
    }
  }

  if (!current_dep) {
    fprintf(stderr, "Error: Module '%s' not found in dependency info\n",
            module_name);
    return false;
  }

  if (current_dep->processed) {
    return true;
  }

  for (size_t i = 0; i < current_dep->dep_count; i++) {
    if (!process_module_codegen_recursive(ctx, current_dep->dependencies[i],
                                          modules, module_count, dep_info)) {
      return false;
    }
  }

  AstNode *module = modules[current_idx];
  ModuleCompilationUnit *unit = find_module(ctx, module_name);
  if (!unit) {
    fprintf(stderr, "Error: Module unit not found for '%s'\n", module_name);
    return false;
  }

  set_current_module(ctx, unit);
  ctx->module = unit->module;

  AstNode **body = module->preprocessor.module.body;
  int body_count = module->preprocessor.module.body_count;

  for (int j = 0; j < body_count; j++) {
    if (!body[j])
      continue;
    if (body[j]->type != AST_PREPROCESSOR_USE) {
      codegen_stmt(ctx, body[j]);
    }
  }

  current_dep->processed = true;
  return true;
}

// =============================================================================
// MULTI-MODULE PROGRAM HANDLER
// =============================================================================

LLVMValueRef codegen_stmt_program_multi_module(CodeGenContext *ctx,
                                               AstNode *node) {
  if (!node || node->type != AST_PROGRAM) {
    return NULL;
  }

  // PASS 1: Create all module units
  for (size_t i = 0; i < node->stmt.program.module_count; i++) {
    AstNode *module_node = node->stmt.program.modules[i];

    if (module_node->type == AST_PREPROCESSOR_MODULE) {
      const char *module_name = module_node->preprocessor.module.name;

      ModuleCompilationUnit *existing = find_module(ctx, module_name);
      if (existing) {
        fprintf(stderr, "Error: Duplicate module definition: %s\n",
                module_name);
        return NULL;
      }

      ModuleCompilationUnit *unit = create_module_unit(ctx, module_name);
      set_current_module(ctx, unit);
      ctx->module = unit->module;
    }
  }

  // PASS 2: Process all @use directives
  for (size_t i = 0; i < node->stmt.program.module_count; i++) {
    AstNode *module_node = node->stmt.program.modules[i];

    if (module_node->type == AST_PREPROCESSOR_MODULE) {
      const char *module_name = module_node->preprocessor.module.name;
      ModuleCompilationUnit *unit = find_module(ctx, module_name);

      if (!unit) {
        fprintf(stderr, "Error: Module unit not found: %s\n", module_name);
        return NULL;
      }

      set_current_module(ctx, unit);
      ctx->module = unit->module;

      AstNode **body = module_node->preprocessor.module.body;
      int body_count = module_node->preprocessor.module.body_count;

      for (int j = 0; j < body_count; j++) {
        if (body[j] && body[j]->type == AST_PREPROCESSOR_USE) {
          codegen_stmt_use(ctx, body[j]);
        }
      }
    }
  }

  preprocess_all_modules(ctx);

  // PASS 3: Generate code in dependency order
  ModuleDependencyInfo *dep_info = build_codegen_dependency_info(
      node->stmt.program.modules, node->stmt.program.module_count, ctx->arena);

  for (size_t i = 0; i < node->stmt.program.module_count; i++) {
    AstNode *module_node = node->stmt.program.modules[i];
    if (module_node->type == AST_PREPROCESSOR_MODULE) {
      const char *module_name = module_node->preprocessor.module.name;

      if (!process_module_codegen_recursive(
              ctx, module_name, node->stmt.program.modules,
              node->stmt.program.module_count, dep_info)) {
        return NULL;
      }
    }
  }

  return NULL;
}

LLVMValueRef codegen_stmt_module(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_PREPROCESSOR_MODULE) {
    return NULL;
  }

  for (size_t i = 0; i < node->preprocessor.module.body_count; i++) {
    AstNode *stmt = node->preprocessor.module.body[i];

    if (stmt->type == AST_PREPROCESSOR_USE) {
      codegen_stmt_use(ctx, stmt);
    } else {
      codegen_stmt(ctx, stmt);
    }
  }

  return NULL;
}

LLVMValueRef codegen_stmt_use(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_PREPROCESSOR_USE) {
    return NULL;
  }

  const char *module_name = node->preprocessor.use.module_name;
  const char *alias = node->preprocessor.use.alias;

  ModuleCompilationUnit *referenced_module = find_module(ctx, module_name);
  if (!referenced_module) {
    fprintf(stderr, "Error: Cannot import module '%s' - module not found\n",
            module_name);
    fprintf(stderr,
            "Note: Make sure the module is defined before it's imported\n");
    return NULL;
  }

  if (ctx->current_module &&
      strcmp(ctx->current_module->module_name, module_name) == 0) {
    fprintf(stderr, "Warning: Module '%s' trying to import itself - skipping\n",
            module_name);
    return NULL;
  }

  import_module_symbols(ctx, referenced_module, alias);

  return NULL;
}

// =============================================================================
// SYMBOL IMPORT FUNCTIONALITY
// =============================================================================

void import_module_symbols(CodeGenContext *ctx,
                           ModuleCompilationUnit *source_module,
                           const char *alias) {
  if (!ctx->current_module || !source_module) {
    return;
  }

  for (LLVM_Symbol *sym = source_module->symbols; sym; sym = sym->next) {
    if (sym->is_function) {
      LLVMValueRef func_value =
          LLVMGetNamedFunction(source_module->module, sym->name);
      if (func_value && LLVMGetLinkage(func_value) == LLVMExternalLinkage) {
        import_function_symbol(ctx, sym, source_module, alias);
      }
    } else {
      LLVMValueRef global_value =
          LLVMGetNamedGlobal(source_module->module, sym->name);
      if (global_value && LLVMGetLinkage(global_value) == LLVMExternalLinkage) {
        import_variable_symbol(ctx, sym, source_module, alias);
      }
    }
  }
}

void import_function_symbol(CodeGenContext *ctx, LLVM_Symbol *source_symbol,
                            ModuleCompilationUnit *source_module,
                            const char *alias) {
  (void)source_module;

  char imported_name[256];
  if (alias) {
    snprintf(imported_name, sizeof(imported_name), "%s.%s", alias,
             source_symbol->name);
  } else {
    snprintf(imported_name, sizeof(imported_name), "%s", source_symbol->name);
  }

  if (LLVMGetNamedFunction(ctx->current_module->module, imported_name)) {
    return;
  }

  LLVMTypeRef func_type = LLVMGlobalGetValueType(source_symbol->value);
  LLVMValueRef external_func = LLVMAddFunction(ctx->current_module->module,
                                               source_symbol->name, func_type);
  LLVMSetLinkage(external_func, LLVMExternalLinkage);

  LLVMTypeRef return_type = LLVMGetReturnType(func_type);
  if (LLVMGetTypeKind(return_type) == LLVMStructTypeKind) {
    LLVMCallConv source_cc = LLVMGetFunctionCallConv(source_symbol->value);
    LLVMSetFunctionCallConv(external_func, source_cc);

    unsigned param_count = LLVMCountParams(source_symbol->value);
    for (unsigned i = 0; i < param_count; i++) {
      LLVMValueRef src_param = LLVMGetParam(source_symbol->value, i);
      LLVMValueRef dst_param = LLVMGetParam(external_func, i);

      if (LLVMGetAlignment(src_param) > 0) {
        LLVMSetAlignment(dst_param, LLVMGetAlignment(src_param));
      }
    }
  }

  add_symbol_to_module(ctx->current_module, imported_name, external_func,
                       func_type, true);
}

void import_variable_symbol(CodeGenContext *ctx, LLVM_Symbol *source_symbol,
                            ModuleCompilationUnit *source_module,
                            const char *alias) {
  (void)source_module;

  char imported_name[256];
  if (alias) {
    snprintf(imported_name, sizeof(imported_name), "%s.%s", alias,
             source_symbol->name);
  } else {
    snprintf(imported_name, sizeof(imported_name), "%s", source_symbol->name);
  }

  if (LLVMGetNamedGlobal(ctx->current_module->module, imported_name)) {
    return;
  }

  LLVMValueRef external_global = LLVMAddGlobal(
      ctx->current_module->module, source_symbol->type, source_symbol->name);
  LLVMSetLinkage(external_global, LLVMExternalLinkage);

  add_symbol_to_module(ctx->current_module, imported_name, external_global,
                       source_symbol->type, false);
}

// =============================================================================
// MEMBER ACCESS - KEPT ORIGINAL VERSION
// =============================================================================

LLVMValueRef codegen_expr_member_access_enhanced(CodeGenContext *ctx,
                                                 AstNode *node) {
  if (!node || node->type != AST_EXPR_MEMBER) {
    return NULL;
  }

  AstNode *object = node->expr.member.object;
  const char *member = node->expr.member.member;
  bool is_compiletime = node->expr.member.is_compiletime;

  if (object->type != AST_EXPR_IDENTIFIER &&
      !(object->type == AST_EXPR_MEMBER && is_compiletime)) {
    return codegen_expr_struct_access(ctx, node);
  }

  if (is_compiletime) {
    if (object->type == AST_EXPR_MEMBER && object->expr.member.is_compiletime) {
      if (object->expr.member.object->type != AST_EXPR_IDENTIFIER) {
        fprintf(stderr,
                "Error: Expected identifier in chained compile-time access\n");
        return NULL;
      }

      const char *module_name =
          object->expr.member.object->expr.identifier.name;
      const char *type_name = object->expr.member.member;

      char type_qualified_name[256];
      snprintf(type_qualified_name, sizeof(type_qualified_name), "%s.%s",
               type_name, member);

      ModuleCompilationUnit *source_module = find_module(ctx, module_name);
      if (source_module) {
        LLVM_Symbol *enum_member =
            find_symbol_in_module(source_module, type_qualified_name);
        if (enum_member && is_enum_constant(enum_member)) {
          return LLVMGetInitializer(enum_member->value);
        }
      }

      LLVM_Symbol *enum_member =
          find_symbol_in_module(ctx->current_module, type_qualified_name);
      if (enum_member && is_enum_constant(enum_member)) {
        return LLVMGetInitializer(enum_member->value);
      }

      for (ModuleCompilationUnit *unit = ctx->modules; unit;
           unit = unit->next) {
        if (unit == ctx->current_module)
          continue;

        LLVM_Symbol *sym = find_symbol_in_module(unit, type_qualified_name);
        if (sym && is_enum_constant(sym)) {
          return LLVMGetInitializer(sym->value);
        }
      }

      fprintf(stderr, "Error: Enum member '%s::%s::%s' not found\n",
              module_name, type_name, member);
      return NULL;
    }

    const char *object_name = NULL;
    if (object->type == AST_EXPR_IDENTIFIER) {
      object_name = object->expr.identifier.name;
    } else {
      fprintf(stderr, "Error: Expected identifier for compile-time access\n");
      return NULL;
    }

    char qualified_name[256];
    snprintf(qualified_name, sizeof(qualified_name), "%s.%s", object_name,
             member);

    LLVM_Symbol *qualified_sym =
        find_symbol_in_module(ctx->current_module, qualified_name);
    if (qualified_sym) {
      if (qualified_sym->is_function) {
        return qualified_sym->value;
      } else if (is_enum_constant(qualified_sym)) {
        return LLVMGetInitializer(qualified_sym->value);
      } else {
        return LLVMBuildLoad2(ctx->builder, qualified_sym->type,
                              qualified_sym->value, "load");
      }
    }

    LLVM_Symbol *imported_sym =
        find_symbol_in_module(ctx->current_module, qualified_name);
    if (imported_sym) {
      if (imported_sym->is_function) {
        return imported_sym->value;
      } else if (is_enum_constant(imported_sym)) {
        return LLVMGetInitializer(imported_sym->value);
      } else {
        return LLVMBuildLoad2(ctx->builder, imported_sym->type,
                              imported_sym->value, "load");
      }
    }

    LLVMModuleRef current_llvm_module =
        ctx->current_module ? ctx->current_module->module : ctx->module;

    for (ModuleCompilationUnit *search_module = ctx->modules; search_module;
         search_module = search_module->next) {

      if (search_module == ctx->current_module) {
        continue;
      }

      LLVMValueRef source_func =
          LLVMGetNamedFunction(search_module->module, member);

      if (source_func) {
        LLVMValueRef existing =
            LLVMGetNamedFunction(current_llvm_module, member);

        if (!existing) {
          LLVMTypeRef func_type = LLVMGlobalGetValueType(source_func);
          existing = LLVMAddFunction(current_llvm_module, member, func_type);
          LLVMSetLinkage(existing, LLVMExternalLinkage);

          LLVMCallConv cc = LLVMGetFunctionCallConv(source_func);
          LLVMSetFunctionCallConv(existing, cc);

          add_symbol_to_module(ctx->current_module, member, existing, func_type,
                               true);
          add_symbol_to_module(ctx->current_module, qualified_name, existing,
                               func_type, true);
        }

        return existing;
      }

      LLVM_Symbol *source_sym = find_symbol_in_module(search_module, member);
      if (source_sym) {
        if (source_sym->is_function) {
          LLVMValueRef existing =
              LLVMGetNamedFunction(current_llvm_module, member);

          if (!existing) {
            LLVMTypeRef func_type = LLVMGlobalGetValueType(source_sym->value);
            existing = LLVMAddFunction(current_llvm_module, member, func_type);
            LLVMSetLinkage(existing, LLVMExternalLinkage);

            LLVMCallConv cc = LLVMGetFunctionCallConv(source_sym->value);
            LLVMSetFunctionCallConv(existing, cc);

            add_symbol_to_module(ctx->current_module, member, existing,
                                 func_type, true);
            add_symbol_to_module(ctx->current_module, qualified_name, existing,
                                 func_type, true);
          }

          return existing;
        } else if (is_enum_constant(source_sym)) {
          return LLVMGetInitializer(source_sym->value);
        } else {
          import_variable_symbol(ctx, source_sym, search_module, object_name);

          qualified_sym =
              find_symbol_in_module(ctx->current_module, qualified_name);
          if (qualified_sym) {
            return LLVMBuildLoad2(ctx->builder, qualified_sym->type,
                                  qualified_sym->value, "load");
          }
        }
      }
    }

    fprintf(stderr, "Error: No compile-time symbol '%s::%s' found\n",
            object_name, member);
    return NULL;
  }

  const char *object_name = object->expr.identifier.name;

  LLVM_Symbol *local_sym = find_symbol(ctx, object_name);
  if (local_sym && !local_sym->is_function) {
    LLVMTypeRef sym_type = local_sym->type;
    LLVMTypeKind sym_kind = LLVMGetTypeKind(sym_type);

    bool is_struct_access = false;

    if (sym_kind == LLVMStructTypeKind) {
      is_struct_access = true;
    } else if (sym_kind == LLVMPointerTypeKind && local_sym->element_type) {
      if (LLVMGetTypeKind(local_sym->element_type) == LLVMStructTypeKind) {
        is_struct_access = true;
      }
    }

    if (is_struct_access) {
      return codegen_expr_struct_access(ctx, node);
    }
  }

  bool is_module_access = false;
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    if (strcmp(unit->module_name, object_name) == 0) {
      is_module_access = true;
      break;
    }
  }

  if (is_module_access) {
    fprintf(stderr,
            "Error: Cannot use runtime access '.' for module function.\n"
            "  Did you mean '%s::%s'?\n",
            object_name, member);
    return NULL;
  }

  LLVM_Symbol *obj_sym = find_symbol(ctx, object_name);
  if (!obj_sym) {
    fprintf(stderr,
            "Error: Undefined identifier '%s' in member access '%s.%s'\n",
            object_name, object_name, member);
    return NULL;
  }

  if (obj_sym->is_function) {
    fprintf(stderr, "Error: Cannot use member access on function '%s'\n",
            object_name);
    return NULL;
  }

  return codegen_expr_struct_access(ctx, node);
}

LLVMValueRef codegen_expr_member_access(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_MEMBER) {
    return NULL;
  }

  AstNode *object = node->expr.member.object;
  const char *member = node->expr.member.member;

  if (object->type != AST_EXPR_IDENTIFIER) {
    fprintf(stderr, "Error: Invalid member access syntax\n");
    return NULL;
  }

  const char *object_name = object->expr.identifier.name;

  char qualified_name[256];
  snprintf(qualified_name, sizeof(qualified_name), "%s.%s", object_name,
           member);

  LLVM_Symbol *sym = find_symbol_in_module(ctx->current_module, qualified_name);
  if (sym) {
    if (sym->is_function) {
      return sym->value;
    } else if (is_enum_constant(sym)) {
      return LLVMGetInitializer(sym->value);
    } else {
      return LLVMBuildLoad2(ctx->builder, sym->type, sym->value, "load");
    }
  }

  LLVM_Symbol *enum_type_sym = find_symbol(ctx, object_name);
  if (enum_type_sym && enum_type_sym->value == NULL) {
    fprintf(stderr, "Error: Enum member '%s' not found in enum '%s'\n", member,
            object_name);
  } else {
    fprintf(stderr, "Error: Symbol '%s.%s' not found\n", object_name, member);
  }

  return NULL;
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

LLVM_Symbol *find_symbol_with_module_support(CodeGenContext *ctx,
                                             const char *name) {
  if (ctx->current_module) {
    LLVM_Symbol *sym = find_symbol_in_module(ctx->current_module, name);
    if (sym) {
      return sym;
    }
  }

  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    if (unit == ctx->current_module) {
      continue;
    }

    LLVM_Symbol *sym = find_symbol_in_module(unit, name);
    if (sym && sym->is_function) {
      LLVMValueRef func = LLVMGetNamedFunction(unit->module, name);
      if (func && LLVMGetLinkage(func) == LLVMExternalLinkage) {
        return sym;
      }
    }
  }

  return NULL;
}

bool is_main_module(ModuleCompilationUnit *unit) {
  return unit && unit->is_main_module;
}

void set_module_as_main(ModuleCompilationUnit *unit) {
  if (unit) {
    unit->is_main_module = true;
  }
}

void print_module_info(CodeGenContext *ctx) {
  printf("\n=== MODULE INFORMATION ===\n");
  for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
    printf("Module: %s %s\n", unit->module_name,
           unit->is_main_module ? "(main)" : "");

    printf("  Symbols:\n");
    for (LLVM_Symbol *sym = unit->symbols; sym; sym = sym->next) {
      printf("    %s %s\n", sym->name,
             sym->is_function ? "(function)" : "(variable)");
    }
  }
  printf("========================\n\n");
}

void debug_object_files(const char *output_dir) {
  printf("\n=== OBJECT FILE DEBUG INFO ===\n");
  char command[512];
  snprintf(command, sizeof(command), "ls -la %s/*.o", output_dir);
  printf("Object files in %s:\n", output_dir);
  system(command);

  snprintf(command, sizeof(command), "file %s/*.o", output_dir);
  printf("\nFile types:\n");
  system(command);

  snprintf(command, sizeof(command), "nm %s/*.o | head -20", output_dir);
  printf("\nSymbols (first 20):\n");
  system(command);

  printf("==============================\n\n");
}
