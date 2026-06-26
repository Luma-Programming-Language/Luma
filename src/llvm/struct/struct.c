#include "../llvm.h"

// Find a concrete struct that embeds base_info as its first field and contains field_name.
// Used to resolve field access on base-typed pointers (C-style inheritance pattern).
// Returns NULL if no such struct is found.
StructInfo *find_concrete_struct_for_base(CodeGenContext *ctx,
                                          StructInfo     *base_info,
                                          const char     *field_name) {
    StructInfo *best = NULL;
    for (StructInfo *info = ctx->struct_types; info; info = info->next) {
        if (info->field_count < 2) continue;
        if (info->field_types[0] != base_info->llvm_type) continue;
        if (get_field_index(info, field_name) < 0) continue;
        if (!best || info->field_count > best->field_count) {
            best = info;
        }
    }
    return best;
}

// Find a struct type by name
StructInfo *find_struct_type(CodeGenContext *ctx, const char *name) {
  StructInfo *cached = lookup_cached_struct(name);
  if (cached)
    return cached;

  for (StructInfo *info = ctx->struct_types; info; info = info->next) {
    if (strcmp(info->name, name) == 0) {
      cache_struct(name, info);
      return info;
    }
  }
  return NULL;
}

// Add a struct type to the context
void add_struct_type(CodeGenContext *ctx, StructInfo *struct_info) {
  struct_info->next = ctx->struct_types;
  ctx->struct_types = struct_info;
}

