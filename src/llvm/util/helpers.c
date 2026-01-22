#include "../llvm.h"

// Alloca + Store pattern
LLVMValueRef alloca_and_store(CodeGenContext *ctx, LLVMTypeRef type,
                              LLVMValueRef value, const char *name) {
  LLVMValueRef alloca = LLVMBuildAlloca(ctx->builder, type, name);
  LLVMBuildStore(ctx->builder, value, alloca);
  return alloca;
}

// GEP + Load pattern for structs
LLVMValueRef struct_gep_load(CodeGenContext *ctx, LLVMTypeRef struct_type,
                             LLVMValueRef ptr, unsigned index,
                             LLVMTypeRef element_type, const char *name) {
  LLVMValueRef gep =
      LLVMBuildStructGEP2(ctx->builder, struct_type, ptr, index, "tmp_gep");
  return LLVMBuildLoad2(ctx->builder, element_type, gep, name);
}

// GEP + Store pattern for structs
void struct_gep_store(CodeGenContext *ctx, LLVMTypeRef struct_type,
                      LLVMValueRef ptr, unsigned index, LLVMValueRef value) {
  LLVMValueRef gep =
      LLVMBuildStructGEP2(ctx->builder, struct_type, ptr, index, "tmp_gep");
  LLVMBuildStore(ctx->builder, value, gep);
}

// Array GEP helper
LLVMValueRef array_gep(CodeGenContext *ctx, LLVMTypeRef array_type,
                       LLVMValueRef array_ptr, LLVMValueRef index,
                       const char *name) {
  LLVMValueRef indices[2] = {ctx->common_types.const_i32_0, index};
  return LLVMBuildGEP2(ctx->builder, array_type, array_ptr, indices, 2, name);
}

// Check if block has terminator (cached to avoid repeated calls)
bool block_has_terminator(LLVMBuilderRef builder) {
  LLVMBasicBlockRef block = LLVMGetInsertBlock(builder);
  return LLVMGetBasicBlockTerminator(block) != NULL;
}

// Create branch if no terminator exists
void branch_if_no_terminator(CodeGenContext *ctx, LLVMBasicBlockRef target) {
  if (!block_has_terminator(ctx->builder)) {
    LLVMBuildBr(ctx->builder, target);
  }
}

// Global string helper (cached to avoid duplicates)
static LLVMValueRef cached_strings[256];
static const char *cached_string_values[256];
static size_t cached_string_count = 0;

LLVMValueRef build_global_string(CodeGenContext *ctx, const char *str,
                                 const char *name) {
  // Check cache first
  for (size_t i = 0; i < cached_string_count; i++) {
    if (strcmp(cached_string_values[i], str) == 0) {
      return cached_strings[i];
    }
  }

  LLVMValueRef global_str = LLVMBuildGlobalStringPtr(ctx->builder, str, name);

  // Cache if space available
  if (cached_string_count < 256) {
    cached_strings[cached_string_count] = global_str;
    cached_string_values[cached_string_count] = str;
    cached_string_count++;
  }

  return global_str;
}
