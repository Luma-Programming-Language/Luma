#include "../llvm.h"
#include <stdlib.h>

// ============================================================================
// STRUCT ACCESS - Optimized field lookup with caching
// ============================================================================

// Field access cache for faster repeated lookups
typedef struct FieldAccessCache {
    const char *struct_name;
    const char *field_name;
    int field_index;
    LLVMTypeRef field_type;
    LLVMTypeRef element_type;
    struct FieldAccessCache *next;
} FieldAccessCache;

static FieldAccessCache *field_cache[256] = {0};

// Forward declarations
static FieldAccessCache *lookup_field_cache(const char *struct_name, const char *field_name);
static void cache_field_access(StructInfo *info, const char *field_name, int index);
static LLVMValueRef handle_identifier_member(CodeGenContext *ctx, AstNode *node);
static LLVMValueRef handle_chained_member(CodeGenContext *ctx, AstNode *node);
static LLVMValueRef handle_indexed_member(CodeGenContext *ctx, AstNode *node);
static LLVMValueRef handle_call_result_member(CodeGenContext *ctx, AstNode *node);
static LLVMValueRef handle_deref_member(CodeGenContext *ctx, AstNode *node);

// Main entry point - optimized routing
LLVMValueRef codegen_expr_struct_access(CodeGenContext *ctx, AstNode *node) {
    if (!node || node->type != AST_EXPR_MEMBER) {
        return NULL;
    }

    AstNode *object = node->expr.member.object;

    // Route based on object type
    switch (object->type) {
    case AST_EXPR_IDENTIFIER:
        return handle_identifier_member(ctx, node);
    case AST_EXPR_MEMBER:
        return handle_chained_member(ctx, node);
    case AST_EXPR_INDEX:
        return handle_indexed_member(ctx, node);
    case AST_EXPR_CALL:
        return handle_call_result_member(ctx, node);
    case AST_EXPR_DEREF:
        return handle_deref_member(ctx, node);
    default:
        cg_error(ctx, node, "Codegen Error",
                 "Unsupported struct access pattern (type: %d)", object->type);
        return NULL;
    }
}

// Cache lookup - O(1) average case
static FieldAccessCache *lookup_field_cache(const char *struct_name, const char *field_name) {
    unsigned hash = (hash_string(struct_name) ^ hash_string(field_name)) % 256;
    
    for (FieldAccessCache *entry = field_cache[hash]; entry; entry = entry->next) {
        if (strcmp(entry->struct_name, struct_name) == 0 &&
            strcmp(entry->field_name, field_name) == 0) {
            return entry;
        }
    }
    return NULL;
}

// Cache a field access
static void cache_field_access(StructInfo *info, const char *field_name, int index) {
    unsigned hash = (hash_string(info->name) ^ hash_string(field_name)) % 256;
    
    // Check if already cached
    for (FieldAccessCache *entry = field_cache[hash]; entry; entry = entry->next) {
        if (strcmp(entry->struct_name, info->name) == 0 &&
            strcmp(entry->field_name, field_name) == 0) {
            return; // Already cached
        }
    }
    
    FieldAccessCache *entry = malloc(sizeof(FieldAccessCache));
    entry->struct_name = info->name;
    entry->field_name = field_name;
    entry->field_index = index;
    entry->field_type = info->field_types[index];
    entry->element_type = info->field_element_types[index];
    entry->next = field_cache[hash];
    field_cache[hash] = entry;
}

