#include "../llvm.h"

// Forward declarations
static LLVMValueRef codegen_arithmetic_op(CodeGenContext *ctx, BinaryOp op,
                                          LLVMValueRef left, LLVMValueRef right,
                                          bool is_float);
static LLVMValueRef codegen_comparison_op(CodeGenContext *ctx, BinaryOp op,
                                          LLVMValueRef left, LLVMValueRef right,
                                          bool is_float);
static LLVMValueRef codegen_logical_op(CodeGenContext *ctx, BinaryOp op,
                                       LLVMValueRef left, LLVMValueRef right,
                                       bool is_float);
static LLVMValueRef codegen_bitwise_op(CodeGenContext *ctx, BinaryOp op,
                                       LLVMValueRef left, LLVMValueRef right,
                                       bool is_float);
static void promote_operands(CodeGenContext *ctx, LLVMValueRef *left, LLVMValueRef *right,
                             LLVMTypeRef *left_type, LLVMTypeRef *right_type);

// Main entry point - routes to specialized handlers
LLVMValueRef codegen_expr_binary(CodeGenContext *ctx, AstNode *node) {
    LLVMValueRef left = codegen_expr(ctx, node->expr.binary.left);
    LLVMValueRef right = codegen_expr(ctx, node->expr.binary.right);

    if (!left || !right) return NULL;

    LLVMTypeRef left_type = LLVMTypeOf(left);
    LLVMTypeRef right_type = LLVMTypeOf(right);
    
    bool is_float_op = is_float_type(left_type) || is_float_type(right_type);

    // Promote types if needed
    if (is_float_op) {
        promote_operands(ctx, &left, &right, &left_type, &right_type);
    }

    BinaryOp op = node->expr.binary.op;

    // Route to appropriate handler
    if (op >= BINOP_ADD && op <= BINOP_MOD) {
        return codegen_arithmetic_op(ctx, op, left, right, is_float_op);
    }
    if (op >= BINOP_EQ && op <= BINOP_GE) {
        return codegen_comparison_op(ctx, op, left, right, is_float_op);
    }
    if (op == BINOP_AND || op == BINOP_OR) {
        return codegen_logical_op(ctx, op, left, right, is_float_op);
    }
    if (op >= BINOP_BIT_AND && op <= BINOP_SHR) {
        return codegen_bitwise_op(ctx, op, left, right, is_float_op);
    }
    if (op == BINOP_RANGE) {
        return create_range_struct(ctx, left, right);
    }

    return NULL;
}

// Type promotion logic
static void promote_operands(CodeGenContext *ctx, LLVMValueRef *left, LLVMValueRef *right,
                             LLVMTypeRef *left_type, LLVMTypeRef *right_type) {
    LLVMTypeKind left_kind = LLVMGetTypeKind(*left_type);
    LLVMTypeKind right_kind = LLVMGetTypeKind(*right_type);

    // Integer to float promotion
    if (is_int_type(*left_type) && is_float_type(*right_type)) {
        *left = LLVMBuildSIToFP(ctx->builder, *left, *right_type, "int_to_float");
        *left_type = *right_type;
    } else if (is_int_type(*right_type) && is_float_type(*left_type)) {
        *right = LLVMBuildSIToFP(ctx->builder, *right, *left_type, "int_to_float");
        *right_type = *left_type;
    }

    // Float to double promotion
    if (left_kind == LLVMFloatTypeKind && right_kind == LLVMDoubleTypeKind) {
        *left = LLVMBuildFPExt(ctx->builder, *left, *right_type, "float_to_double");
    } else if (right_kind == LLVMFloatTypeKind && left_kind == LLVMDoubleTypeKind) {
        *right = LLVMBuildFPExt(ctx->builder, *right, *left_type, "float_to_double");
    }
}

