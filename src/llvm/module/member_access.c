#include "../llvm.h"

// ============================================================================
// MODULE ACCESS - Handles compile-time access (::) to module symbols
// ============================================================================
// This file handles:
//   - Alias::function (where alias is from @use "module" as alias)
//   - Module::Type::EnumMember
//   - Works with the existing import system that creates alias.symbol entries
// ============================================================================

// Forward declarations
static LLVMValueRef handle_symbol_value(CodeGenContext *ctx, LLVM_Symbol *sym);

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Handle compile-time member access (::)
 *
 * Works with the existing import system:
 * - @use "std_io" as io creates symbols like "io.println"
 * - Then io::println looks up "io.println"
 *
 * @param ctx Code generation context
 * @param node AST node for member access expression
 * @return Generated LLVM value or NULL on error
 */
LLVMValueRef codegen_module_access(CodeGenContext *ctx, AstNode *node) {
  AstNode *object = node->expr.member.object;
  const char *member = node->expr.member.member;

  // Handle chained compile-time access (Module::Type::member)
  if (object->type == AST_EXPR_MEMBER && object->expr.member.is_compiletime) {
    if (object->expr.member.object->type != AST_EXPR_IDENTIFIER) {
      fprintf(stderr,
              "Error: Expected identifier in chained compile-time access\n");
      return NULL;
    }

    const char *module_name = object->expr.member.object->expr.identifier.name;
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

    for (ModuleCompilationUnit *unit = ctx->modules; unit; unit = unit->next) {
      if (unit == ctx->current_module || unit == source_module)
        continue;

      LLVM_Symbol *sym = find_symbol_in_module(unit, type_qualified_name);
      if (sym && is_enum_constant(sym)) {
        return LLVMGetInitializer(sym->value);
      }
    }

    fprintf(stderr, "Error: Enum member '%s::%s::%s' not found\n", module_name,
            type_name, member);
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

  // Not found in current module, search other modules
  LLVMModuleRef current_llvm_module =
      ctx->current_module ? ctx->current_module->module : ctx->module;

  for (ModuleCompilationUnit *search_module = ctx->modules; search_module;
       search_module = search_module->next) {

    if (search_module == ctx->current_module) {
      continue;
    }

    // Try LLVM module first (faster for functions)
    LLVMValueRef source_func =
        LLVMGetNamedFunction(search_module->module, member);

    if (source_func) {
      LLVMValueRef existing = LLVMGetNamedFunction(current_llvm_module, member);

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

    // Check symbol table
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

          add_symbol_to_module(ctx->current_module, member, existing, func_type,
                               true);
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

  fprintf(stderr, "Error: No compile-time symbol '%s::%s' found\n", object_name,
          member);
  return NULL;
}

/**
 * @brief Check if an identifier might be a module/alias
 */
bool is_module_identifier(CodeGenContext *ctx, const char *name) {
  // Check if there are any symbols with this prefix
  char prefix[258];
  snprintf(prefix, sizeof(prefix), "%s.", name);

  // Check current module
  for (LLVM_Symbol *sym = ctx->current_module->symbols; sym; sym = sym->next) {
    if (strncmp(sym->name, prefix, strlen(prefix)) == 0) {
      return true;
    }
  }

  // Also check if it's an actual module name
  return find_module(ctx, name) != NULL;
}

/**
 * @brief Handle different symbol types appropriately
 */
static LLVMValueRef handle_symbol_value(CodeGenContext *ctx, LLVM_Symbol *sym) {
  if (sym->is_function) {
    return sym->value;
  }

  if (is_enum_constant(sym)) {
    return LLVMGetInitializer(sym->value);
  }

  // Check if it's a global constant variable
  if (LLVMIsAGlobalVariable(sym->value)) {
    if (LLVMIsGlobalConstant(sym->value)) {
      // For global constants, return the initializer value
      LLVMValueRef init = LLVMGetInitializer(sym->value);
      if (init) {
        return init;
      }
    }
    // For non-const globals, load the value
    return LLVMBuildLoad2(ctx->builder, sym->type, sym->value, "load_global");
  }

  // Regular variable - load it
  return LLVMBuildLoad2(ctx->builder, sym->type, sym->value, "load");
}

/**
 * @brief Get module name/alias from a compile-time member access node
 */
const char *get_module_name_from_access(AstNode *node) {
  if (!node || node->type != AST_EXPR_MEMBER) {
    return NULL;
  }

  if (!node->expr.member.is_compiletime) {
    return NULL;
  }

  if (node->expr.member.object->type == AST_EXPR_IDENTIFIER) {
    return node->expr.member.object->expr.identifier.name;
  }

  // Chained access
  if (node->expr.member.object->type == AST_EXPR_MEMBER) {
    return get_module_name_from_access(node->expr.member.object);
  }

  return NULL;
}

/**
 * @brief Validate that a module access is permitted
 *
 * Checks that the qualified symbol exists
 */
bool validate_module_access(CodeGenContext *ctx, const char *prefix,
                            const char *symbol_name) {
  char qualified_name[256];
  snprintf(qualified_name, sizeof(qualified_name), "%s.%s", prefix,
           symbol_name);

  // Check if symbol exists in current module
  LLVM_Symbol *sym = find_symbol_in_module(ctx->current_module, qualified_name);
  if (sym) {
    return true;
  }

  fprintf(stderr, "Error: Symbol '%s' not found\n", qualified_name);
  fprintf(stderr, "  Available symbols with prefix '%s':\n", prefix);

  // List available symbols with this prefix for debugging
  char prefix_dot[258];
  snprintf(prefix_dot, sizeof(prefix_dot), "%s.", prefix);

  int count = 0;
  for (LLVM_Symbol *s = ctx->current_module->symbols; s; s = s->next) {
    if (strncmp(s->name, prefix_dot, strlen(prefix_dot)) == 0) {
      fprintf(stderr, "    - %s\n", s->name);
      count++;
    }
  }

  if (count == 0) {
    fprintf(stderr, "    (none found - check @use directive)\n");
  }

  return false;
}