// Get field index by name in a struct
int get_field_index(StructInfo *struct_info, const char *field_name) {
  for (size_t i = 0; i < struct_info->field_count; i++) {
    if (strcmp(struct_info->field_names[i], field_name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

// Check if field access is allowed (public field, or access from within the struct)
bool is_field_access_allowed(CodeGenContext *ctx, StructInfo *struct_info,
                             int field_index) {
  if (field_index < 0 || field_index >= (int)struct_info->field_count) {
    return false;
  }
  if (struct_info->field_is_public[field_index]) {
    return true;
  }
  // Allow private field access from within the struct's own methods
  if (ctx->current_function) {
    const char *func_name = LLVMGetValueName(ctx->current_function);
    if (func_name) {
      size_t name_len = strlen(struct_info->name);
      if (strncmp(func_name, struct_info->name, name_len) == 0 &&
          func_name[name_len] == '.') {
        return true;
      }
    }
  }
  return false;
}

LLVMValueRef codegen_stmt_struct(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_STMT_STRUCT) {
    return NULL;
  }

  const char *struct_name = node->stmt.struct_decl.name;
  size_t public_count = node->stmt.struct_decl.public_count;
  size_t private_count = node->stmt.struct_decl.private_count;

  // Count data fields from both FIELD_DECL and SPREAD_DECL
  size_t data_field_count = 0;
  for (size_t i = 0; i < public_count; i++) {
    AstNode *member = node->stmt.struct_decl.public_members[i];
    if (member->type == AST_STMT_FIELD_DECL &&
        !member->stmt.field_decl.function) {
      data_field_count++;
    } else if (member->type == AST_STMT_SPREAD_DECL) {
      // Include parent struct's data fields
      AstNode *parent_type = member->stmt.spread_decl.type;
      if (parent_type && parent_type->type == AST_TYPE_BASIC) {
        StructInfo *parent = find_struct_type(ctx, parent_type->type_data.basic.name);
        if (parent)
          data_field_count += parent->field_count;
      }
    }
  }
  for (size_t i = 0; i < private_count; i++) {
    AstNode *member = node->stmt.struct_decl.private_members[i];
    if (member->type == AST_STMT_FIELD_DECL &&
        !member->stmt.field_decl.function) {
      data_field_count++;
    } else if (member->type == AST_STMT_SPREAD_DECL) {
      AstNode *parent_type = member->stmt.spread_decl.type;
      if (parent_type && parent_type->type == AST_TYPE_BASIC) {
        StructInfo *parent = find_struct_type(ctx, parent_type->type_data.basic.name);
        if (parent)
          data_field_count += parent->field_count;
      }
    }
  }

  if (data_field_count == 0) {
    fprintf(stderr, "Error: Struct %s must have at least one data field\n",
            struct_name);
    return NULL;
  }

  // Check if struct already exists
  if (find_struct_type(ctx, struct_name)) {
    fprintf(stderr, "Error: Struct %s is already defined\n", struct_name);
    return NULL;
  }

  // Create StructInfo for data fields only
  StructInfo *struct_info = (StructInfo *)arena_alloc(
      ctx->arena, sizeof(StructInfo), alignof(StructInfo));

  struct_info->name = arena_strdup(ctx->arena, struct_name);
  struct_info->field_count = data_field_count;
  struct_info->is_public = node->stmt.struct_decl.is_public;

  // Allocate arrays for field information
  struct_info->field_names = (char **)arena_alloc(
      ctx->arena, sizeof(char *) * data_field_count, alignof(char *));
  struct_info->field_types = (LLVMTypeRef *)arena_alloc(
      ctx->arena, sizeof(LLVMTypeRef) * data_field_count, alignof(LLVMTypeRef));
  struct_info->field_element_types = (LLVMTypeRef *)arena_alloc(
      ctx->arena, sizeof(LLVMTypeRef) * data_field_count, alignof(LLVMTypeRef));
  struct_info->field_is_public = (bool *)arena_alloc(
      ctx->arena, sizeof(bool) * data_field_count, alignof(bool));

  struct_info->llvm_type = LLVMStructCreateNamed(ctx->context, struct_name);

  // Add to context IMMEDIATELY so it can be found during field type resolution
  add_struct_type(ctx, struct_info);

  // Process all data fields (spread first, then own fields)
  size_t field_index = 0;

  // First pass: process spread_decl members (parent fields come first)
  for (size_t i = 0; i < public_count; i++) {
    AstNode *member = node->stmt.struct_decl.public_members[i];
    if (member->type != AST_STMT_SPREAD_DECL)
      continue;
    AstNode *parent_type = member->stmt.spread_decl.type;
    if (!parent_type || parent_type->type != AST_TYPE_BASIC)
      continue;
    StructInfo *parent = find_struct_type(ctx, parent_type->type_data.basic.name);
    if (!parent) {
      fprintf(stderr, "Error: Parent struct '%s' not found for spread in '%s'\n",
              parent_type->type_data.basic.name, struct_name);
      return NULL;
    }
    for (size_t j = 0; j < parent->field_count; j++) {
      struct_info->field_names[field_index] = arena_strdup(ctx->arena, parent->field_names[j]);
      struct_info->field_types[field_index] = parent->field_types[j];
      struct_info->field_element_types[field_index] = parent->field_element_types[j];
      struct_info->field_is_public[field_index] = member->stmt.spread_decl.is_public;
      field_index++;
    }
  }
  for (size_t i = 0; i < private_count; i++) {
    AstNode *member = node->stmt.struct_decl.private_members[i];
    if (member->type != AST_STMT_SPREAD_DECL)
      continue;
    AstNode *parent_type = member->stmt.spread_decl.type;
    if (!parent_type || parent_type->type != AST_TYPE_BASIC)
      continue;
    StructInfo *parent = find_struct_type(ctx, parent_type->type_data.basic.name);
    if (!parent) {
      fprintf(stderr, "Error: Parent struct '%s' not found for spread in '%s'\n",
              parent_type->type_data.basic.name, struct_name);
      return NULL;
    }
    for (size_t j = 0; j < parent->field_count; j++) {
      struct_info->field_names[field_index] = arena_strdup(ctx->arena, parent->field_names[j]);
      struct_info->field_types[field_index] = parent->field_types[j];
      struct_info->field_element_types[field_index] = parent->field_element_types[j];
      struct_info->field_is_public[field_index] = member->stmt.spread_decl.is_public;
      field_index++;
    }
  }

  // Process public data fields (own fields)
  for (size_t i = 0; i < public_count; i++) {
    AstNode *member = node->stmt.struct_decl.public_members[i];
    if (member->type != AST_STMT_FIELD_DECL)
      continue;
    if (member->stmt.field_decl.function)
      continue;

    const char *field_name = member->stmt.field_decl.name;
    for (size_t j = 0; j < field_index; j++) {
      if (strcmp(struct_info->field_names[j], field_name) == 0) {
        fprintf(stderr, "Error: Duplicate field name '%s' in struct %s\n",
                field_name, struct_name);
        return NULL;
      }
    }
    struct_info->field_names[field_index] = arena_strdup(ctx->arena, field_name);
    struct_info->field_types[field_index] = codegen_type(ctx, member->stmt.field_decl.type);
    struct_info->field_element_types[field_index] = extract_element_type_from_ast(ctx, member->stmt.field_decl.type);
    struct_info->field_is_public[field_index] = true;
    if (!struct_info->field_types[field_index]) {
      fprintf(stderr, "Error: Failed to resolve type for field %s in struct %s\n",
              field_name, struct_name);
      return NULL;
    }
    field_index++;
  }

  // Process private data fields (own fields)
  for (size_t i = 0; i < private_count; i++) {
    AstNode *member = node->stmt.struct_decl.private_members[i];
    if (member->type != AST_STMT_FIELD_DECL)
      continue;
    if (member->stmt.field_decl.function)
      continue;

    const char *field_name = member->stmt.field_decl.name;
    for (size_t j = 0; j < field_index; j++) {
      if (strcmp(struct_info->field_names[j], field_name) == 0) {
        fprintf(stderr, "Error: Duplicate field name '%s' in struct %s\n",
                field_name, struct_name);
        return NULL;
      }
    }
    struct_info->field_names[field_index] = arena_strdup(ctx->arena, field_name);
    struct_info->field_types[field_index] = codegen_type(ctx, member->stmt.field_decl.type);
    struct_info->field_element_types[field_index] = extract_element_type_from_ast(ctx, member->stmt.field_decl.type);
    struct_info->field_is_public[field_index] = member->stmt.field_decl.is_public;
    if (!struct_info->field_types[field_index]) {
      fprintf(stderr, "Error: Failed to resolve type for field %s in struct %s\n",
              field_name, struct_name);
      return NULL;
    }
    field_index++;
  }

  // CRITICAL: Set the struct body AFTER all field types are resolved
  LLVMStructSetBody(struct_info->llvm_type, struct_info->field_types,
                    data_field_count, false);

  // Process own methods (both non-static and static)
  for (size_t i = 0; i < public_count; i++) {
    AstNode *member = node->stmt.struct_decl.public_members[i];
    if (member->type == AST_STMT_FIELD_DECL && member->stmt.field_decl.function) {
      AstNode *func_node = member->stmt.field_decl.function;
      const char *method_name = member->stmt.field_decl.name;
      codegen_struct_method(ctx, func_node, struct_info, method_name, true,
                            member->stmt.field_decl.is_static);
    }
  }
  for (size_t i = 0; i < private_count; i++) {
    AstNode *member = node->stmt.struct_decl.private_members[i];
    if (member->type == AST_STMT_FIELD_DECL && member->stmt.field_decl.function) {
      AstNode *func_node = member->stmt.field_decl.function;
      const char *method_name = member->stmt.field_decl.name;
      codegen_struct_method(ctx, func_node, struct_info, method_name, false,
                            member->stmt.field_decl.is_static);
    }
  }

  // Register inherited method symbols from spread_decl parents
  // This allows "Child.method" qualified lookup to find the parent's function
  // by creating "Child.method" symbol entries pointing to "Parent.method" LLVM functions
  for (size_t i = 0; i < public_count; i++) {
    AstNode *member = node->stmt.struct_decl.public_members[i];
    if (member->type != AST_STMT_SPREAD_DECL) continue;
    AstNode *parent_type = member->stmt.spread_decl.type;
    if (!parent_type || parent_type->type != AST_TYPE_BASIC) continue;

    LLVM_Symbol *sym = ctx->current_module ? ctx->current_module->symbols : NULL;
    size_t parent_name_len = strlen(parent_type->type_data.basic.name);
    while (sym) {
      if (strncmp(sym->name, parent_type->type_data.basic.name, parent_name_len) == 0 &&
          sym->name[parent_name_len] == '.') {
        const char *method_part = sym->name + parent_name_len + 1;
        size_t child_qlen = strlen(struct_name) + 1 + strlen(method_part) + 1;
        char *child_qname = arena_alloc(ctx->arena, child_qlen, 1);
        snprintf(child_qname, child_qlen, "%s.%s", struct_name, method_part);
        add_symbol_to_module(ctx->current_module, child_qname, sym->value,
                             sym->type, sym->is_function);
      }
      sym = sym->next;
    }
  }
  for (size_t i = 0; i < private_count; i++) {
    AstNode *member = node->stmt.struct_decl.private_members[i];
    if (member->type != AST_STMT_SPREAD_DECL) continue;
    AstNode *parent_type = member->stmt.spread_decl.type;
    if (!parent_type || parent_type->type != AST_TYPE_BASIC) continue;

    LLVM_Symbol *sym = ctx->current_module ? ctx->current_module->symbols : NULL;
    size_t parent_name_len = strlen(parent_type->type_data.basic.name);
    while (sym) {
      if (strncmp(sym->name, parent_type->type_data.basic.name, parent_name_len) == 0 &&
          sym->name[parent_name_len] == '.') {
        const char *method_part = sym->name + parent_name_len + 1;
        size_t child_qlen = strlen(struct_name) + 1 + strlen(method_part) + 1;
        char *child_qname = arena_alloc(ctx->arena, child_qlen, 1);
        snprintf(child_qname, child_qlen, "%s.%s", struct_name, method_part);
        add_symbol_to_module(ctx->current_module, child_qname, sym->value,
                             sym->type, sym->is_function);
      }
      sym = sym->next;
    }
  }

  // ===== CACHING =====
  cache_struct(struct_info->name, struct_info);

  return NULL;
}

LLVMValueRef codegen_struct_method(CodeGenContext *ctx, AstNode *func_node,
                                   StructInfo *struct_info,
                                   const char *method_name, bool is_public,
                                   bool is_static) {
  if (!func_node || func_node->type != AST_STMT_FUNCTION) {
    fprintf(stderr, "Error: Invalid function node for method '%s'\n",
            method_name);
    return NULL;
  }

  // Get method signature
  AstNode *return_type_node = func_node->stmt.func_decl.return_type;
  size_t original_param_count = func_node->stmt.func_decl.param_count;
  AstNode **original_param_type_nodes = func_node->stmt.func_decl.param_types;
  char **original_param_names = func_node->stmt.func_decl.param_names;

  // For non-static methods: implicit 'self' as first parameter
  // For static methods: no 'self' parameter
  size_t param_count = is_static ? original_param_count : original_param_count + 1;

  // Allocate arrays for parameters
  LLVMTypeRef *llvm_param_types = (LLVMTypeRef *)arena_alloc(
      ctx->arena, sizeof(LLVMTypeRef) * param_count, alignof(LLVMTypeRef));

  char **param_names = (char **)arena_alloc(
      ctx->arena, sizeof(char *) * param_count, alignof(char *));

  AstNode **param_type_nodes = (AstNode **)arena_alloc(
      ctx->arena, sizeof(AstNode *) * param_count, alignof(AstNode *));

  size_t offset = 0;
  if (!is_static) {
    // First parameter is 'self' - a pointer to the struct
    llvm_param_types[0] = LLVMPointerType(struct_info->llvm_type, 0);
    param_names[0] = "self";
    param_type_nodes[0] = NULL;
    offset = 1;
  }

  // Copy the original parameters
  for (size_t i = 0; i < original_param_count; i++) {
    llvm_param_types[i + offset] = codegen_type(ctx, original_param_type_nodes[i]);
    if (!llvm_param_types[i + offset]) {
      fprintf(stderr,
              "Error: Failed to resolve parameter type %zu for method '%s'\n",
              i, method_name);
      return NULL;
    }
    param_names[i + offset] = original_param_names[i];
    param_type_nodes[i + offset] = original_param_type_nodes[i];
  }

  // CREATE QUALIFIED METHOD NAME: StructName.method or StructName::method
  const char *sep = is_static ? "::" : ".";
  size_t qualified_name_len =
      strlen(struct_info->name) + strlen(sep) + strlen(method_name) + 1;
  char *qualified_method_name = arena_alloc(ctx->arena, qualified_name_len, 1);
  snprintf(qualified_method_name, qualified_name_len, "%s%s%s",
           struct_info->name, sep, method_name);

  // Create function type
  LLVMTypeRef llvm_return_type = codegen_type(ctx, return_type_node);
  if (!llvm_return_type) {
    fprintf(stderr, "Error: Failed to resolve return type for method '%s'\n",
            method_name);
    return NULL;
  }

  LLVMTypeRef func_type =
      LLVMFunctionType(llvm_return_type, llvm_param_types, param_count, 0);

  // Get the current LLVM module
  LLVMModuleRef current_llvm_module =
      ctx->current_module ? ctx->current_module->module : ctx->module;

  // CHANGED: Use qualified_method_name instead of method_name
  LLVMValueRef func =
      LLVMAddFunction(current_llvm_module, qualified_method_name, func_type);

  if (!func) {
    fprintf(stderr, "Error: Failed to create LLVM function for method '%s'\n",
            qualified_method_name);
    return NULL;
  }
  // Set linkage
  if (is_public) {
    LLVMSetLinkage(func, LLVMExternalLinkage);
  } else {
    LLVMSetLinkage(func, LLVMInternalLinkage);
  }

  // Register the function in the symbol table under its qualified name
  add_symbol_to_module(ctx->current_module, qualified_method_name, func,
                       func_type, true);

  // CRITICAL: Save the old function context before starting method generation
  LLVMValueRef old_function = ctx->current_function;

  // Set current function context
  ctx->current_function = func;

  // Create entry basic block
  LLVMBasicBlockRef entry =
      LLVMAppendBasicBlockInContext(ctx->context, func, "entry");
  LLVMPositionBuilderAtEnd(ctx->builder, entry);

  // Add all parameters to symbol table
  for (size_t i = 0; i < param_count; i++) {
    LLVMValueRef param = LLVMGetParam(func, i);
    const char *param_name = param_names[i];

    LLVMSetValueName2(param, param_name, strlen(param_name));

    LLVMValueRef alloca =
        LLVMBuildAlloca(ctx->builder, llvm_param_types[i], param_name);
    LLVMBuildStore(ctx->builder, param, alloca);

    LLVMTypeRef element_type = NULL;

    if (!is_static && i == 0) {
      // 'self' is a pointer to the struct
      element_type = struct_info->llvm_type;
    } else {
      element_type = extract_element_type_from_ast(ctx, param_type_nodes[i]);
    }

    add_symbol_with_element_type(ctx, param_name, alloca, llvm_param_types[i],
                                 element_type, false);
  }

  // Generate method body
  AstNode *body = func_node->stmt.func_decl.body;
  if (body) {
    codegen_stmt(ctx, body);
  }

  // Add return if missing for void functions
  if (LLVMGetTypeKind(llvm_return_type) == LLVMVoidTypeKind) {
    if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(ctx->builder))) {
      LLVMBuildRetVoid(ctx->builder);
    }
  }

  // Verify the function
  if (LLVMVerifyFunction(func, LLVMReturnStatusAction)) {
    fprintf(stderr, "Error: Function verification failed for method '%s'\n",
            method_name);
    LLVMDumpValue(func);
    // Restore context even on error
    ctx->current_function = old_function;
    return NULL;
  }

  // CRITICAL: Restore the old function context
  ctx->current_function = old_function;

  return func;
}

// Handle individual field declarations (mainly for completeness)
LLVMValueRef codegen_stmt_field(CodeGenContext *ctx, AstNode *node) {
  // Field declarations are handled by the parent struct
  // This function exists for completeness and error checking
  (void)ctx; // Suppress unused parameter warning

  if (!node || node->type != AST_STMT_FIELD_DECL) {
    return NULL;
  }

  // If we reach here, it means a field declaration was used outside a struct
  fprintf(stderr, "Error: Field declaration '%s' must be inside a struct\n",
          node->stmt.field_decl.name);
  return NULL;
}

// Handle struct member assignment (obj.field = value)
LLVMValueRef codegen_expr_struct_assignment(CodeGenContext *ctx,
                                            AstNode *node) {
  if (!node || node->type != AST_EXPR_ASSIGNMENT) {
    return NULL;
  }

  AstNode *target = node->expr.assignment.target;
  LLVMValueRef value = codegen_expr(ctx, node->expr.assignment.value);
  if (!value) {
    return NULL;
  }

  if (target->type != AST_EXPR_MEMBER) {
    return NULL; // Not a struct field assignment
  }

  const char *field_name = target->expr.member.member;
  AstNode *object = target->expr.member.object;

  if (object->type == AST_EXPR_IDENTIFIER) {
    const char *var_name = object->expr.identifier.name;
    LLVM_Symbol *sym = find_symbol(ctx, var_name);
    if (!sym || sym->is_function) {
      fprintf(stderr, "Error: Variable %s not found or is a function\n",
              var_name);
      return NULL;
    }

    // Find the struct info from the symbol's type
    StructInfo *struct_info = NULL;
    LLVMTypeRef sym_type = sym->type;
    LLVMTypeKind sym_kind = LLVMGetTypeKind(sym_type);

    if (sym_kind == LLVMPointerTypeKind && sym->element_type) {
      const char *struct_name = LLVMGetStructName(sym->element_type);
      if (struct_name) {
        struct_info = find_struct_type(ctx, struct_name);
      }
    } else if (sym_kind == LLVMStructTypeKind) {
      const char *struct_name = LLVMGetStructName(sym_type);
      if (struct_name) {
        struct_info = find_struct_type(ctx, struct_name);
      }
    }

    if (!struct_info) {
      fprintf(stderr, "Error: Could not find struct for field '%s'\n",
              field_name);
      return NULL;
    }

    // Try declared struct first, then embedded-base resolution
    int field_index = get_field_index(struct_info, field_name);

    if (field_index < 0) {
      StructInfo *concrete =
          find_concrete_struct_for_base(ctx, struct_info, field_name);
      if (concrete) {
        struct_info = concrete;
        field_index = get_field_index(concrete, field_name);
      }
    }

    if (field_index < 0) {
      fprintf(stderr,
              "Error: Field '%s' not found in struct '%s' or any struct "
              "embedding it\n",
              field_name, struct_info->name);
      return NULL;
    }

    if (!is_field_access_allowed(ctx, struct_info, field_index)) {
      fprintf(stderr, "Error: Field '%s' in struct '%s' is private\n",
              field_name, struct_info->name);
      return NULL;
    }

    // Handle the different cases for assignment
    LLVMValueRef struct_ptr;

    if (LLVMGetTypeKind(sym_type) == LLVMPointerTypeKind) {
      LLVMTypeRef ptr_to_struct_type =
          LLVMPointerType(struct_info->llvm_type, 0);
      struct_ptr = LLVMBuildLoad2(ctx->builder, ptr_to_struct_type, sym->value,
                                  "load_struct_ptr");
    } else if (sym_type == struct_info->llvm_type) {
      struct_ptr = sym->value;
    } else {
      fprintf(stderr,
              "Error: Variable '%s' is not a struct or pointer to struct\n",
              var_name);
      return NULL;
    }

    // Check type compatibility
    LLVMTypeRef expected_type = struct_info->field_types[field_index];
    LLVMTypeRef actual_type = LLVMTypeOf(value);

    if (expected_type != actual_type) {
      LLVMTypeKind expected_kind = LLVMGetTypeKind(expected_type);
      LLVMTypeKind actual_kind = LLVMGetTypeKind(actual_type);

      if (expected_kind == LLVMIntegerTypeKind &&
          actual_kind == LLVMIntegerTypeKind) {
        unsigned expected_bits = LLVMGetIntTypeWidth(expected_type);
        unsigned actual_bits = LLVMGetIntTypeWidth(actual_type);
        if (expected_bits > actual_bits) {
          value = LLVMBuildSExt(ctx->builder, value, expected_type, "extend");
        } else if (expected_bits < actual_bits) {
          value = LLVMBuildTrunc(ctx->builder, value, expected_type, "trunc");
        }
      } else if (expected_kind == LLVMFloatTypeKind &&
                 actual_kind == LLVMDoubleTypeKind) {
        value = LLVMBuildFPTrunc(ctx->builder, value, expected_type, "fptrunc");
      } else if (expected_kind == LLVMDoubleTypeKind &&
                 actual_kind == LLVMFloatTypeKind) {
        value = LLVMBuildFPExt(ctx->builder, value, expected_type, "fpext");
      }
    }

    LLVMValueRef field_ptr =
        LLVMBuildStructGEP2(ctx->builder, struct_info->llvm_type, struct_ptr,
                            field_index, "field_ptr");

    LLVMBuildStore(ctx->builder, value, field_ptr);
    return value;
  }

  fprintf(stderr, "Error: Unsupported struct assignment pattern\n");
  return NULL;
}

// Handle struct type in type system
LLVMTypeRef codegen_type_struct(CodeGenContext *ctx, const char *struct_name) {
  StructInfo *struct_info = find_struct_type(ctx, struct_name);
  if (struct_info) {
    return struct_info->llvm_type;
  }

  fprintf(stderr, "Error: Struct type '%s' not found\n", struct_name);
  return NULL;
}

LLVMValueRef codegen_struct_literal(CodeGenContext *ctx,
                                    const char *struct_name,
                                    LLVMValueRef *field_values,
                                    size_t field_count) {
  StructInfo *struct_info = find_struct_type(ctx, struct_name);
  if (!struct_info) {
    fprintf(stderr, "Error: Struct type '%s' not found for literal\n",
            struct_name);
    return NULL;
  }

  if (field_count != struct_info->field_count) {
    fprintf(stderr,
            "Error: Struct literal field count mismatch for '%s': expected "
            "%zu, got %zu\n",
            struct_name, struct_info->field_count, field_count);
    return NULL;
  }

  // Create struct value using LLVMConstStruct for constants
  // or build it piece by piece for runtime values
  bool all_constant = true;
  for (size_t i = 0; i < field_count; i++) {
    if (!LLVMIsConstant(field_values[i])) {
      all_constant = false;
      break;
    }
  }

  if (all_constant) {
    return LLVMConstStructInContext(ctx->context, field_values, field_count, 0);
  } else {
    LLVMValueRef struct_alloca =
        LLVMBuildAlloca(ctx->builder, struct_info->llvm_type, "struct_lit");

    for (size_t i = 0; i < field_count; i++) {
      LLVMValueRef field_ptr = LLVMBuildStructGEP2(
          ctx->builder, struct_info->llvm_type, struct_alloca, i, "init_field");
      LLVMBuildStore(ctx->builder, field_values[i], field_ptr);
    }

    return LLVMBuildLoad2(ctx->builder, struct_info->llvm_type, struct_alloca,
                          "struct_val");
  }
}