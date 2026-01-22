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
    bool is_public;
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
        fprintf(stderr, "Error: Unsupported struct access pattern (type: %d)\n", object->type);
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
    entry->is_public = info->field_is_public[index];
    entry->next = field_cache[hash];
    field_cache[hash] = entry;
}

// Handle: obj.field (where obj is identifier)
static LLVMValueRef handle_identifier_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    const char *var_name = node->expr.member.object->expr.identifier.name;
    
    LLVM_Symbol *sym = find_symbol(ctx, var_name);
    if (!sym || sym->is_function) {
        fprintf(stderr, "Error: Variable %s not found or is a function\n", var_name);
        return NULL;
    }

    // Find struct info - try cache first
    StructInfo *struct_info = NULL;
    FieldAccessCache *cached = NULL;
    
    // Try to determine struct name from symbol
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
    
    // Fallback: find by field name
    if (!struct_info) {
        struct_info = find_struct_by_field_cached(ctx, field_name);
    }

    if (!struct_info) {
        fprintf(stderr, "Error: Could not find struct with field '%s'\n", field_name);
        return NULL;
    }

    // Get field index - use cache if available
    int field_index;
    LLVMTypeRef field_type;
    
    if (cached) {
        field_index = cached->field_index;
        field_type = cached->field_type;
        
        // Check permissions
        if (!cached->is_public) {
            fprintf(stderr, "Error: Field '%s' is private\n", field_name);
            return NULL;
        }
    } else {
        field_index = get_field_index(struct_info, field_name);
        if (field_index < 0) {
            fprintf(stderr, "Error: Field '%s' not found in struct '%s'\n",
                    field_name, struct_info->name);
            return NULL;
        }
        
        if (!is_field_access_allowed(ctx, struct_info, field_index)) {
            fprintf(stderr, "Error: Field '%s' is private\n", field_name);
            return NULL;
        }
        
        field_type = struct_info->field_types[field_index];
        
        // Cache for next time
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

    // Load and return
    return LLVMBuildLoad2(ctx->builder, field_type, field_ptr, "field_val");
}

// Handle: obj.field1.field2 (chained member access)
static LLVMValueRef handle_chained_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    
    // Recursively resolve base
    LLVMValueRef base_value = codegen_expr_struct_access(ctx, node->expr.member.object);
    if (!base_value) {
        fprintf(stderr, "Error: Failed to resolve chained member access\n");
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
            struct_info = find_struct_by_field_cached(ctx, field_name);
        }

        if (struct_info) {
            struct_ptr = alloca_and_store(ctx, base_type, base_value, "chained_struct_temp");
        }
    } else if (base_kind == LLVMPointerTypeKind) {
        struct_ptr = base_value;
        struct_info = find_struct_by_field_cached(ctx, field_name);
    } else {
        fprintf(stderr, "Error: Chained access does not produce struct (kind: %d)\n", base_kind);
        return NULL;
    }

    if (!struct_info || !struct_ptr) {
        fprintf(stderr, "Error: Could not resolve chained member access\n");
        return NULL;
    }

    // Use cached lookup
    FieldAccessCache *cached = lookup_field_cache(struct_info->name, field_name);
    int field_index;
    LLVMTypeRef field_type;
    
    if (cached) {
        field_index = cached->field_index;
        field_type = cached->field_type;
        if (!cached->is_public) {
            fprintf(stderr, "Error: Field '%s' is private\n", field_name);
            return NULL;
        }
    } else {
        field_index = get_field_index(struct_info, field_name);
        if (field_index < 0 || !is_field_access_allowed(ctx, struct_info, field_index)) {
            fprintf(stderr, "Error: Cannot access field '%s'\n", field_name);
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
        fprintf(stderr, "Error: Failed to generate indexed expression\n");
        return NULL;
    }

    LLVMTypeRef indexed_type = LLVMTypeOf(indexed_value);
    LLVMTypeKind indexed_kind = LLVMGetTypeKind(indexed_type);

    if (indexed_kind != LLVMStructTypeKind) {
        fprintf(stderr, "Error: Indexed expression is not a struct (kind: %d)\n", indexed_kind);
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
        struct_info = find_struct_by_field_cached(ctx, field_name);
    }

    if (!struct_info) {
        fprintf(stderr, "Error: Could not determine struct type for indexed access\n");
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
            struct_info = find_struct_by_field_cached(ctx, field_name);
        }
        if (struct_info) {
            struct_ptr = alloca_and_store(ctx, result_type, call_result, "call_result_temp");
        }
    } else if (result_kind == LLVMPointerTypeKind) {
        struct_ptr = call_result;
        struct_info = find_struct_by_field_cached(ctx, field_name);
    } else {
        return NULL;
    }

    if (!struct_info || !struct_ptr) return NULL;

    FieldAccessCache *cached = lookup_field_cache(struct_info->name, field_name);
    int field_index = cached ? cached->field_index : get_field_index(struct_info, field_name);
    LLVMTypeRef field_type = cached ? cached->field_type : struct_info->field_types[field_index];
    
    if (!cached) cache_field_access(struct_info, field_name, field_index);

    return struct_gep_load(ctx, struct_info->llvm_type, struct_ptr,
                          field_index, field_type, "field_val");
}

// Handle: (*ptr).field (dereference member access)
static LLVMValueRef handle_deref_member(CodeGenContext *ctx, AstNode *node) {
    const char *field_name = node->expr.member.member;
    
    LLVMValueRef ptr = codegen_expr(ctx, node->expr.member.object->expr.deref.object);
    if (!ptr) return NULL;

    StructInfo *struct_info = find_struct_by_field_cached(ctx, field_name);
    if (!struct_info) {
        fprintf(stderr, "Error: Could not find struct with field '%s'\n", field_name);
        return NULL;
    }

    FieldAccessCache *cached = lookup_field_cache(struct_info->name, field_name);
    int field_index = cached ? cached->field_index : get_field_index(struct_info, field_name);
    LLVMTypeRef field_type = cached ? cached->field_type : struct_info->field_types[field_index];
    
    if (!cached) cache_field_access(struct_info, field_name, field_index);

    return struct_gep_load(ctx, struct_info->llvm_type, ptr,
                          field_index, field_type, "field_val");
}