// Arithmetic operations: +, -, *, /, %
static LLVMValueRef codegen_arithmetic_op(CodeGenContext *ctx, BinaryOp op,
                                          LLVMValueRef left, LLVMValueRef right,
                                          bool is_float) {
    switch (op) {
    case BINOP_ADD:
        return is_float ? LLVMBuildFAdd(ctx->builder, left, right, "fadd")
                        : LLVMBuildAdd(ctx->builder, left, right, "add");

    case BINOP_SUB:
        return is_float ? LLVMBuildFSub(ctx->builder, left, right, "fsub")
                        : LLVMBuildSub(ctx->builder, left, right, "sub");

    case BINOP_MUL:
        return is_float ? LLVMBuildFMul(ctx->builder, left, right, "fmul")
                        : LLVMBuildMul(ctx->builder, left, right, "mul");

    case BINOP_DIV:
        return is_float ? LLVMBuildFDiv(ctx->builder, left, right, "fdiv")
                        : LLVMBuildSDiv(ctx->builder, left, right, "div");

    case BINOP_MOD:
        if (is_float) {
            // Floating point modulo: a - (b * floor(a/b))
            LLVMModuleRef current_module =
                ctx->current_module ? ctx->current_module->module : ctx->module;

            LLVMTypeRef left_type = LLVMTypeOf(left);
            LLVMTypeRef floor_type = LLVMFunctionType(left_type, &left_type, 1, false);
            
            const char *floor_name = (LLVMGetTypeKind(left_type) == LLVMDoubleTypeKind)
                                     ? "llvm.floor.f64" : "llvm.floor.f32";
            
            LLVMValueRef floor_func = LLVMGetNamedFunction(current_module, floor_name);
            if (!floor_func) {
                floor_func = LLVMAddFunction(current_module, floor_name, floor_type);
            }

            LLVMValueRef division = LLVMBuildFDiv(ctx->builder, left, right, "fdiv_for_mod");
            LLVMValueRef floor_result = LLVMBuildCall2(ctx->builder, floor_type, 
                                                       floor_func, &division, 1, "floor_result");
            LLVMValueRef multiply = LLVMBuildFMul(ctx->builder, right, floor_result, "fmul_for_mod");
            return LLVMBuildFSub(ctx->builder, left, multiply, "fmod_result");
        } else {
            return LLVMBuildSRem(ctx->builder, left, right, "mod");
        }

    default:
        return NULL;
    }
}

// Comparison operations: ==, !=, <, <=, >, >=
static LLVMValueRef codegen_comparison_op(CodeGenContext *ctx, BinaryOp op,
                                          LLVMValueRef left, LLVMValueRef right,
                                          bool is_float) {
    if (is_float) {
        LLVMRealPredicate pred;
        switch (op) {
        case BINOP_EQ: pred = LLVMRealOEQ; break;
        case BINOP_NE: pred = LLVMRealONE; break;
        case BINOP_LT: pred = LLVMRealOLT; break;
        case BINOP_LE: pred = LLVMRealOLE; break;
        case BINOP_GT: pred = LLVMRealOGT; break;
        case BINOP_GE: pred = LLVMRealOGE; break;
        default: return NULL;
        }
        return LLVMBuildFCmp(ctx->builder, pred, left, right, "fcmp");
    } else {
        LLVMIntPredicate pred;
        switch (op) {
        case BINOP_EQ: pred = LLVMIntEQ; break;
        case BINOP_NE: pred = LLVMIntNE; break;
        case BINOP_LT: pred = LLVMIntSLT; break;
        case BINOP_LE: pred = LLVMIntSLE; break;
        case BINOP_GT: pred = LLVMIntSGT; break;
        case BINOP_GE: pred = LLVMIntSGE; break;
        default: return NULL;
        }
        return LLVMBuildICmp(ctx->builder, pred, left, right, "icmp");
    }
}

// Logical operations: &&, ||
static LLVMValueRef codegen_logical_op(CodeGenContext *ctx, BinaryOp op,
                                       LLVMValueRef left, LLVMValueRef right,
                                       bool is_float) {
    if (is_float) {
        fprintf(stderr, "Error: Logical operations not supported for floating point\n");
        return NULL;
    }

    switch (op) {
    case BINOP_AND:
        return LLVMBuildAnd(ctx->builder, left, right, "and");
    case BINOP_OR:
        return LLVMBuildOr(ctx->builder, left, right, "or");
    default:
        return NULL;
    }
}

// Bitwise operations: &, |, ^, <<, >>
static LLVMValueRef codegen_bitwise_op(CodeGenContext *ctx, BinaryOp op,
                                       LLVMValueRef left, LLVMValueRef right,
                                       bool is_float) {
    if (is_float) {
        fprintf(stderr, "Error: Bitwise operations not supported for floating point\n");
        return NULL;
    }

    switch (op) {
    case BINOP_BIT_AND:
        return LLVMBuildAnd(ctx->builder, left, right, "bitand");
    case BINOP_BIT_OR:
        return LLVMBuildOr(ctx->builder, left, right, "bitor");
    case BINOP_BIT_XOR:
        return LLVMBuildXor(ctx->builder, left, right, "bitxor");
    case BINOP_SHL:
        return LLVMBuildShl(ctx->builder, left, right, "shl");
    case BINOP_SHR:
        return LLVMBuildAShr(ctx->builder, left, right, "ashr");
    default:
        return NULL;
    }
}