// Handle: obj.field (where obj is identifier)
static LLVMValueRef handle_identifier_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    const char *var_name = node->expr.member.object->expr.identifier.name;

    LLVM_Symbol *sym = find_symbol(ctx, var_name);
    if (!sym || sym->is_function) {
        cg_error(ctx, node, "Codegen Error",
                 "Variable %s not found or is a function", var_name);
        return NULL;
    }

    // Find struct info - try cache first
    StructInfo *struct_info = NULL;
    FieldAccessCache *cached = NULL;

    LLVMTypeRef symbol_type = sym->type;
    LLVMTypeKind symbol_kind = LLVMGetTypeKind(symbol_type);

    if (symbol_kind == LLVMPointerTypeKind && sym->element_type) {
        for (StructInfo *info = ctx->struct_types; info; info = info->next) {
            if (info->llvm_type == sym->element_type) {
                struct_info = info;
                cached = lookup_field_cache(info->name, field_name);
                break;
            }
        }
    } else if (symbol_kind == LLVMStructTypeKind) {
        for (StructInfo *info = ctx->struct_types; info; info = info->next) {
            if (info->llvm_type == symbol_type) {
                struct_info = info;
                cached = lookup_field_cache(info->name, field_name);
                break;
            }
        }
    }

    // Fallback: resolve struct by name from LLVM type
    if (!struct_info) {
        if (symbol_kind == LLVMPointerTypeKind && sym->element_type) {
            const char *type_name = LLVMGetStructName(sym->element_type);
            if (type_name) struct_info = find_struct_type(ctx, type_name);
        } else if (symbol_kind == LLVMStructTypeKind) {
            const char *type_name = LLVMGetStructName(symbol_type);
            if (type_name) struct_info = find_struct_type(ctx, type_name);
        }
    }

    if (!struct_info) {
        cg_error(ctx, node, "Codegen Error",
                 "Could not find struct with field '%s'", field_name);
        return NULL;
    }

    // Get field index - use cache if available
    int field_index;
    LLVMTypeRef field_type;

    if (cached) {
        field_index = cached->field_index;
        field_type = cached->field_type;

        if (!is_field_access_allowed(ctx, struct_info, field_index)) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' in struct '%s' is private", field_name,
                     struct_info->name);
            return NULL;
        }
    } else {
        field_index = get_field_index(struct_info, field_name);

        // Try embedded-base resolution if not found in declared type
        if (field_index < 0) {
            StructInfo *concrete =
                find_concrete_struct_for_base(ctx, struct_info, field_name);
            if (concrete) {
                struct_info = concrete;
                field_index = get_field_index(concrete, field_name);
            }
        }

        if (field_index < 0) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' not found in struct '%s' or any struct "
                     "embedding it",
                     field_name, struct_info->name);
            return NULL;
        }

        if (!is_field_access_allowed(ctx, struct_info, field_index)) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' in struct '%s' is private", field_name,
                     struct_info->name);
            return NULL;
        }

        field_type = struct_info->field_types[field_index];
        cache_field_access(struct_info, field_name, field_index);
    }

    // Get struct pointer
    LLVMValueRef struct_ptr;
    if (symbol_kind == LLVMPointerTypeKind) {
        LLVMTypeRef ptr_to_struct_type = LLVMPointerType(struct_info->llvm_type, 0);
        struct_ptr = LLVMBuildLoad2(ctx->builder, ptr_to_struct_type,
                                    sym->value, "load_struct_ptr");
    } else {
        struct_ptr = sym->value;
    }

    // Generate GEP
    LLVMValueRef field_ptr = LLVMBuildStructGEP2(ctx->builder, struct_info->llvm_type,
                                                 struct_ptr, field_index, "field_ptr");

    // Handle array fields specially
    if (LLVMGetTypeKind(field_type) == LLVMArrayTypeKind) {
        LLVMValueRef indices[2] = {
            ctx->common_types.const_i32_0,
            ctx->common_types.const_i32_0
        };
        return LLVMBuildGEP2(ctx->builder, field_type, field_ptr, indices, 2,
                            "array_field_ptr");
    }

    return LLVMBuildLoad2(ctx->builder, field_type, field_ptr, "field_val");
}

