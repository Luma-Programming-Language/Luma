#include "../llvm.h"

// Helper function to infer struct type from context
static StructInfo *infer_struct_type_from_context(CodeGenContext *ctx,
                                                  char **field_names,
                                                  size_t field_count) {
  for (StructInfo *info = ctx->struct_types; info; info = info->next) {
    if (info->field_count != field_count)
      continue;

    bool all_match = true;
    for (size_t i = 0; i < field_count; i++) {
      bool found = false;
      for (size_t j = 0; j < info->field_count; j++) {
        if (strcmp(field_names[i], info->field_names[j]) == 0) {
          found = true;
          break;
        }
      }
      if (!found) { all_match = false; break; }
    }
    if (all_match) return info;
  }
  return NULL;
}

// Check if a field value is a spread expression (field_name is NULL and value is AST_EXPR_SPREAD)
static bool is_spread_entry(char *field_name, AstNode *field_value) {
  return !field_name && field_value && field_value->type == AST_EXPR_SPREAD;
}

LLVMValueRef codegen_expr_struct_literal(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_STRUCT) {
    fprintf(stderr, "Error: Expected struct expression node\n");
    return NULL;
  }

  const char *struct_name = node->expr.struct_expr.name;
  char **field_names = node->expr.struct_expr.field_names;
  AstNode **field_values = node->expr.struct_expr.field_value;
  size_t field_count = node->expr.struct_expr.field_count;

  StructInfo *struct_info = NULL;

  if (struct_name) {
    struct_info = find_struct_type(ctx, struct_name);
    if (!struct_info) {
      fprintf(stderr, "Error: Struct type '%s' not found\n", struct_name);
      return NULL;
    }
  } else {
    struct_info = infer_struct_type_from_context(ctx, field_names, field_count);
    if (!struct_info) {
      fprintf(stderr, "Error: Could not infer struct type from field names.\n");
      return NULL;
    }
  }

  // First pass: evaluate all named field values and collect spread sources
  LLVMValueRef *llvm_field_values = (LLVMValueRef *)arena_alloc(
      ctx->arena, sizeof(LLVMValueRef) * struct_info->field_count, alignof(LLVMValueRef));

  // Initialize all to NULL (not yet set)
  for (size_t i = 0; i < struct_info->field_count; i++)
    llvm_field_values[i] = NULL;

  bool all_constant = true;

  // Process each entry in the struct literal
  for (size_t i = 0; i < field_count; i++) {
    if (is_spread_entry(field_names[i], field_values[i])) {
      // Spread expression: ...expr
      // Evaluate the spread expression
      LLVMValueRef spread_val = codegen_expr(ctx, field_values[i]->expr.spread.expr);
      if (!spread_val) {
        fprintf(stderr, "Error: Failed to evaluate spread expression in struct literal\n");
        return NULL;
      }

      // Determine the source struct type
      LLVMTypeRef spread_type = LLVMTypeOf(spread_val);
      StructInfo *source_info = NULL;
      for (StructInfo *info = ctx->struct_types; info; info = info->next) {
        if (info->llvm_type == spread_type) {
          source_info = info;
          break;
        }
      }
      if (!source_info) {
        const char *tn = LLVMGetStructName(spread_type);
        if (tn) source_info = find_struct_type(ctx, tn);
      }
      if (!source_info) {
        fprintf(stderr, "Error: Cannot determine struct type for spread\n");
        return NULL;
      }

      // Alloca the spread value so we can GEP into it
      LLVMValueRef spread_alloca = alloca_and_store(ctx, spread_type, spread_val, "spread_tmp");

      // Copy each field from source to target by name
      for (size_t j = 0; j < source_info->field_count; j++) {
        int target_idx = get_field_index(struct_info, source_info->field_names[j]);
        if (target_idx < 0) {
          fprintf(stderr, "Error: Field '%s' from spread doesn't exist in target\n",
                  source_info->field_names[j]);
          return NULL;
        }
        // Only set if not already provided (named fields override spread)
        if (!llvm_field_values[target_idx]) {
          LLVMValueRef src_field_ptr = LLVMBuildStructGEP2(
              ctx->builder, source_info->llvm_type, spread_alloca, j, "spread_field_ptr");
          LLVMValueRef src_val = LLVMBuildLoad2(
              ctx->builder, source_info->field_types[j], src_field_ptr, "spread_field_val");
          llvm_field_values[target_idx] = src_val;
          if (all_constant && !LLVMIsConstant(src_val))
            all_constant = false;
        }
      }

      if (all_constant && !LLVMIsConstant(spread_val))
        all_constant = false;
    } else {
      // Named field: field_name: value
      int field_idx = get_field_index(struct_info, field_names[i]);
      if (field_idx < 0) {
        fprintf(stderr, "Error: Field '%s' not found in struct '%s'\n",
                field_names[i], struct_info->name);
        return NULL;
      }

      LLVMValueRef val = codegen_expr(ctx, field_values[i]);
      if (!val) {
        fprintf(stderr, "Error: Failed to generate value for field '%s'\n",
                field_names[i]);
        return NULL;
      }

      // Type conversion if needed
      LLVMTypeRef expected_type = struct_info->field_types[field_idx];
      LLVMTypeRef actual_type = LLVMTypeOf(val);
      if (expected_type != actual_type) {
        LLVMTypeKind expected_kind = LLVMGetTypeKind(expected_type);
        LLVMTypeKind actual_kind = LLVMGetTypeKind(actual_type);

        if (expected_kind == LLVMIntegerTypeKind && actual_kind == LLVMIntegerTypeKind) {
          unsigned eb = LLVMGetIntTypeWidth(expected_type);
          unsigned ab = LLVMGetIntTypeWidth(actual_type);
          if (eb > ab)
            val = LLVMBuildSExt(ctx->builder, val, expected_type, "sext_field");
          else if (eb < ab)
            val = LLVMBuildTrunc(ctx->builder, val, expected_type, "trunc_field");
        } else if (expected_kind == LLVMDoubleTypeKind && actual_kind == LLVMFloatTypeKind)
          val = LLVMBuildFPExt(ctx->builder, val, expected_type, "fpext_field");
        else if (expected_kind == LLVMFloatTypeKind && actual_kind == LLVMDoubleTypeKind)
          val = LLVMBuildFPTrunc(ctx->builder, val, expected_type, "fptrunc_field");
        else if ((expected_kind == LLVMFloatTypeKind || expected_kind == LLVMDoubleTypeKind) &&
                  actual_kind == LLVMIntegerTypeKind)
          val = LLVMBuildSIToFP(ctx->builder, val, expected_type, "sitofp_field");
        else if (expected_kind == LLVMIntegerTypeKind &&
                  (actual_kind == LLVMFloatTypeKind || actual_kind == LLVMDoubleTypeKind))
          val = LLVMBuildFPToSI(ctx->builder, val, expected_type, "fptosi_field");
        else {
          fprintf(stderr, "Error: Type mismatch for field '%s' in struct '%s'\n",
                  field_names[i], struct_info->name);
          return NULL;
        }
      }

      llvm_field_values[field_idx] = val;
      if (all_constant && !LLVMIsConstant(val))
        all_constant = false;
    }
  }

  // Check that all fields are filled
  for (size_t i = 0; i < struct_info->field_count; i++) {
    if (!llvm_field_values[i]) {
      fprintf(stderr, "Error: Missing field '%s' in struct initialization for '%s'\n",
              struct_info->field_names[i], struct_info->name);
      return NULL;
    }
  }

  // Create the struct value
  if (all_constant) {
    return LLVMConstNamedStruct(struct_info->llvm_type, llvm_field_values,
                                struct_info->field_count);
  } else {
    LLVMValueRef struct_alloca =
        LLVMBuildAlloca(ctx->builder, struct_info->llvm_type, "struct_literal");
    for (size_t i = 0; i < struct_info->field_count; i++) {
      LLVMValueRef field_ptr = LLVMBuildStructGEP2(
          ctx->builder, struct_info->llvm_type, struct_alloca, i, "field_ptr");
      LLVMBuildStore(ctx->builder, llvm_field_values[i], field_ptr);
    }
    return LLVMBuildLoad2(ctx->builder, struct_info->llvm_type, struct_alloca, "struct_val");
  }
}
