#include "../llvm.h"

// Initialize common types cache
void init_type_cache(CodeGenContext *ctx) {
  CommonTypes *types = &ctx->common_types;

  // Integer types
  types->i1 = LLVMInt1TypeInContext(ctx->context);
  types->i8 = LLVMInt8TypeInContext(ctx->context);
  types->i16 = LLVMInt16TypeInContext(ctx->context);
  types->i32 = LLVMInt32TypeInContext(ctx->context);
  types->i64 = LLVMInt64TypeInContext(ctx->context);

  // Float types
  types->f32 = LLVMFloatTypeInContext(ctx->context);
  types->f64 = LLVMDoubleTypeInContext(ctx->context);

  // Special types
  types->void_type = LLVMVoidTypeInContext(ctx->context);
  types->i8_ptr = LLVMPointerType(types->i8, 0);

  // Common constants
  types->const_i32_0 = LLVMConstInt(types->i32, 0, false);
  types->const_i32_1 = LLVMConstInt(types->i32, 1, false);
  types->const_i64_0 = LLVMConstInt(types->i64, 0, false);
  types->const_i64_1 = LLVMConstInt(types->i64, 1, false);
}

// Get common type by kind and size
LLVMTypeRef get_int_type(CodeGenContext *ctx, unsigned bits) {
  switch (bits) {
  case 1:
    return ctx->common_types.i1;
  case 8:
    return ctx->common_types.i8;
  case 16:
    return ctx->common_types.i16;
  case 32:
    return ctx->common_types.i32;
  case 64:
    return ctx->common_types.i64;
  default:
    return LLVMIntTypeInContext(ctx->context, bits);
  }
}

LLVMTypeRef get_float_type(CodeGenContext *ctx, bool is_double) {
  return is_double ? ctx->common_types.f64 : ctx->common_types.f32;
}

// Get common constant
LLVMValueRef get_const_int(CodeGenContext *ctx, unsigned bits, uint64_t value) {
  if (bits == 32) {
    if (value == 0)
      return ctx->common_types.const_i32_0;
    if (value == 1)
      return ctx->common_types.const_i32_1;
  } else if (bits == 64) {
    if (value == 0)
      return ctx->common_types.const_i64_0;
    if (value == 1)
      return ctx->common_types.const_i64_1;
  }
  return LLVMConstInt(get_int_type(ctx, bits), value, false);
}

// Type comparison helpers
bool is_int_type(LLVMTypeRef type) {
  return LLVMGetTypeKind(type) == LLVMIntegerTypeKind;
}

bool is_float_type(LLVMTypeRef type) {
  LLVMTypeKind kind = LLVMGetTypeKind(type);
  return kind == LLVMFloatTypeKind || kind == LLVMDoubleTypeKind;
}

bool pointer_type(LLVMTypeRef type) {
  return LLVMGetTypeKind(type) == LLVMPointerTypeKind;
}

bool types_are_equal(LLVMTypeRef a, LLVMTypeRef b) { return a == b; }

// Check if conversion is needed
bool needs_conversion(LLVMTypeRef from, LLVMTypeRef to) {
  if (from == to)
    return false;

  LLVMTypeKind from_kind = LLVMGetTypeKind(from);
  LLVMTypeKind to_kind = LLVMGetTypeKind(to);

  if (from_kind != to_kind)
    return true;

  if (from_kind == LLVMIntegerTypeKind) {
    return LLVMGetIntTypeWidth(from) != LLVMGetIntTypeWidth(to);
  }

  return false;
}