// Handle: obj.field1.field2 (chained member access)
static LLVMValueRef handle_chained_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    
    // Recursively resolve base
    LLVMValueRef base_value = codegen_expr_struct_access(ctx, node->expr.member.object);
    if (!base_value) {
        cg_error(ctx, node, "Codegen Error", "Failed to resolve chained member access");
        return NULL;
    }

    LLVMTypeRef base_type = LLVMTypeOf(base_value);
    LLVMTypeKind base_kind = LLVMGetTypeKind(base_type);

    StructInfo *struct_info = NULL;
    LLVMValueRef struct_ptr;

    if (base_kind == LLVMStructTypeKind) {
        // Find struct type
        for (StructInfo *info = ctx->struct_types; info; info = info->next) {
            if (info->llvm_type == base_type) {
                struct_info = info;
                break;
            }
        }

        if (!struct_info) {
            const char *type_name = LLVMGetStructName(base_type);
            if (type_name) struct_info = find_struct_type(ctx, type_name);
        }

        if (struct_info) {
            struct_ptr = alloca_and_store(ctx, base_type, base_value, "chained_struct_temp");
        }
    } else if (base_kind == LLVMPointerTypeKind) {
        struct_ptr = base_value;
        LLVMTypeRef pointee_type = LLVMGetElementType(base_type);
        if (pointee_type) {
            const char *type_name = LLVMGetStructName(pointee_type);
            if (type_name) struct_info = find_struct_type(ctx, type_name);
        }
    } else {
        cg_error(ctx, node, "Codegen Error",
                 "Chained access does not produce struct (kind: %d)", base_kind);
        return NULL;
    }

    if (!struct_info || !struct_ptr) {
        cg_error(ctx, node, "Codegen Error", "Could not resolve chained member access");
        return NULL;
    }

    // Use cached lookup
    FieldAccessCache *cached = lookup_field_cache(struct_info->name, field_name);
    int field_index;
    LLVMTypeRef field_type;
    
    if (cached) {
        field_index = cached->field_index;
        field_type = cached->field_type;
        if (!is_field_access_allowed(ctx, struct_info, field_index)) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' in struct '%s' is private", field_name,
                     struct_info->name);
            return NULL;
        }
    } else {
        field_index = get_field_index(struct_info, field_name);

        if (field_index < 0) {
            StructInfo *concrete = find_concrete_struct_for_base(ctx, struct_info, field_name);
            if (concrete) {
                struct_info = concrete;
                field_index = get_field_index(concrete, field_name);
            }
        }

        if (field_index < 0 || !is_field_access_allowed(ctx, struct_info, field_index)) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' not found in struct '%s' or any struct embedding it",
                     field_name, struct_info->name);
            return NULL;
        }
        field_type = struct_info->field_types[field_index];
        cache_field_access(struct_info, field_name, field_index);
    }

    return struct_gep_load(ctx, struct_info->llvm_type, struct_ptr,
                          field_index, field_type, "field_val");
}

// Handle: arr[i].field (indexed then member access)
static LLVMValueRef handle_indexed_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    
    LLVMValueRef indexed_value = codegen_expr_index(ctx, node->expr.member.object);
    if (!indexed_value) {
        cg_error(ctx, node, "Codegen Error", "Failed to generate indexed expression");
        return NULL;
    }

    LLVMTypeRef indexed_type = LLVMTypeOf(indexed_value);
    LLVMTypeKind indexed_kind = LLVMGetTypeKind(indexed_type);

    if (indexed_kind != LLVMStructTypeKind) {
        cg_error(ctx, node, "Codegen Error",
                 "Indexed expression is not a struct (kind: %d)", indexed_kind);
        return NULL;
    }

    StructInfo *struct_info = NULL;
    for (StructInfo *info = ctx->struct_types; info; info = info->next) {
        if (info->llvm_type == indexed_type) {
            struct_info = info;
            break;
        }
    }

    if (!struct_info) {
        const char *type_name = LLVMGetStructName(indexed_type);
        if (type_name) struct_info = find_struct_type(ctx, type_name);
    }

    if (!struct_info) {
        cg_error(ctx, node, "Codegen Error",
                 "Could not determine struct type for indexed access");
        return NULL;
    }

    LLVMValueRef struct_ptr = alloca_and_store(ctx, indexed_type, indexed_value,
                                                "indexed_struct_temp");

    FieldAccessCache *cached = lookup_field_cache(struct_info->name, field_name);
    int field_index;
    LLVMTypeRef field_type;
    
    if (cached) {
        field_index = cached->field_index;
        field_type = cached->field_type;
    } else {
        field_index = get_field_index(struct_info, field_name);

        if (field_index < 0) {
            StructInfo *concrete = find_concrete_struct_for_base(ctx, struct_info, field_name);
            if (concrete) {
                struct_info = concrete;
                field_index = get_field_index(concrete, field_name);
            }
        }

        if (field_index < 0) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' not found in struct '%s' or any struct embedding it",
                     field_name, struct_info->name);
            return NULL;
        }
        field_type = struct_info->field_types[field_index];
        cache_field_access(struct_info, field_name, field_index);
    }

    return struct_gep_load(ctx, struct_info->llvm_type, struct_ptr,
                          field_index, field_type, "field_val");
}

// Handle: func().field (call result member access)
static LLVMValueRef handle_call_result_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    
    LLVMValueRef call_result = codegen_expr(ctx, node->expr.member.object);
    if (!call_result) return NULL;

    LLVMTypeRef result_type = LLVMTypeOf(call_result);
    LLVMTypeKind result_kind = LLVMGetTypeKind(result_type);

    StructInfo *struct_info = NULL;
    LLVMValueRef struct_ptr;

    if (result_kind == LLVMStructTypeKind) {
        for (StructInfo *info = ctx->struct_types; info; info = info->next) {
            if (info->llvm_type == result_type) {
                struct_info = info;
                break;
            }
        }
        if (!struct_info) {
            const char *type_name = LLVMGetStructName(result_type);
            if (type_name) struct_info = find_struct_type(ctx, type_name);
        }
        if (struct_info) {
            struct_ptr = alloca_and_store(ctx, result_type, call_result, "call_result_temp");
        }
    } else if (result_kind == LLVMPointerTypeKind) {
        struct_ptr = call_result;
        LLVMTypeRef pointee_type = LLVMGetElementType(result_type);
        if (pointee_type) {
            const char *type_name = LLVMGetStructName(pointee_type);
            if (type_name) struct_info = find_struct_type(ctx, type_name);
        }
    } else {
        return NULL;
    }

    if (!struct_info || !struct_ptr) return NULL;

    FieldAccessCache *cached = lookup_field_cache(struct_info->name, field_name);
    int field_index;
    LLVMTypeRef field_type;

    if (cached) {
        field_index = cached->field_index;
        field_type = cached->field_type;
    } else {
        field_index = get_field_index(struct_info, field_name);

        if (field_index < 0) {
            StructInfo *concrete = find_concrete_struct_for_base(ctx, struct_info, field_name);
            if (concrete) {
                struct_info = concrete;
                field_index = get_field_index(concrete, field_name);
            }
        }

        if (field_index < 0) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' not found in struct '%s' or any struct embedding it",
                     field_name, struct_info->name);
            return NULL;
        }
        field_type = struct_info->field_types[field_index];
        cache_field_access(struct_info, field_name, field_index);
    }

    return struct_gep_load(ctx, struct_info->llvm_type, struct_ptr,
                          field_index, field_type, "field_val");
}

// Handle: (*ptr).field (dereference member access)
static LLVMValueRef handle_deref_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    
    LLVMValueRef ptr = codegen_expr(ctx, node->expr.member.object->expr.deref.object);
    if (!ptr) return NULL;

    LLVMTypeRef ptr_type = LLVMTypeOf(ptr);
    LLVMTypeRef pointee_type = LLVMGetElementType(ptr_type);
    StructInfo *struct_info = NULL;
    if (pointee_type) {
        const char *type_name = LLVMGetStructName(pointee_type);
        if (type_name) struct_info = find_struct_type(ctx, type_name);
    }
    if (!struct_info) {
        cg_error(ctx, node, "Codegen Error",
                 "Could not find struct for deref member access");
        return NULL;
    }

    FieldAccessCache *cached = lookup_field_cache(struct_info->name, field_name);
    int field_index;
    LLVMTypeRef field_type;

    if (cached) {
        field_index = cached->field_index;
        field_type = cached->field_type;
    } else {
        field_index = get_field_index(struct_info, field_name);

        if (field_index < 0) {
            StructInfo *concrete = find_concrete_struct_for_base(ctx, struct_info, field_name);
            if (concrete) {
                struct_info = concrete;
                field_index = get_field_index(concrete, field_name);
            }
        }

        if (field_index < 0) {
            cg_error(ctx, node, "Codegen Error",
                     "Field '%s' not found in struct '%s' or any struct embedding it",
                     field_name, struct_info->name);
            return NULL;
        }
        field_type = struct_info->field_types[field_index];
        cache_field_access(struct_info, field_name, field_index);
    }

    return struct_gep_load(ctx, struct_info->llvm_type, ptr,
                          field_index, field_type, "field_val");
}

// ============================================================================
// LVALUE ADDRESS RESOLUTION - compute the address of an assignable expression
// ============================================================================

// Find a StructInfo given an LLVM struct type.
static StructInfo *find_struct_info_by_type(CodeGenContext *ctx,
                                            LLVMTypeRef type) {
  if (!type) {
    return NULL;
  }
  for (StructInfo *info = ctx->struct_types; info; info = info->next) {
    if (info->llvm_type == type) {
      return info;
    }
  }
  const char *type_name = LLVMGetStructName(type);
  if (type_name) {
    return find_struct_type(ctx, type_name);
  }
  return NULL;
}

// Resolve the address of an lvalue expression: identifiers, pointer derefs,
// array/pointer indexing, and chained member access (e.g. self.cells[idx].j).
//
// LLVM opaque pointers cannot report pointee types, so the type of the value
// stored at the returned address is passed back through *value_type_out, and
// the tracked element/pointee type (for pointer-typed values) through
// *element_type_out.
LLVMValueRef codegen_member_address(CodeGenContext *ctx, AstNode *node,
                                    LLVMTypeRef *value_type_out,
                                    LLVMTypeRef *element_type_out) {
  if (!node) {
    return NULL;
  }

  LLVMTypeRef value_type = NULL;
  LLVMTypeRef element_type = NULL;

  switch (node->type) {
  case AST_EXPR_IDENTIFIER: {
    LLVM_Symbol *sym = find_symbol(ctx, node->expr.identifier.name);
    if (!sym || sym->is_function) {
      cg_error(ctx, node, "Codegen Error",
               "Variable '%s' not found or is a function",
               node->expr.identifier.name);
      return NULL;
    }
    value_type = sym->type;
    element_type = sym->element_type;
    if (value_type_out) *value_type_out = value_type;
    if (element_type_out) *element_type_out = element_type;
    return sym->value;
  }

  case AST_EXPR_DEREF: {
    AstNode *object = node->expr.deref.object;
    LLVMValueRef ptr = codegen_expr(ctx, object);
    if (!ptr) {
      return NULL;
    }
    element_type = NULL;
    if (object->type == AST_EXPR_IDENTIFIER) {
      LLVM_Symbol *sym = find_symbol(ctx, object->expr.identifier.name);
      if (sym && !sym->is_function) {
        element_type = sym->element_type;
      }
    }
    if (!element_type) {
      cg_error(ctx, node, "Codegen Error",
               "Could not determine pointee type for dereference");
      return NULL;
    }
    value_type = element_type;
    if (value_type_out) *value_type_out = value_type;
    if (element_type_out) *element_type_out = NULL;
    return ptr;
  }

  case AST_EXPR_INDEX: {
    AstNode *obj = node->expr.index.object;
    LLVMTypeRef base_value_type = NULL;
    LLVMTypeRef base_element_type = NULL;
    LLVMValueRef base_addr =
        codegen_member_address(ctx, obj, &base_value_type, &base_element_type);
    if (!base_addr) {
      return NULL;
    }

    LLVMValueRef index = codegen_expr(ctx, node->expr.index.index);
    if (!index) {
      return NULL;
    }

    LLVMTypeKind base_kind = LLVMGetTypeKind(base_value_type);
    if (base_kind == LLVMPointerTypeKind) {
      if (!base_element_type) {
        cg_error(ctx, node, "Codegen Error",
                 "Could not determine element type for index expression");
        return NULL;
      }
      LLVMValueRef base_ptr = LLVMBuildLoad2(ctx->builder, base_value_type,
                                             base_addr, "load_base_ptr");
      value_type = base_element_type;
      if (value_type_out) *value_type_out = value_type;
      if (element_type_out) *element_type_out = NULL;
      return LLVMBuildGEP2(ctx->builder, base_element_type, base_ptr, &index,
                           1, "element_addr");
    } else if (base_kind == LLVMArrayTypeKind) {
      LLVMValueRef indices[2] = {ctx->common_types.const_i32_0, index};
      value_type = LLVMGetElementType(base_value_type);
      if (value_type_out) *value_type_out = value_type;
      if (element_type_out) *element_type_out = NULL;
      return LLVMBuildGEP2(ctx->builder, base_value_type, base_addr, indices,
                           2, "array_element_addr");
    }

    cg_error(ctx, node, "Codegen Error",
             "Cannot index into non-pointer, non-array lvalue");
    return NULL;
  }

  case AST_EXPR_MEMBER: {
    const char *field_name = node->expr.member.member;
    AstNode *object = node->expr.member.object;

    LLVMTypeRef obj_value_type = NULL;
    LLVMTypeRef obj_element_type = NULL;
    LLVMValueRef obj_addr =
        codegen_member_address(ctx, object, &obj_value_type, &obj_element_type);
    if (!obj_addr) {
      return NULL;
    }

    StructInfo *struct_info = NULL;
    LLVMValueRef struct_ptr = NULL;

    LLVMTypeKind obj_kind = LLVMGetTypeKind(obj_value_type);
    if (obj_kind == LLVMPointerTypeKind) {
      if (!obj_element_type) {
        cg_error(ctx, node, "Codegen Error",
                 "Cannot access field '%s': object pointee type unknown",
                 field_name);
        return NULL;
      }
      struct_info = find_struct_info_by_type(ctx, obj_element_type);
      if (struct_info) {
        struct_ptr = LLVMBuildLoad2(ctx->builder, obj_value_type, obj_addr,
                                    "load_struct_ptr");
      }
    } else if (obj_kind == LLVMStructTypeKind) {
      struct_info = find_struct_info_by_type(ctx, obj_value_type);
      struct_ptr = obj_addr;
    }

    if (!struct_info || !struct_ptr) {
      cg_error(ctx, node, "Codegen Error",
               "Cannot access field '%s' on non-struct lvalue", field_name);
      return NULL;
    }

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
      cg_error(ctx, node, "Codegen Error",
               "Field '%s' not found in struct '%s' or any struct embedding it",
               field_name, struct_info->name);
      return NULL;
    }

    if (!is_field_access_allowed(ctx, struct_info, field_index)) {
      cg_error(ctx, node, "Codegen Error",
               "Field '%s' in struct '%s' is private", field_name,
               struct_info->name);
      return NULL;
    }

    value_type = struct_info->field_types[field_index];
    element_type = struct_info->field_element_types[field_index];
    if (value_type_out) *value_type_out = value_type;
    if (element_type_out) *element_type_out = element_type;
    return LLVMBuildStructGEP2(ctx->builder, struct_info->llvm_type, struct_ptr,
                               field_index, "field_addr");
  }

  default:
    cg_error(ctx, node, "Codegen Error",
             "Unsupported lvalue expression type: %d", node->type);
    return NULL;
  }
}
