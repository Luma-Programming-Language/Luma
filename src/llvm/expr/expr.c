#include <stdio.h>
#include <stdlib.h>

#include "../llvm.h"

// Add these utility functions to help with consistent range creation and type
// management You can put these in expr.c or create a new ranges.c file

LLVMTypeRef get_range_struct_type(CodeGenContext *ctx,
                                  LLVMTypeRef element_type) {
  // Create or reuse a range struct type for the given element type
  // This ensures all ranges with the same element type use the same struct
  // layout
  LLVMTypeRef field_types[] = {element_type, element_type};
  return LLVMStructTypeInContext(ctx->context, field_types, 2, false);
}

LLVMValueRef create_range_struct(CodeGenContext *ctx, LLVMValueRef start,
                                 LLVMValueRef end) {
  // Helper function to create a range struct consistently
  // This can be used in your BINOP_RANGE case and elsewhere

  LLVMTypeRef element_type =
      LLVMTypeOf(start); // Assume both operands have same type
  LLVMTypeRef range_struct_type = get_range_struct_type(ctx, element_type);

  // Allocate space for the range struct
  LLVMValueRef range_alloca =
      LLVMBuildAlloca(ctx->builder, range_struct_type, "range");

  // Store the start value
  LLVMValueRef start_ptr = LLVMBuildStructGEP2(ctx->builder, range_struct_type,
                                               range_alloca, 0, "start_ptr");
  LLVMBuildStore(ctx->builder, start, start_ptr);

  // Store the end value
  LLVMValueRef end_ptr = LLVMBuildStructGEP2(ctx->builder, range_struct_type,
                                             range_alloca, 1, "end_ptr");
  LLVMBuildStore(ctx->builder, end, end_ptr);

  // Return the struct value
  return LLVMBuildLoad2(ctx->builder, range_struct_type, range_alloca,
                        "range_val");
}

// Optional: Add these functions for future range operations
LLVMValueRef range_contains(CodeGenContext *ctx, LLVMValueRef range_struct,
                            LLVMValueRef value) {
  LLVMValueRef start = get_range_start_value(ctx, range_struct);
  LLVMValueRef end = get_range_end_value(ctx, range_struct);

  // Check if value >= start && value <= end
  LLVMValueRef ge_start =
      LLVMBuildICmp(ctx->builder, LLVMIntSGE, value, start, "ge_start");
  LLVMValueRef le_end =
      LLVMBuildICmp(ctx->builder, LLVMIntSLE, value, end, "le_end");

  return LLVMBuildAnd(ctx->builder, ge_start, le_end, "in_range");
}

LLVMValueRef range_length(CodeGenContext *ctx, LLVMValueRef range_struct) {
  LLVMValueRef start = get_range_start_value(ctx, range_struct);
  LLVMValueRef end = get_range_end_value(ctx, range_struct);

  // Calculate end - start + 1 (inclusive range length)
  LLVMValueRef diff = LLVMBuildSub(ctx->builder, end, start, "diff");
  LLVMValueRef one = LLVMConstInt(LLVMTypeOf(diff), 1, false);
  return LLVMBuildAdd(ctx->builder, diff, one, "range_length");
}

LLVMValueRef codegen_expr_literal(CodeGenContext *ctx, AstNode *node) {
  switch (node->expr.literal.lit_type) {
  case LITERAL_INT:
    return LLVMConstInt(ctx->common_types.i64, node->expr.literal.value.int_val,
                        false);

  case LITERAL_FLOAT:
    return LLVMConstReal(ctx->common_types.f64,
                         node->expr.literal.value.float_val);

  case LITERAL_BOOL:
    return node->expr.literal.value.bool_val
               ? LLVMConstInt(ctx->common_types.i1, 1, false)
               : LLVMConstInt(ctx->common_types.i1, 0, false);

  case LITERAL_CHAR:
    return LLVMConstInt(ctx->common_types.i8,
                        (unsigned char)node->expr.literal.value.char_val,
                        false);

  case LITERAL_STRING: {
    char *processed_str =
        process_escape_sequences(node->expr.literal.value.string_val);
    LLVMModuleRef current_llvm_module =
        ctx->current_module ? ctx->current_module->module : ctx->module;

    // CHANGED: Use cached i8 type
    LLVMValueRef global_str = LLVMAddGlobal(
        current_llvm_module,
        LLVMArrayType(ctx->common_types.i8, strlen(processed_str) + 1), "str");

    LLVMSetInitializer(global_str,
                       LLVMConstStringInContext(ctx->context, processed_str,
                                                strlen(processed_str), 0));
    LLVMSetLinkage(global_str, LLVMPrivateLinkage);
    LLVMSetGlobalConstant(global_str, 1);
    LLVMSetUnnamedAddr(global_str, 1);

    LLVMValueRef indices[2] = {ctx->common_types.const_i32_0,
                               ctx->common_types.const_i32_0};

    LLVMValueRef result = LLVMConstGEP2(
        LLVMArrayType(ctx->common_types.i8, strlen(processed_str) + 1),
        global_str, indices, 2);

    free(processed_str);
    return result;
  }

  case LITERAL_NULL:
    return LLVMConstNull(ctx->common_types.i8_ptr);

  default:
    fprintf(stderr, "ERROR: Unknown literal type: %d\n",
            node->expr.literal.lit_type);
    return NULL;
  }
}

LLVMValueRef codegen_expr_identifier(CodeGenContext *ctx, AstNode *node) {
  const char *name = node->expr.identifier.name;

  LLVM_Symbol *sym = find_symbol(ctx, name);
  if (sym) {
    if (sym->is_function) {
      return sym->value;
    } else if (is_enum_constant(sym)) {
      // Enum constant - return the constant value directly
      return LLVMGetInitializer(sym->value);
    } else {
      // Load variable value
      return LLVMBuildLoad2(ctx->builder, sym->type, sym->value, "load");
    }
  }

  fprintf(stderr, "Error: Undefined symbol '%s'\n", name);
  return NULL;
}

LLVMValueRef codegen_expr_unary(CodeGenContext *ctx, AstNode *node) {
  LLVMValueRef operand = codegen_expr(ctx, node->expr.unary.operand);
  if (!operand)
    return NULL;

  LLVMTypeRef operand_type = LLVMTypeOf(operand);
  LLVMTypeKind operand_kind = LLVMGetTypeKind(operand_type);
  bool is_float =
      (operand_kind == LLVMFloatTypeKind || operand_kind == LLVMDoubleTypeKind);

  switch (node->expr.unary.op) {
  case UNOP_NEG:
    if (is_float) {
      return LLVMBuildFNeg(ctx->builder, operand, "fneg");
    } else {
      return LLVMBuildNeg(ctx->builder, operand, "neg");
    }

  case UNOP_NOT:
    if (is_float) {
      fprintf(stderr,
              "Error: Logical NOT not supported for floating point values\n");
      return NULL;
    }
    return LLVMBuildNot(ctx->builder, operand, "not");

  case UNOP_BIT_NOT:
    if (is_float) {
      fprintf(
          stderr,
          "Error: Bitwise NOT (~) not supported for floating point values\n");
      return NULL;
    }
    return LLVMBuildNot(ctx->builder, operand, "bitnot");

  case UNOP_PRE_INC:
  case UNOP_POST_INC: {
    if (node->expr.unary.operand->type != AST_EXPR_IDENTIFIER) {
      fprintf(stderr, "Error: Increment/decrement requires an lvalue\n");
      return NULL;
    }

    LLVM_Symbol *sym =
        find_symbol(ctx, node->expr.unary.operand->expr.identifier.name);
    if (!sym || sym->is_function) {
      fprintf(stderr, "Error: Undefined variable for increment\n");
      return NULL;
    }

    LLVMValueRef loaded_val =
        LLVMBuildLoad2(ctx->builder, sym->type, sym->value, "load");
    LLVMValueRef one;
    LLVMValueRef incremented;

    if (is_float) {
      one = LLVMConstReal(LLVMTypeOf(loaded_val), 1.0);
      incremented = LLVMBuildFAdd(ctx->builder, loaded_val, one, "finc");
    } else {
      one = LLVMConstInt(LLVMTypeOf(loaded_val), 1, false);
      incremented = LLVMBuildAdd(ctx->builder, loaded_val, one, "inc");
    }

    LLVMBuildStore(ctx->builder, incremented, sym->value);
    return (node->expr.unary.op == UNOP_PRE_INC) ? incremented : loaded_val;
  }

  case UNOP_PRE_DEC:
  case UNOP_POST_DEC: {
    if (node->expr.unary.operand->type != AST_EXPR_IDENTIFIER) {
      fprintf(stderr, "Error: Increment/decrement requires an lvalue\n");
      return NULL;
    }

    LLVM_Symbol *sym =
        find_symbol(ctx, node->expr.unary.operand->expr.identifier.name);
    if (!sym || sym->is_function) {
      fprintf(stderr, "Error: Undefined variable for decrement\n");
      return NULL;
    }

    LLVMValueRef loaded_val =
        LLVMBuildLoad2(ctx->builder, sym->type, sym->value, "load");
    LLVMValueRef one;
    LLVMValueRef decremented;

    if (is_float) {
      one = LLVMConstReal(LLVMTypeOf(loaded_val), 1.0);
      decremented = LLVMBuildFSub(ctx->builder, loaded_val, one, "fdec");
    } else {
      one = LLVMConstInt(LLVMTypeOf(loaded_val), 1, false);
      decremented = LLVMBuildSub(ctx->builder, loaded_val, one, "dec");
    }

    LLVMBuildStore(ctx->builder, decremented, sym->value);
    return (node->expr.unary.op == UNOP_PRE_DEC) ? decremented : loaded_val;
  }

  default:
    return NULL;
  }
}

LLVMValueRef codegen_expr_call(CodeGenContext *ctx, AstNode *node) {
  AstNode *callee = node->expr.call.callee;
  LLVMValueRef callee_value = NULL;
  LLVMValueRef *args = NULL;
  size_t arg_count = node->expr.call.arg_count;

  // Check if this is a method call (obj.method())
  if (callee->type == AST_EXPR_MEMBER && !callee->expr.member.is_compiletime) {
    // Method call: obj.method(arg1, arg2)
    const char *member_name = callee->expr.member.member;

    LLVMValueRef method_func = NULL;

    // First try current module
    LLVMModuleRef current_llvm_module =
        ctx->current_module ? ctx->current_module->module : ctx->module;
    method_func = LLVMGetNamedFunction(current_llvm_module, member_name);

    // If not found in current module, search all other modules
    if (!method_func) {
      for (ModuleCompilationUnit *unit = ctx->modules; unit;
           unit = unit->next) {
        if (unit == ctx->current_module)
          continue;

        LLVMValueRef found_func =
            LLVMGetNamedFunction(unit->module, member_name);
        if (found_func) {
          // CRITICAL FIX: Create external declaration in current module
          LLVMTypeRef func_type = LLVMGlobalGetValueType(found_func);
          method_func =
              LLVMAddFunction(current_llvm_module, member_name, func_type);
          LLVMSetLinkage(method_func, LLVMExternalLinkage);
          break;
        }
      }
    }

    if (!method_func) {
      fprintf(stderr, "Error: Method '%s' not found in any module\n",
              member_name);
      return NULL;
    }

    callee_value = method_func;

    // Allocate space for arguments (typechecker already added self!)
    args = (LLVMValueRef *)arena_alloc(
        ctx->arena, sizeof(LLVMValueRef) * arg_count, alignof(LLVMValueRef));

    // Generate all arguments (including self at index 0)
    for (size_t i = 0; i < arg_count; i++) {
      args[i] = codegen_expr(ctx, node->expr.call.args[i]);
      if (!args[i]) {
        fprintf(stderr,
                "Error: Failed to generate argument %zu for method '%s'\n", i,
                member_name);
        return NULL;
      }
    }
  } else {
    // Regular function call or compile-time member access (module::func)
    callee_value = codegen_expr(ctx, callee);
    if (!callee_value) {
      return NULL;
    }

    // Allocate space for arguments
    args = (LLVMValueRef *)arena_alloc(
        ctx->arena, sizeof(LLVMValueRef) * arg_count, alignof(LLVMValueRef));

    for (size_t i = 0; i < arg_count; i++) {
      args[i] = codegen_expr(ctx, node->expr.call.args[i]);
      if (!args[i]) {
        return NULL;
      }
    }
  }

  // CRITICAL: Validate callee_value before proceeding
  if (!callee_value) {
    fprintf(stderr, "Error: callee_value is NULL in codegen_expr_call\n");
    return NULL;
  }

  // CRITICAL: Check if callee_value is actually a function
  if (!LLVMIsAFunction(callee_value)) {
    fprintf(stderr, "Error: callee_value is not a function\n");
    LLVMDumpValue(callee_value);
    return NULL;
  }

  // Get the function type to check return type
  LLVMTypeRef func_type = LLVMGlobalGetValueType(callee_value);
  if (!func_type) {
    fprintf(stderr, "Error: Failed to get function type\n");
    return NULL;
  }

  LLVMTypeRef return_type = LLVMGetReturnType(func_type);
  if (!return_type) {
    fprintf(stderr, "Error: Failed to get return type\n");
    return NULL;
  }

  // Check if return type is void
  if (LLVMGetTypeKind(return_type) == LLVMVoidTypeKind) {
    LLVMBuildCall2(ctx->builder, func_type, callee_value, args, arg_count, "");
    return LLVMConstNull(LLVMVoidTypeInContext(ctx->context));
  }

  // CRITICAL: For struct returns, we need special handling
  LLVMTypeKind return_kind = LLVMGetTypeKind(return_type);
  if (return_kind == LLVMStructTypeKind) {
    // Check if this is a cross-module call
    LLVMModuleRef callee_module = LLVMGetGlobalParent(callee_value);
    LLVMModuleRef current_llvm_module =
        ctx->current_module ? ctx->current_module->module : ctx->module;

    bool is_cross_module = (callee_module != current_llvm_module);

    if (is_cross_module) {
      // For cross-module struct returns, ensure we have a proper external
      // declaration
      const char *func_name = LLVMGetValueName(callee_value);

      // Check if we already have a declaration in current module
      LLVMValueRef local_func =
          LLVMGetNamedFunction(current_llvm_module, func_name);

      if (!local_func) {
        // Create external declaration
        local_func = LLVMAddFunction(current_llvm_module, func_name, func_type);
        LLVMSetLinkage(local_func, LLVMExternalLinkage);
      }

      // CRITICAL: Ensure both functions have the same calling convention
      LLVMCallConv source_cc = LLVMGetFunctionCallConv(callee_value);
      LLVMSetFunctionCallConv(local_func, source_cc);

      // Use local declaration for the call
      callee_value = local_func;
    }

    // For struct returns, allocate space and load the result
    // This ensures proper struct return handling regardless of ABI
    LLVMValueRef call_result = LLVMBuildCall2(
        ctx->builder, func_type, callee_value, args, arg_count, "struct_call");

    // The struct is returned by value - just return it
    return call_result;
  }

  // Regular (non-struct, non-void) return
  return LLVMBuildCall2(ctx->builder, func_type, callee_value, args, arg_count,
                        "call");
}

// Unified assignment handler that supports all assignment types
LLVMValueRef codegen_expr_assignment(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_ASSIGNMENT) {
    return NULL;
  }

  LLVMValueRef value = codegen_expr(ctx, node->expr.assignment.value);
  if (!value) {
    return NULL;
  }

  AstNode *target = node->expr.assignment.target;

  // Handle direct variable assignment: x = value
  if (target->type == AST_EXPR_IDENTIFIER) {
    LLVM_Symbol *sym = find_symbol(ctx, target->expr.identifier.name);
    if (sym && !sym->is_function) {
      // NEW: If assigning a cast expression, update element type
      if (node->expr.assignment.value->type == AST_EXPR_CAST) {
        AstNode *cast_node = node->expr.assignment.value;
        LLVMTypeRef new_element_type =
            extract_element_type_from_ast(ctx, cast_node->expr.cast.type);
        if (new_element_type) {
          sym->element_type = new_element_type;
        }
      }

      LLVMBuildStore(ctx->builder, value, sym->value);
      return value;
    }
    fprintf(stderr, "Error: Variable %s not found\n",
            target->expr.identifier.name);
    return NULL;
  }

  // Handle pointer dereference assignment: *ptr = value
  else if (target->type == AST_EXPR_DEREF) {
    LLVMValueRef ptr = codegen_expr(ctx, target->expr.deref.object);
    if (!ptr) {
      return NULL;
    }
    LLVMBuildStore(ctx->builder, value, ptr);
    return value;
  }

  // Handle index assignment: arr[i] = value or ptr[i] = value
  else if (target->type == AST_EXPR_INDEX) {
    // Generate the object being indexed
    LLVMValueRef object = codegen_expr(ctx, target->expr.index.object);
    if (!object) {
      return NULL;
    }

    // Generate the index expression
    LLVMValueRef index = codegen_expr(ctx, target->expr.index.index);
    if (!index) {
      return NULL;
    }

    LLVMTypeRef object_type = LLVMTypeOf(object);
    LLVMTypeKind object_kind = LLVMGetTypeKind(object_type);

    if (object_kind == LLVMArrayTypeKind) {
      // Array assignment: arr[i] = value
      LLVMValueRef array_ptr;

      if (target->expr.index.object->type == AST_EXPR_IDENTIFIER) {
        // Get the symbol directly for proper array access
        const char *var_name = target->expr.index.object->expr.identifier.name;
        LLVM_Symbol *sym = find_symbol(ctx, var_name);
        if (sym && !sym->is_function) {
          array_ptr = sym->value;
        } else {
          fprintf(stderr, "Error: Array variable %s not found for assignment\n",
                  var_name);
          return NULL;
        }
      } else {
        // Create temporary storage for complex array expressions
        array_ptr =
            LLVMBuildAlloca(ctx->builder, object_type, "temp_array_ptr");
        LLVMBuildStore(ctx->builder, object, array_ptr);
      }

      LLVMValueRef indices[2];
      indices[0] = LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false);
      indices[1] = index;

      LLVMValueRef element_ptr = LLVMBuildGEP2(
          ctx->builder, object_type, array_ptr, indices, 2, "array_assign_ptr");

      // Type conversion if needed
      LLVMTypeRef element_type = LLVMGetElementType(object_type);
      LLVMTypeRef value_type = LLVMTypeOf(value);

      if (element_type != value_type) {
        value = convert_value_to_type(ctx, value, value_type, element_type);
        if (!value) {
          fprintf(stderr,
                  "Error: Cannot convert value to array element type\n");
          return NULL;
        }
      }

      LLVMBuildStore(ctx->builder, value, element_ptr);
      return value;

    } else if (object_kind == LLVMPointerTypeKind) {
      // Pointer indexing: ptr[i] = value
      LLVMTypeRef element_type = NULL;
      LLVMTypeRef value_type = LLVMTypeOf(value);

      // Strategy 1: Try to get element type from symbol table
      if (target->expr.index.object->type == AST_EXPR_IDENTIFIER) {
        const char *var_name = target->expr.index.object->expr.identifier.name;
        LLVM_Symbol *sym = find_symbol(ctx, var_name);

        if (sym && !sym->is_function && sym->element_type) {
          element_type = sym->element_type;
        }
      }

      // Strategy 2: Extract from cast expression
      if (!element_type && target->expr.index.object->type == AST_EXPR_CAST) {
        AstNode *cast_node = target->expr.index.object;
        if (cast_node->expr.cast.type->type == AST_TYPE_POINTER) {
          element_type = codegen_type(
              ctx, cast_node->expr.cast.type->type_data.pointer.pointee_type);
        }
      }

      // NEW: Check if element_type is a struct - this is likely an error
      if (element_type && LLVMGetTypeKind(element_type) == LLVMStructTypeKind) {
        if (LLVMGetTypeKind(value_type) != LLVMStructTypeKind) {
          const char *var_name =
              target->expr.index.object->type == AST_EXPR_IDENTIFIER
                  ? target->expr.index.object->expr.identifier.name
                  : "pointer";
          fprintf(
              stderr,
              "Error: Cannot assign scalar value to struct pointer element.\n"
              "  Variable '%s' is a pointer to struct, not an array of "
              "values.\n"
              "  Did you mean to use a different pointer variable?\n",
              var_name);
          return NULL;
        }
      }

      // Strategy 3: Infer from variable name (fallback for legacy code)
      if (!element_type &&
          target->expr.index.object->type == AST_EXPR_IDENTIFIER) {
        const char *var_name = target->expr.index.object->expr.identifier.name;

        if (strstr(var_name, "int") && !strstr(var_name, "char")) {
          element_type = LLVMInt64TypeInContext(ctx->context);
        } else if (strstr(var_name, "double")) {
          element_type = LLVMDoubleTypeInContext(ctx->context);
        } else if (strstr(var_name, "float")) {
          element_type = LLVMFloatTypeInContext(ctx->context);
        } else if (strstr(var_name, "char")) {
          element_type = LLVMInt8TypeInContext(ctx->context);
        }
      }

      // Final fallback: use value type
      if (!element_type) {
        element_type = value_type;
      }

      // Convert value to match element type if needed
      LLVMValueRef value_final = value;
      if (LLVMGetTypeKind(element_type) != LLVMGetTypeKind(value_type) ||
          (LLVMGetTypeKind(element_type) == LLVMIntegerTypeKind &&
           LLVMGetIntTypeWidth(element_type) !=
               LLVMGetIntTypeWidth(value_type))) {

        if (LLVMGetTypeKind(element_type) == LLVMIntegerTypeKind &&
            LLVMGetTypeKind(value_type) == LLVMIntegerTypeKind) {
          unsigned element_bits = LLVMGetIntTypeWidth(element_type);
          unsigned value_bits = LLVMGetIntTypeWidth(value_type);

          if (element_bits > value_bits) {
            value_final = LLVMBuildZExt(ctx->builder, value, element_type,
                                        "zext_for_store");
          } else if (element_bits < value_bits) {
            value_final = LLVMBuildTrunc(ctx->builder, value, element_type,
                                         "trunc_for_store");
          }
        } else if (LLVMGetTypeKind(element_type) == LLVMIntegerTypeKind &&
                   (LLVMGetTypeKind(value_type) == LLVMFloatTypeKind ||
                    LLVMGetTypeKind(value_type) == LLVMDoubleTypeKind)) {
          value_final = LLVMBuildFPToSI(ctx->builder, value, element_type,
                                        "float_to_int_for_store");
        } else if ((LLVMGetTypeKind(element_type) == LLVMFloatTypeKind ||
                    LLVMGetTypeKind(element_type) == LLVMDoubleTypeKind) &&
                   LLVMGetTypeKind(value_type) == LLVMIntegerTypeKind) {
          value_final = LLVMBuildSIToFP(ctx->builder, value, element_type,
                                        "int_to_float_for_store");
        } else {
          // NEW: Better error for incompatible types
          fprintf(stderr,
                  "Error: Cannot convert value type (kind %d) to pointer "
                  "element type (kind %d)\n",
                  LLVMGetTypeKind(value_type), LLVMGetTypeKind(element_type));
          return NULL;
        }
      }

      // Use proper typed GEP for pointer arithmetic
      LLVMValueRef element_ptr = LLVMBuildGEP2(
          ctx->builder, element_type, object, &index, 1, "ptr_assign_ptr");
      LLVMBuildStore(ctx->builder, value_final, element_ptr);
      return value;
    } else {
      fprintf(stderr, "Error: Cannot assign to index of this type (kind: %d)\n",
              object_kind);
      return NULL;
    }
  }

  // Handle struct member assignment: obj.field = value
  else if (target->type == AST_EXPR_MEMBER) {
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

      // Find the struct info by checking which struct has this field
      StructInfo *struct_info = NULL;
      for (StructInfo *info = ctx->struct_types; info; info = info->next) {
        int field_idx = get_field_index(info, field_name);
        if (field_idx >= 0) {
          struct_info = info;
          break;
        }
      }

      if (!struct_info) {
        fprintf(stderr, "Error: Could not find struct with field '%s'\n",
                field_name);
        return NULL;
      }

      // Find field index and check permissions
      int field_index = get_field_index(struct_info, field_name);
      if (!is_field_access_allowed(ctx, struct_info, field_index)) {
        fprintf(stderr, "Error: Cannot assign to private field '%s'\n",
                field_name);
        return NULL;
      }

      // Handle the different cases for assignment
      LLVMTypeRef symbol_type = sym->type;
      LLVMValueRef struct_ptr;

      if (LLVMGetTypeKind(symbol_type) == LLVMPointerTypeKind) {
        // Pointer to struct - load the pointer value
        LLVMTypeRef ptr_to_struct_type =
            LLVMPointerType(struct_info->llvm_type, 0);
        struct_ptr = LLVMBuildLoad2(ctx->builder, ptr_to_struct_type,
                                    sym->value, "load_struct_ptr");
      } else if (symbol_type == struct_info->llvm_type) {
        // Direct struct variable
        struct_ptr = sym->value;
      } else {
        fprintf(stderr,
                "Error: Variable '%s' is not a struct or pointer to struct\n",
                var_name);
        return NULL;
      }

      // Use GEP to get the field address and store the value
      LLVMValueRef field_ptr =
          LLVMBuildStructGEP2(ctx->builder, struct_info->llvm_type, struct_ptr,
                              field_index, "field_ptr");

      LLVMBuildStore(ctx->builder, value, field_ptr);
      return value;
    }
  }

  fprintf(stderr, "Error: Invalid assignment target\n");
  return NULL;
}

LLVMValueRef codegen_expr_array(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_ARRAY) {
    fprintf(stderr, "Error: Expected array expression node\n");
    return NULL;
  }

  AstNode **elements = node->expr.array.elements;
  size_t element_count = node->expr.array.element_count;
  size_t target_size =
      node->expr.array.target_size; // NEW: Get target size for padding

  if (element_count == 0) {
    fprintf(stderr, "Error: Empty array literals not supported\n");
    return NULL;
  }

  // Generate the first element to determine the array's element type
  LLVMValueRef first_element = codegen_expr(ctx, elements[0]);
  if (!first_element) {
    fprintf(stderr, "Error: Failed to generate first array element\n");
    return NULL;
  }

  LLVMTypeRef element_type = LLVMTypeOf(first_element);

  // NEW: Use target_size if set (for padding), otherwise use actual
  // element_count
  size_t actual_array_size = (target_size > 0) ? target_size : element_count;
  LLVMTypeRef array_type = LLVMArrayType(element_type, actual_array_size);

  // Check if all elements are constants
  bool all_constants = LLVMIsConstant(first_element);

  // NEW: Allocate for the FULL size (including padding)
  LLVMValueRef *element_values = (LLVMValueRef *)arena_alloc(
      ctx->arena, sizeof(LLVMValueRef) * actual_array_size,
      alignof(LLVMValueRef));

  element_values[0] = first_element;

  // Generate provided elements
  for (size_t i = 1; i < element_count; i++) {
    element_values[i] = codegen_expr(ctx, elements[i]);
    if (!element_values[i]) {
      fprintf(stderr, "Error: Failed to generate array element %zu\n", i);
      return NULL;
    }

    // Type conversion if needed
    LLVMTypeRef current_type = LLVMTypeOf(element_values[i]);
    if (current_type != element_type) {
      element_values[i] = convert_value_to_type(ctx, element_values[i],
                                                current_type, element_type);
      if (!element_values[i]) {
        fprintf(stderr,
                "Error: Cannot convert element %zu to array element type\n", i);
        return NULL;
      }
    }

    if (all_constants && !LLVMIsConstant(element_values[i])) {
      all_constants = false;
    }
  }

  // NEW: Pad remaining elements with zeros if target_size > element_count
  if (target_size > element_count) {
    LLVMValueRef zero_value = LLVMConstNull(element_type);

    for (size_t i = element_count; i < target_size; i++) {
      element_values[i] = zero_value;
    }

    // If we're padding, we can't be all constants unless zeros count
    // (which they do, so keep checking)
    // zero_value is always constant, so all_constants remains unchanged
  }

  // Check if any element references a global from another module
  for (size_t i = 0; i < actual_array_size && all_constants; i++) {
    if (LLVMIsConstant(element_values[i]) &&
        LLVMIsAGlobalVariable(element_values[i])) {
      // Check if it's from a different module
      LLVMModuleRef elem_module = LLVMGetGlobalParent(element_values[i]);
      LLVMModuleRef current_module =
          ctx->current_module ? ctx->current_module->module : ctx->module;
      if (elem_module != current_module) {
        all_constants = false; // Force runtime initialization
        break;
      }
    }
  }

  if (all_constants) {
    // Create constant array (now with padding)
    return LLVMConstArray(element_type, element_values, actual_array_size);
  } else {
    // Create runtime array (now with padding)
    LLVMValueRef array_alloca =
        LLVMBuildAlloca(ctx->builder, array_type, "array_literal");

    for (size_t i = 0; i < actual_array_size; i++) {
      // Create GEP to element
      LLVMValueRef indices[2];
      indices[0] = LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false);
      indices[1] = LLVMConstInt(LLVMInt32TypeInContext(ctx->context), i, false);

      LLVMValueRef element_ptr = LLVMBuildGEP2(
          ctx->builder, array_type, array_alloca, indices, 2, "element_ptr");
      LLVMBuildStore(ctx->builder, element_values[i], element_ptr);
    }

    return LLVMBuildLoad2(ctx->builder, array_type, array_alloca, "array_val");
  }
}

LLVMValueRef codegen_expr_index(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_INDEX) {
    fprintf(stderr, "Error: Expected index expression node\n");
    return NULL;
  }

  // **NEW: Special handling for indexing member access (struct.field[index])**
  if (node->expr.index.object->type == AST_EXPR_MEMBER) {
    AstNode *member_expr = node->expr.index.object;
    const char *field_name = member_expr->expr.member.member;

    // Generate the member access to get the pointer
    LLVMValueRef pointer = codegen_expr_struct_access(ctx, member_expr);
    if (!pointer) {
      fprintf(stderr, "Error: Failed to resolve member access for indexing\n");
      return NULL;
    }

    // Generate the index expression
    LLVMValueRef index = codegen_expr(ctx, node->expr.index.index);
    if (!index) {
      return NULL;
    }

    LLVMTypeRef pointer_type = LLVMTypeOf(pointer);
    LLVMTypeKind pointer_kind = LLVMGetTypeKind(pointer_type);

    // The member access should have returned a pointer
    if (pointer_kind != LLVMPointerTypeKind) {
      fprintf(stderr, "Error: Member '%s' is not a pointer type for indexing\n",
              field_name);
      return NULL;
    }

    // Find the struct that contains this field to get element type info
    LLVMTypeRef element_type = NULL;

    // Build a list of field names in the chain (in reverse order)
    const char *field_chain[32];
    int chain_length = 0;
    AstNode *current_node = member_expr;

    while (current_node && current_node->type == AST_EXPR_MEMBER &&
           chain_length < 32) {
      field_chain[chain_length++] = current_node->expr.member.member;
      current_node = current_node->expr.member.object;
    }

    // Reverse the chain to get the correct order
    for (int i = 0; i < chain_length / 2; i++) {
      const char *temp = field_chain[i];
      field_chain[i] = field_chain[chain_length - 1 - i];
      field_chain[chain_length - 1 - i] = temp;
    }

    // Find the base identifier
    AstNode *base_obj = member_expr->expr.member.object;
    while (base_obj->type == AST_EXPR_MEMBER) {
      base_obj = base_obj->expr.member.object;
    }

    if (base_obj->type == AST_EXPR_IDENTIFIER) {
      const char *base_name = base_obj->expr.identifier.name;
      LLVM_Symbol *base_sym = find_symbol(ctx, base_name);

      if (base_sym) {
        // Find the initial struct type
        StructInfo *current_struct = NULL;
        LLVMTypeRef sym_type = base_sym->type;
        LLVMTypeKind sym_kind = LLVMGetTypeKind(sym_type);

        if (sym_kind == LLVMPointerTypeKind && base_sym->element_type) {
          // It's a pointer to struct
          for (StructInfo *info = ctx->struct_types; info; info = info->next) {
            if (info->llvm_type == base_sym->element_type) {
              current_struct = info;
              break;
            }
          }
        } else if (sym_kind == LLVMStructTypeKind) {
          // Direct struct type
          for (StructInfo *info = ctx->struct_types; info; info = info->next) {
            if (info->llvm_type == sym_type) {
              current_struct = info;
              break;
            }
          }
        }

        // Trace through the field chain
        for (int i = 0; i < chain_length && current_struct; i++) {
          const char *current_field_name = field_chain[i];
          int field_idx = get_field_index(current_struct, current_field_name);

          if (field_idx < 0) {
            fprintf(stderr, "Error: Field '%s' not found in struct '%s'\n",
                    current_field_name, current_struct->name);
            break;
          }

          // If this is the last field in the chain, get its element type
          if (i == chain_length - 1) {
            LLVMTypeRef field_type = current_struct->field_types[field_idx];

            // Check if this field is an array - if so, get its element type
            if (LLVMGetTypeKind(field_type) == LLVMArrayTypeKind) {
              element_type = LLVMGetElementType(field_type);
            } else {
              element_type = current_struct->field_element_types[field_idx];
            }
            break;
          }

          // Otherwise, move to the next struct in the chain
          LLVMTypeRef field_type = current_struct->field_types[field_idx];
          LLVMTypeKind field_kind = LLVMGetTypeKind(field_type);

          if (field_kind == LLVMStructTypeKind) {
            // Field is a struct - find its info
            StructInfo *next_struct = NULL;
            for (StructInfo *info = ctx->struct_types; info;
                 info = info->next) {
              if (info->llvm_type == field_type) {
                next_struct = info;
                break;
              }
            }
            current_struct = next_struct;
          } else if (field_kind == LLVMPointerTypeKind) {
            // Field is a pointer - get what it points to
            LLVMTypeRef pointee =
                current_struct->field_element_types[field_idx];
            if (pointee && LLVMGetTypeKind(pointee) == LLVMStructTypeKind) {
              // Find the struct info for the pointee
              StructInfo *next_struct = NULL;
              for (StructInfo *info = ctx->struct_types; info;
                   info = info->next) {
                if (info->llvm_type == pointee) {
                  next_struct = info;
                  break;
                }
              }
              current_struct = next_struct;
            } else {
              // Not a struct pointer, can't continue
              break;
            }
          } else {
            // Field is not a struct or pointer, can't continue
            break;
          }
        }
      }
    }

    if (!element_type) {
      fprintf(
          stderr,
          "Error: Could not determine pointer element type for indexing '%s'\n",
          field_name);
      return NULL;
    }

    // Build GEP for pointer arithmetic
    LLVMValueRef element_ptr = LLVMBuildGEP2(
        ctx->builder, element_type, pointer, &index, 1, "member_ptr_element");

    // Load the actual value
    return LLVMBuildLoad2(ctx->builder, element_type, element_ptr,
                          "member_element_val");
  }

  // Generate the object being indexed
  LLVMValueRef object = codegen_expr(ctx, node->expr.index.object);
  if (!object) {
    fprintf(stderr, "Error: Failed to generate indexed object\n");
    return NULL;
  }

  // Generate the index expression
  LLVMValueRef index = codegen_expr(ctx, node->expr.index.index);
  if (!index) {
    fprintf(stderr, "Error: Failed to generate index expression\n");
    return NULL;
  }

  LLVMTypeRef object_type = LLVMTypeOf(object);
  LLVMTypeKind object_kind = LLVMGetTypeKind(object_type);

  if (object_kind == LLVMArrayTypeKind) {
    // Direct array value indexing (from array literals)
    LLVMTypeRef element_type = LLVMGetElementType(object_type);

    // Allocate with proper alignment for the array type
    LLVMValueRef array_alloca =
        LLVMBuildAlloca(ctx->builder, object_type, "temp_array");

    // Store with proper alignment - CRITICAL for i64 arrays!
    LLVMValueRef store_inst =
        LLVMBuildStore(ctx->builder, object, array_alloca);
    // For i64 arrays, we need 8-byte alignment
    LLVMSetAlignment(store_inst, 8);

    // Use InBoundsGEP for safety
    LLVMValueRef indices[2];
    indices[0] = LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false);
    indices[1] = index;

    LLVMValueRef element_ptr =
        LLVMBuildInBoundsGEP2(ctx->builder, object_type, array_alloca, indices,
                              2, "array_element_ptr");

    // Load with proper alignment
    LLVMValueRef load_inst = LLVMBuildLoad2(ctx->builder, element_type,
                                            element_ptr, "array_element");
    LLVMSetAlignment(load_inst, 8);

    return load_inst;

  } else if (object_kind == LLVMPointerTypeKind) {
    // Handle pointer indexing - check if it's a symbol first for better type
    // info
    LLVMTypeRef pointee_type = NULL;

    if (node->expr.index.object->type == AST_EXPR_IDENTIFIER) {
      const char *var_name = node->expr.index.object->expr.identifier.name;
      LLVM_Symbol *sym = find_symbol(ctx, var_name);

      if (sym && !sym->is_function) {
        // CRITICAL FIX: Use the stored element_type from symbol table
        if (sym->element_type) {
          pointee_type = sym->element_type;
        } else {
          // Check if the symbol type is a pointer to array or just array
          LLVMTypeRef sym_type = sym->type;
          LLVMTypeKind sym_kind = LLVMGetTypeKind(sym_type);

          if (sym_kind == LLVMArrayTypeKind) {
            // This is a direct array stored in the symbol
            LLVMTypeRef element_type = LLVMGetElementType(sym_type);

            LLVMValueRef indices[2];
            indices[0] =
                LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false);
            indices[1] = index;

            LLVMValueRef element_ptr =
                LLVMBuildGEP2(ctx->builder, sym_type, sym->value, indices, 2,
                              "array_element_ptr");

            // Always load the element
            return LLVMBuildLoad2(ctx->builder, element_type, element_ptr,
                                  "array_element");
          } else {
            // Fallback: try to infer from variable name (temporary workaround)
            if (strstr(var_name, "int") && !strstr(var_name, "char")) {
              pointee_type = LLVMInt64TypeInContext(ctx->context);
            } else if (strstr(var_name, "double")) {
              pointee_type = LLVMDoubleTypeInContext(ctx->context);
            } else if (strstr(var_name, "float")) {
              pointee_type = LLVMFloatTypeInContext(ctx->context);
            } else if (strstr(var_name, "char") || strstr(var_name, "_buf")) {
              pointee_type = LLVMInt8TypeInContext(ctx->context);
            }
          }
        }
      }
    } else if (node->expr.index.object->type == AST_EXPR_INDEX) {
      // Second-level indexing: either arr[i][j] for arrays OR ptr[i][j] for
      // pointers We need to handle these differently!

      // First, let's check what the FIRST indexing returns
      // Generate it to see if we get an array value or a pointer
      LLVMValueRef first_index_result =
          codegen_expr(ctx, node->expr.index.object);
      if (!first_index_result) {
        return NULL;
      }

      LLVMTypeRef first_result_type = LLVMTypeOf(first_index_result);
      LLVMTypeKind first_result_kind = LLVMGetTypeKind(first_result_type);

      // CASE 1: First indexing returned an ARRAY value (nested array literal)
      // Example: directions[d] where directions is [[int; 2]; 8]
      if (first_result_kind == LLVMArrayTypeKind) {
        // This is a nested array access - the first index gave us an array
        // Now index into that array
        LLVMTypeRef inner_element_type = LLVMGetElementType(first_result_type);

        // Store the array value so we can GEP into it
        LLVMValueRef temp_alloca = LLVMBuildAlloca(
            ctx->builder, first_result_type, "nested_array_temp");

        // CRITICAL: Store with proper alignment for i64 arrays
        LLVMValueRef store_inst =
            LLVMBuildStore(ctx->builder, first_index_result, temp_alloca);
        LLVMSetAlignment(store_inst, 8);

        // Use InBoundsGEP with proper indices
        LLVMValueRef gep_indices[2];
        gep_indices[0] =
            LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false);
        gep_indices[1] = index;

        LLVMValueRef element_ptr =
            LLVMBuildInBoundsGEP2(ctx->builder, first_result_type, temp_alloca,
                                  gep_indices, 2, "nested_element_ptr");

        // Load with proper alignment
        LLVMValueRef load_inst = LLVMBuildLoad2(
            ctx->builder, inner_element_type, element_ptr, "nested_element");
        LLVMSetAlignment(load_inst, 8);

        return load_inst;
      }

      // CASE 2: First indexing returned a POINTER (double pointer like **byte)
      else if (first_result_kind == LLVMPointerTypeKind) {
        // This is pointer indexing: grid[r][c] where grid is **byte
        // first_index_result is the result of grid[r], which is a *byte

        // Trace back to find the base variable for type info
        AstNode *base_node = node->expr.index.object;
        while (base_node->type == AST_EXPR_INDEX) {
          base_node = base_node->expr.index.object;
        }

        if (base_node->type == AST_EXPR_IDENTIFIER) {
          const char *base_var_name = base_node->expr.identifier.name;
          LLVM_Symbol *base_sym = find_symbol(ctx, base_var_name);

          if (base_sym && base_sym->element_type) {
            // For **byte, element_type is *byte (pointer to byte)
            // We need to dereference once more to get byte
            if (LLVMGetTypeKind(base_sym->element_type) ==
                LLVMPointerTypeKind) {
              // Check the base variable name to infer the final type
              if (strstr(base_var_name, "byte") ||
                  strstr(base_var_name, "char")) {
                pointee_type = LLVMInt8TypeInContext(ctx->context);
              } else if (strstr(base_var_name, "int") &&
                         !strstr(base_var_name, "byte")) {
                pointee_type = LLVMInt64TypeInContext(ctx->context);
              } else if (strstr(base_var_name, "double")) {
                pointee_type = LLVMDoubleTypeInContext(ctx->context);
              } else if (strstr(base_var_name, "float")) {
                pointee_type = LLVMFloatTypeInContext(ctx->context);
              } else {
                pointee_type = LLVMInt8TypeInContext(ctx->context);
              }
            } else {
              pointee_type = base_sym->element_type;
            }
          } else {
            // Fallback based on variable name
            if (strstr(base_var_name, "byte") ||
                strstr(base_var_name, "char")) {
              pointee_type = LLVMInt8TypeInContext(ctx->context);
            } else if (strstr(base_var_name, "double")) {
              pointee_type = LLVMDoubleTypeInContext(ctx->context);
            } else if (strstr(base_var_name, "float")) {
              pointee_type = LLVMFloatTypeInContext(ctx->context);
            } else if (strstr(base_var_name, "int")) {
              pointee_type = LLVMInt64TypeInContext(ctx->context);
            } else {
              pointee_type = LLVMInt8TypeInContext(ctx->context);
            }
          }

          // Now we have the pointer from first indexing and the element type
          // Do the second level of pointer indexing
          LLVMValueRef element_ptr =
              LLVMBuildGEP2(ctx->builder, pointee_type, first_index_result,
                            &index, 1, "ptr_ptr_element");
          return LLVMBuildLoad2(ctx->builder, pointee_type, element_ptr,
                                "ptr_ptr_element_val");
        }
      }
    } else if (node->expr.index.object->type == AST_EXPR_CAST) {
      // Handle cast expressions specially
      AstNode *cast_node = node->expr.index.object;
      if (cast_node->expr.cast.type->type == AST_TYPE_POINTER) {
        // Get the pointee type from the cast target type
        AstNode *pointee_node =
            cast_node->expr.cast.type->type_data.pointer.pointee_type;
        pointee_type = codegen_type(ctx, pointee_node);
      }
    }

    // CRITICAL: Don't fall back to i8! This causes the bug.
    // If we can't determine the type, it's an error condition.
    if (!pointee_type) {
      fprintf(
          stderr,
          "Error: Could not determine pointer element type for indexing '%s'\n",
          node->expr.index.object->type == AST_EXPR_IDENTIFIER
              ? node->expr.index.object->expr.identifier.name
              : "expression");
      return NULL;
    }

    // Build GEP for pointer arithmetic
    LLVMValueRef element_ptr = LLVMBuildGEP2(ctx->builder, pointee_type, object,
                                             &index, 1, "ptr_element_ptr");

    // Load the actual value
    LLVMValueRef result = LLVMBuildLoad2(ctx->builder, pointee_type,
                                         element_ptr, "ptr_element_val");

    return result;

  } else {
    fprintf(stderr, "Error: Cannot index expression of type kind %d\n",
            object_kind);
    return NULL;
  }
}

// cast<type>(value)
LLVMValueRef codegen_expr_cast(CodeGenContext *ctx, AstNode *node) {
  LLVMTypeRef target_type = codegen_type(ctx, node->expr.cast.type);
  LLVMValueRef value = codegen_expr(ctx, node->expr.cast.castee);
  if (!target_type || !value)
    return NULL;

  LLVMTypeRef source_type = LLVMTypeOf(value);

  // If types are the same, no cast needed
  if (source_type == target_type)
    return value;

  LLVMTypeKind source_kind = LLVMGetTypeKind(source_type);
  LLVMTypeKind target_kind = LLVMGetTypeKind(target_type);

  // Float to Integer
  if (source_kind == LLVMFloatTypeKind || source_kind == LLVMDoubleTypeKind) {
    if (target_kind == LLVMIntegerTypeKind) {
      // Float to signed integer (truncates decimal part)
      return LLVMBuildFPToSI(ctx->builder, value, target_type, "fptosi");
    }
  }

  // Integer to Float
  if (source_kind == LLVMIntegerTypeKind) {
    if (target_kind == LLVMFloatTypeKind || target_kind == LLVMDoubleTypeKind) {
      // Signed integer to float
      return LLVMBuildSIToFP(ctx->builder, value, target_type, "sitofp");
    }
  }

  // Integer to Integer (different sizes)
  if (source_kind == LLVMIntegerTypeKind &&
      target_kind == LLVMIntegerTypeKind) {
    unsigned source_bits = LLVMGetIntTypeWidth(source_type);
    unsigned target_bits = LLVMGetIntTypeWidth(target_type);

    if (source_bits > target_bits) {
      // Truncate
      return LLVMBuildTrunc(ctx->builder, value, target_type, "trunc");
    } else if (source_bits < target_bits) {
      // Sign extend (for signed integers)
      return LLVMBuildSExt(ctx->builder, value, target_type, "sext");
    }
  }

  // Float to Float (different precision)
  if ((source_kind == LLVMFloatTypeKind || source_kind == LLVMDoubleTypeKind) &&
      (target_kind == LLVMFloatTypeKind || target_kind == LLVMDoubleTypeKind)) {
    if (source_kind == LLVMFloatTypeKind && target_kind == LLVMDoubleTypeKind) {
      // Float to double
      return LLVMBuildFPExt(ctx->builder, value, target_type, "fpext");
    } else if (source_kind == LLVMDoubleTypeKind &&
               target_kind == LLVMFloatTypeKind) {
      // Double to float
      return LLVMBuildFPTrunc(ctx->builder, value, target_type, "fptrunc");
    }
  }

  // Pointer casts
  if (source_kind == LLVMPointerTypeKind &&
      target_kind == LLVMPointerTypeKind) {
    return LLVMBuildPointerCast(ctx->builder, value, target_type, "ptrcast");
  }

  // Integer to Pointer
  if (source_kind == LLVMIntegerTypeKind &&
      target_kind == LLVMPointerTypeKind) {
    return LLVMBuildIntToPtr(ctx->builder, value, target_type, "inttoptr");
  }

  // Pointer to Integer
  if (source_kind == LLVMPointerTypeKind &&
      target_kind == LLVMIntegerTypeKind) {
    return LLVMBuildPtrToInt(ctx->builder, value, target_type, "ptrtoint");
  }

  // Fallback to bitcast (use sparingly)
  return LLVMBuildBitCast(ctx->builder, value, target_type, "bitcast");
}

LLVMValueRef codegen_expr_input(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_INPUT) {
    fprintf(stderr, "Error: Expected input expression node\n");
    return NULL;
  }

  LLVMModuleRef current_llvm_module =
      ctx->current_module ? ctx->current_module->module : ctx->module;

  // Get the target type for the input
  LLVMTypeRef target_type = codegen_type(ctx, node->expr.input.type);
  if (!target_type) {
    fprintf(stderr, "Error: Failed to generate type for input expression\n");
    return NULL;
  }

  // Determine the type kind
  LLVMTypeKind type_kind = LLVMGetTypeKind(target_type);

  // Print the message if provided
  if (node->expr.input.msg) {
    LLVMValueRef printf_func =
        LLVMGetNamedFunction(current_llvm_module, "printf");
    LLVMTypeRef printf_type = NULL;

    if (!printf_func) {
      LLVMTypeRef printf_arg_types[] = {
          LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0)};
      printf_type = LLVMFunctionType(LLVMInt32TypeInContext(ctx->context),
                                     printf_arg_types, 1, true);
      printf_func = LLVMAddFunction(current_llvm_module, "printf", printf_type);
    } else {
      printf_type = LLVMGlobalGetValueType(printf_func);
    }

    // Generate and print the message
    LLVMValueRef msg_value = codegen_expr(ctx, node->expr.input.msg);
    if (msg_value) {
      LLVMValueRef args[] = {msg_value};
      LLVMBuildCall2(ctx->builder, printf_type, printf_func, args, 1, "");
    }
  }

  // Declare scanf if not already declared
  LLVMValueRef scanf_func = LLVMGetNamedFunction(current_llvm_module, "scanf");
  LLVMTypeRef scanf_type = NULL;

  if (!scanf_func) {
    LLVMTypeRef scanf_arg_types[] = {
        LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0)};
    scanf_type = LLVMFunctionType(LLVMInt32TypeInContext(ctx->context),
                                  scanf_arg_types, 1, true);
    scanf_func = LLVMAddFunction(current_llvm_module, "scanf", scanf_type);
    LLVMSetLinkage(scanf_func, LLVMExternalLinkage);
  } else {
    scanf_type = LLVMGlobalGetValueType(scanf_func);
  }

  // Allocate space for the input value
  LLVMValueRef input_alloca =
      LLVMBuildAlloca(ctx->builder, target_type, "input_temp");

  // Determine the scanf format string based on type
  const char *format_str = NULL;
  LLVMValueRef result;

  if (type_kind == LLVMIntegerTypeKind) {
    unsigned bits = LLVMGetIntTypeWidth(target_type);

    if (bits == 1) {
      // bool: read as int, then convert
      LLVMTypeRef int_type = LLVMInt32TypeInContext(ctx->context);
      LLVMValueRef int_alloca =
          LLVMBuildAlloca(ctx->builder, int_type, "bool_temp");

      format_str = "%d";
      LLVMValueRef format_str_val =
          LLVMBuildGlobalStringPtr(ctx->builder, format_str, "input_fmt");
      LLVMValueRef scanf_args[] = {format_str_val, int_alloca};
      LLVMBuildCall2(ctx->builder, scanf_type, scanf_func, scanf_args, 2, "");

      // Load int and convert to bool
      LLVMValueRef int_val =
          LLVMBuildLoad2(ctx->builder, int_type, int_alloca, "int_val");
      LLVMValueRef zero = LLVMConstInt(int_type, 0, false);
      result =
          LLVMBuildICmp(ctx->builder, LLVMIntNE, int_val, zero, "bool_val");

    } else if (bits == 8) {
      // char
      format_str = "%c"; // Space before %c to skip whitespace
      LLVMValueRef format_str_val =
          LLVMBuildGlobalStringPtr(ctx->builder, format_str, "input_fmt");
      LLVMValueRef scanf_args[] = {format_str_val, input_alloca};
      LLVMBuildCall2(ctx->builder, scanf_type, scanf_func, scanf_args, 2, "");
      result =
          LLVMBuildLoad2(ctx->builder, target_type, input_alloca, "input_val");

    } else if (bits <= 32) {
      // int (32-bit or less)
      format_str = "%d";
      LLVMValueRef format_str_val =
          LLVMBuildGlobalStringPtr(ctx->builder, format_str, "input_fmt");
      LLVMValueRef scanf_args[] = {format_str_val, input_alloca};
      LLVMBuildCall2(ctx->builder, scanf_type, scanf_func, scanf_args, 2, "");
      result =
          LLVMBuildLoad2(ctx->builder, target_type, input_alloca, "input_val");

    } else {
      // int (64-bit)
      format_str = "%lld";
      LLVMValueRef format_str_val =
          LLVMBuildGlobalStringPtr(ctx->builder, format_str, "input_fmt");
      LLVMValueRef scanf_args[] = {format_str_val, input_alloca};
      LLVMBuildCall2(ctx->builder, scanf_type, scanf_func, scanf_args, 2, "");
      result =
          LLVMBuildLoad2(ctx->builder, target_type, input_alloca, "input_val");
    }

  } else if (type_kind == LLVMFloatTypeKind) {
    // float
    format_str = "%f";
    LLVMValueRef format_str_val =
        LLVMBuildGlobalStringPtr(ctx->builder, format_str, "input_fmt");
    LLVMValueRef scanf_args[] = {format_str_val, input_alloca};
    LLVMBuildCall2(ctx->builder, scanf_type, scanf_func, scanf_args, 2, "");
    result =
        LLVMBuildLoad2(ctx->builder, target_type, input_alloca, "input_val");

  } else if (type_kind == LLVMDoubleTypeKind) {
    // double
    format_str = "%lf";
    LLVMValueRef format_str_val =
        LLVMBuildGlobalStringPtr(ctx->builder, format_str, "input_fmt");
    LLVMValueRef scanf_args[] = {format_str_val, input_alloca};
    LLVMBuildCall2(ctx->builder, scanf_type, scanf_func, scanf_args, 2, "");
    result =
        LLVMBuildLoad2(ctx->builder, target_type, input_alloca, "input_val");

  } else if (type_kind == LLVMPointerTypeKind) {
    // string: allocate buffer and read line
    // Allocate a 256-byte buffer for string input
    LLVMTypeRef char_type = LLVMInt8TypeInContext(ctx->context);
    LLVMTypeRef buffer_type = LLVMArrayType(char_type, 256);
    LLVMValueRef buffer_alloca =
        LLVMBuildAlloca(ctx->builder, buffer_type, "str_buffer");

    // Get pointer to first element
    LLVMValueRef indices[2] = {
        LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false),
        LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false)};
    LLVMValueRef buffer_ptr = LLVMBuildGEP2(
        ctx->builder, buffer_type, buffer_alloca, indices, 2, "buffer_ptr");

    // Read string (max 255 chars + null terminator)
    format_str = "%255s";
    LLVMValueRef format_str_val =
        LLVMBuildGlobalStringPtr(ctx->builder, format_str, "input_fmt");
    LLVMValueRef scanf_args[] = {format_str_val, buffer_ptr};
    LLVMBuildCall2(ctx->builder, scanf_type, scanf_func, scanf_args, 2, "");

    result = buffer_ptr;

  } else {
    fprintf(stderr, "Error: Unsupported input type kind %d\n", type_kind);
    return NULL;
  }

  return result;
}

LLVMValueRef codegen_expr_system(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_SYSTEM) {
    fprintf(stderr, "Error: Expected system expression node\n");
    return NULL;
  }

  // Get the command expression
  LLVMValueRef command = codegen_expr(ctx, node->expr._system.command);
  if (!command) {
    fprintf(stderr, "Error: Failed to generate system command\n");
    return NULL;
  }

  // Get current LLVM module
  LLVMModuleRef current_llvm_module =
      ctx->current_module ? ctx->current_module->module : ctx->module;

  // Declare system function if not already declared
  LLVMValueRef system_func =
      LLVMGetNamedFunction(current_llvm_module, "system");
  LLVMTypeRef system_type = NULL;

  if (!system_func) {
    // Declare system: int system(const char *command)
    LLVMTypeRef param_types[] = {
        LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0)};
    system_type = LLVMFunctionType(LLVMInt32TypeInContext(ctx->context),
                                   param_types, 1, false);
    system_func = LLVMAddFunction(current_llvm_module, "system", system_type);
    LLVMSetLinkage(system_func, LLVMExternalLinkage);
  } else {
    system_type = LLVMGlobalGetValueType(system_func);
  }

  // Ensure command is a string pointer (char*)
  LLVMTypeRef command_type = LLVMTypeOf(command);
  LLVMTypeKind command_kind = LLVMGetTypeKind(command_type);

  if (command_kind != LLVMPointerTypeKind) {
    fprintf(stderr, "Error: System command must be a string (char*)\n");
    return NULL;
  }

  // Call system with the command
  LLVMValueRef args[] = {command};
  return LLVMBuildCall2(ctx->builder, system_type, system_func, args, 1,
                        "system_call");
}

/**
 * @brief Generate LLVM IR for syscall expression
 *
 * Syscall in x86_64 Linux:
 * - syscall number goes in %rax
 * - arguments go in %rdi, %rsi, %rdx, %r10, %r8, %r9 (in that order)
 * - return value comes back in %rax
 *
 * We use inline assembly to invoke the syscall instruction.
 */
LLVMValueRef codegen_expr_syscall(CodeGenContext *ctx, AstNode *node) {
  if (!node || node->type != AST_EXPR_SYSCALL) {
    fprintf(stderr, "Error: Expected syscall expression node\n");
    return NULL;
  }

  AstNode **args = node->expr.syscall.args;
  size_t arg_count = node->expr.syscall.count;

  // Syscall requires at least 1 argument (the syscall number)
  if (arg_count == 0) {
    fprintf(
        stderr,
        "Error: syscall() requires at least one argument (syscall number)\n");
    return NULL;
  }

  // Maximum 7 arguments: syscall_num + 6 syscall args
  if (arg_count > 7) {
    fprintf(stderr, "Error: syscall() supports maximum 7 arguments (syscall "
                    "number + 6 parameters)\n");
    return NULL;
  }

  // Get current LLVM module
  LLVMModuleRef current_llvm_module =
      ctx->current_module ? ctx->current_module->module : ctx->module;

  // Generate all arguments
  LLVMValueRef *llvm_args = (LLVMValueRef *)arena_alloc(
      ctx->arena, sizeof(LLVMValueRef) * arg_count, alignof(LLVMValueRef));

  for (size_t i = 0; i < arg_count; i++) {
    llvm_args[i] = codegen_expr(ctx, args[i]);
    if (!llvm_args[i]) {
      fprintf(stderr, "Error: Failed to generate syscall argument %zu\n",
              i + 1);
      return NULL;
    }

    // Ensure all arguments are i64 (syscalls expect 64-bit values)
    LLVMTypeRef arg_type = LLVMTypeOf(llvm_args[i]);
    LLVMTypeKind arg_kind = LLVMGetTypeKind(arg_type);

    if (arg_kind == LLVMIntegerTypeKind) {
      unsigned bits = LLVMGetIntTypeWidth(arg_type);
      if (bits < 64) {
        // Zero-extend smaller integers to i64
        llvm_args[i] = LLVMBuildZExt(ctx->builder, llvm_args[i],
                                     LLVMInt64TypeInContext(ctx->context),
                                     "syscall_arg_ext");
      } else if (bits > 64) {
        // Truncate larger integers to i64
        llvm_args[i] = LLVMBuildTrunc(ctx->builder, llvm_args[i],
                                      LLVMInt64TypeInContext(ctx->context),
                                      "syscall_arg_trunc");
      }
    } else if (arg_kind == LLVMPointerTypeKind) {
      // Convert pointer to i64
      llvm_args[i] = LLVMBuildPtrToInt(ctx->builder, llvm_args[i],
                                       LLVMInt64TypeInContext(ctx->context),
                                       "syscall_ptr_to_int");
    } else if (arg_kind == LLVMFloatTypeKind ||
               arg_kind == LLVMDoubleTypeKind) {
      // Convert float/double to i64 (reinterpret bits, don't convert value)
      fprintf(stderr,
              "Warning: syscall argument %zu is float/double, casting to int\n",
              i + 1);
      llvm_args[i] = LLVMBuildFPToSI(ctx->builder, llvm_args[i],
                                     LLVMInt64TypeInContext(ctx->context),
                                     "syscall_float_to_int");
    }
  }

#if defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
  long sysnum = -1;
  if (LLVMIsAConstantInt(llvm_args[0])) {
    sysnum = (long)LLVMConstIntGetSExtValue(llvm_args[0]);
  }
  if ((sysnum == 4 || sysnum == 0x2000004) && arg_count >= 4) {
    LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx->context);
    LLVMTypeRef i64 = LLVMInt64TypeInContext(ctx->context);
    LLVMTypeRef i8p = LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
    LLVMTypeRef fn_ty =
        LLVMFunctionType(i64, (LLVMTypeRef[]){i32, i8p, i64}, 3, false);
    LLVMValueRef fn = LLVMGetNamedFunction(current_llvm_module, "write");
    if (!fn) {
      fn = LLVMAddFunction(current_llvm_module, "write", fn_ty);
      LLVMSetLinkage(fn, LLVMExternalLinkage);
    }
    LLVMValueRef fd = codegen_expr(ctx, args[1]);
    LLVMValueRef buf = codegen_expr(ctx, args[2]);
    LLVMValueRef cnt = codegen_expr(ctx, args[3]);
    if (LLVMGetTypeKind(LLVMTypeOf(fd)) != LLVMIntegerTypeKind) {
      fd = LLVMBuildPtrToInt(ctx->builder, fd, i64, "fd_to_i64");
    }
    unsigned fbits = LLVMGetIntTypeWidth(LLVMTypeOf(fd));
    if (fbits > 32)
      fd = LLVMBuildTrunc(ctx->builder, fd, i32, "fd_trunc");
    if (LLVMGetTypeKind(LLVMTypeOf(buf)) == LLVMIntegerTypeKind) {
      buf = LLVMBuildIntToPtr(ctx->builder, buf, i8p, "buf_to_ptr");
    }
    if (LLVMGetTypeKind(LLVMTypeOf(cnt)) != LLVMIntegerTypeKind) {
      cnt = LLVMBuildPtrToInt(ctx->builder, cnt, i64, "cnt_ptr_to_i64");
    } else {
      unsigned cbits = LLVMGetIntTypeWidth(LLVMTypeOf(cnt));
      if (cbits < 64)
        cnt = LLVMBuildZExt(ctx->builder, cnt, i64, "cnt_zext");
    }
    LLVMValueRef call_args[3] = {fd, buf, cnt};
    return LLVMBuildCall2(ctx->builder, fn_ty, fn, call_args, 3,
                          "write_result");
  } else if ((sysnum == 3 || sysnum == 0x2000003) && arg_count >= 4) {
    LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx->context);
    LLVMTypeRef i64 = LLVMInt64TypeInContext(ctx->context);
    LLVMTypeRef i8p = LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
    LLVMTypeRef fn_ty =
        LLVMFunctionType(i64, (LLVMTypeRef[]){i32, i8p, i64}, 3, false);
    LLVMValueRef fn = LLVMGetNamedFunction(current_llvm_module, "read");
    if (!fn) {
      fn = LLVMAddFunction(current_llvm_module, "read", fn_ty);
      LLVMSetLinkage(fn, LLVMExternalLinkage);
    }
    LLVMValueRef fd = codegen_expr(ctx, args[1]);
    LLVMValueRef buf = codegen_expr(ctx, args[2]);
    LLVMValueRef cnt = codegen_expr(ctx, args[3]);
    if (LLVMGetTypeKind(LLVMTypeOf(fd)) != LLVMIntegerTypeKind) {
      fd = LLVMBuildPtrToInt(ctx->builder, fd, i64, "fd_to_i64");
    }
    unsigned fbits = LLVMGetIntTypeWidth(LLVMTypeOf(fd));
    if (fbits > 32)
      fd = LLVMBuildTrunc(ctx->builder, fd, i32, "fd_trunc");
    if (LLVMGetTypeKind(LLVMTypeOf(buf)) == LLVMIntegerTypeKind) {
      buf = LLVMBuildIntToPtr(ctx->builder, buf, i8p, "buf_to_ptr");
    }
    if (LLVMGetTypeKind(LLVMTypeOf(cnt)) != LLVMIntegerTypeKind) {
      cnt = LLVMBuildPtrToInt(ctx->builder, cnt, i64, "cnt_ptr_to_i64");
    } else {
      unsigned cbits = LLVMGetIntTypeWidth(LLVMTypeOf(cnt));
      if (cbits < 64)
        cnt = LLVMBuildZExt(ctx->builder, cnt, i64, "cnt_zext");
    }
    LLVMValueRef call_args[3] = {fd, buf, cnt};
    return LLVMBuildCall2(ctx->builder, fn_ty, fn, call_args, 3, "read_result");
  } else if ((sysnum == 5 || sysnum == 0x2000005) && arg_count >= 4) {
    LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx->context);
    LLVMTypeRef i8p = LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
    LLVMTypeRef fn_ty =
        LLVMFunctionType(i32, (LLVMTypeRef[]){i8p, i32, i32}, 3, false);
    LLVMValueRef fn = LLVMGetNamedFunction(current_llvm_module, "open");
    if (!fn) {
      fn = LLVMAddFunction(current_llvm_module, "open", fn_ty);
      LLVMSetLinkage(fn, LLVMExternalLinkage);
    }
    LLVMValueRef path = codegen_expr(ctx, args[1]);
    LLVMValueRef flags = codegen_expr(ctx, args[2]);
    LLVMValueRef mode = codegen_expr(ctx, args[3]);
    if (LLVMGetTypeKind(LLVMTypeOf(path)) == LLVMIntegerTypeKind) {
      path = LLVMBuildIntToPtr(ctx->builder, path, i8p, "path_to_ptr");
    }
    if (LLVMGetTypeKind(LLVMTypeOf(flags)) != LLVMIntegerTypeKind) {
      flags = LLVMBuildPtrToInt(ctx->builder, flags, i32, "flags_ptr_to_i32");
    } else {
      unsigned w = LLVMGetIntTypeWidth(LLVMTypeOf(flags));
      if (w > 32)
        flags = LLVMBuildTrunc(ctx->builder, flags, i32, "flags_trunc");
    }
    if (LLVMGetTypeKind(LLVMTypeOf(mode)) != LLVMIntegerTypeKind) {
      mode = LLVMBuildPtrToInt(ctx->builder, mode, i32, "mode_ptr_to_i32");
    } else {
      unsigned w = LLVMGetIntTypeWidth(LLVMTypeOf(mode));
      if (w > 32)
        mode = LLVMBuildTrunc(ctx->builder, mode, i32, "mode_trunc");
    }
    LLVMValueRef call_args[3] = {path, flags, mode};
    LLVMValueRef r =
        LLVMBuildCall2(ctx->builder, fn_ty, fn, call_args, 3, "open_result");
    return LLVMBuildZExt(ctx->builder, r, LLVMInt64TypeInContext(ctx->context),
                         "open_result_i64");
  } else if ((sysnum == 6 || sysnum == 0x2000006) && arg_count >= 2) {
    LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx->context);
    LLVMTypeRef fn_ty = LLVMFunctionType(i32, (LLVMTypeRef[]){i32}, 1, false);
    LLVMValueRef fn = LLVMGetNamedFunction(current_llvm_module, "close");
    if (!fn) {
      fn = LLVMAddFunction(current_llvm_module, "close", fn_ty);
      LLVMSetLinkage(fn, LLVMExternalLinkage);
    }
    LLVMValueRef fd = codegen_expr(ctx, args[1]);
    if (LLVMGetTypeKind(LLVMTypeOf(fd)) != LLVMIntegerTypeKind) {
      fd = LLVMBuildPtrToInt(ctx->builder, fd,
                             LLVMInt64TypeInContext(ctx->context), "fd_to_i64");
    }
    unsigned w = LLVMGetIntTypeWidth(LLVMTypeOf(fd));
    if (w > 32)
      fd = LLVMBuildTrunc(ctx->builder, fd, i32, "fd_trunc");
    LLVMValueRef r = LLVMBuildCall2(ctx->builder, fn_ty, fn,
                                    (LLVMValueRef[]){fd}, 1, "close_result");
    return LLVMBuildZExt(ctx->builder, r, LLVMInt64TypeInContext(ctx->context),
                         "close_result_i64");
  } else {
    LLVMTypeRef ret_ty = LLVMInt64TypeInContext(ctx->context);
    LLVMTypeRef num_ty = LLVMInt64TypeInContext(ctx->context);
    LLVMTypeRef fn_ty =
        LLVMFunctionType(ret_ty, (LLVMTypeRef[]){num_ty}, 1, true);
    LLVMValueRef fn = LLVMGetNamedFunction(current_llvm_module, "syscall");
    if (!fn) {
      fn = LLVMAddFunction(current_llvm_module, "syscall", fn_ty);
      LLVMSetLinkage(fn, LLVMExternalLinkage);
    }
    return LLVMBuildCall2(ctx->builder, fn_ty, fn, llvm_args, arg_count,
                          "syscall_result");
  }
#else
  const char *asm_template;
  const char *constraints;

  switch (arg_count) {
  case 1: // syscall number only
    asm_template = "syscall";
    constraints = "={rax},{rax}";
    break;
  case 2: // syscall + 1 arg
    asm_template = "syscall";
    constraints = "={rax},{rax},{rdi}";
    break;
  case 3: // syscall + 2 args
    asm_template = "syscall";
    constraints = "={rax},{rax},{rdi},{rsi}";
    break;
  case 4: // syscall + 3 args
    asm_template = "syscall";
    constraints = "={rax},{rax},{rdi},{rsi},{rdx}";
    break;
  case 5: // syscall + 4 args
    asm_template = "syscall";
    constraints = "={rax},{rax},{rdi},{rsi},{rdx},{r10}";
    break;
  case 6: // syscall + 5 args
    asm_template = "syscall";
    constraints = "={rax},{rax},{rdi},{rsi},{rdx},{r10},{r8}";
    break;
  case 7: // syscall + 6 args (maximum)
    asm_template = "syscall";
    constraints = "={rax},{rax},{rdi},{rsi},{rdx},{r10},{r8},{r9}";
    break;
  default:
    fprintf(stderr, "Error: Invalid syscall argument count\n");
    return NULL;
  }

  // Create parameter types array (all i64)
  LLVMTypeRef *param_types = (LLVMTypeRef *)arena_alloc(
      ctx->arena, sizeof(LLVMTypeRef) * arg_count, alignof(LLVMTypeRef));

  for (size_t i = 0; i < arg_count; i++) {
    param_types[i] = LLVMInt64TypeInContext(ctx->context);
  }

  // Create inline assembly function type: i64 (args...)
  LLVMTypeRef asm_func_type = LLVMFunctionType(
      LLVMInt64TypeInContext(ctx->context), // return type (syscall result)
      param_types, arg_count,
      false // not vararg
  );

  // Create the inline assembly call
  LLVMValueRef asm_func =
      LLVMGetInlineAsm(asm_func_type,
                       (char *)asm_template, // assembly template
                       strlen(asm_template),
                       (char *)constraints, // constraints
                       strlen(constraints),
                       true,                    // has side effects
                       false,                   // align stack
                       LLVMInlineAsmDialectATT, // AT&T syntax
                       false                    // can throw
      );

  // Call the inline assembly
  LLVMValueRef result = LLVMBuildCall2(ctx->builder, asm_func_type, asm_func,
                                       llvm_args, arg_count, "syscall_result");

  // Mark as volatile to prevent optimization
  LLVMSetVolatile(result, true);

  return result;
#endif
}

// sizeof<type || expr>
LLVMValueRef codegen_expr_sizeof(CodeGenContext *ctx, AstNode *node) {
  LLVMTypeRef type;
  if (node->expr.size_of.is_type) {
    type = codegen_type(ctx, node->expr.size_of.object);
  } else {
    LLVMValueRef expr = codegen_expr(ctx, node->expr.size_of.object);
    if (!expr)
      return NULL;
    type = LLVMTypeOf(expr);
  }
  if (!type)
    return NULL;

  LLVMTypeKind kind = LLVMGetTypeKind(type);

  switch (kind) {
  case LLVMIntegerTypeKind: {
    unsigned width = LLVMGetIntTypeWidth(type);
    return LLVMConstInt(LLVMInt64TypeInContext(ctx->context), width / 8, false);
  }
  case LLVMFloatTypeKind:
    return LLVMConstInt(LLVMInt64TypeInContext(ctx->context), 4, false);
  case LLVMDoubleTypeKind:
    return LLVMConstInt(LLVMInt64TypeInContext(ctx->context), 8, false);
  case LLVMPointerTypeKind:
    return LLVMConstInt(LLVMInt64TypeInContext(ctx->context), 8, false);
  case LLVMStructTypeKind: {
    unsigned field_count = LLVMCountStructElementTypes(type);
    LLVMTypeRef *field_types = malloc(field_count * sizeof(LLVMTypeRef));
    LLVMGetStructElementTypes(type, field_types);

    uint64_t total_size = 0;
    uint64_t max_align = 1;

    for (unsigned i = 0; i < field_count; i++) {
      LLVMTypeRef ftype = field_types[i];
      uint64_t fsize = 0;
      uint64_t falign = 1;

      LLVMTypeKind fkind = LLVMGetTypeKind(ftype);
      switch (fkind) {
      case LLVMIntegerTypeKind:
        fsize = LLVMGetIntTypeWidth(ftype) / 8;
        falign = fsize > 8 ? 8 : fsize; // int64 align=8
        break;
      case LLVMFloatTypeKind:
        fsize = 4;
        falign = 4;
        break;
      case LLVMDoubleTypeKind:
        fsize = 8;
        falign = 8;
        break;
      case LLVMPointerTypeKind:
        fsize = 8;
        falign = 8;
        break;
      default:
        fsize = 8;
        falign = 8;
        break;
      }

      // align current offset
      if (total_size % falign != 0)
        total_size += falign - (total_size % falign);

      total_size += fsize;
      if (falign > max_align)
        max_align = falign;
    }

    // align final struct size to max_align
    if (total_size % max_align != 0)
      total_size += max_align - (total_size % max_align);

    free(field_types);

    return LLVMConstInt(LLVMInt64TypeInContext(ctx->context), total_size,
                        false);
  }

  default:
    return LLVMConstInt(LLVMInt64TypeInContext(ctx->context), 8, false);
  }
}

// alloc(expr) - allocates memory on heap using malloc
LLVMValueRef codegen_expr_alloc(CodeGenContext *ctx, AstNode *node) {
  LLVMValueRef size = codegen_expr(ctx, node->expr.alloc.size);
  if (!size)
    return NULL;

  // Get or declare malloc function
  LLVMModuleRef current_llvm_module =
      ctx->current_module ? ctx->current_module->module : ctx->module;
  LLVMValueRef malloc_func =
      LLVMGetNamedFunction(current_llvm_module, "malloc");

  if (!malloc_func) {
    // Declare malloc: void* malloc(size_t size)
    LLVMTypeRef size_t_type = LLVMInt64TypeInContext(ctx->context);
    LLVMTypeRef void_ptr_type =
        LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
    LLVMTypeRef malloc_type =
        LLVMFunctionType(void_ptr_type, &size_t_type, 1, 0);
    malloc_func = LLVMAddFunction(current_llvm_module, "malloc", malloc_type);

    // Set malloc as external linkage
    LLVMSetLinkage(malloc_func, LLVMExternalLinkage);
  }

  // Call malloc with the size
  LLVMTypeRef malloc_func_type = LLVMGlobalGetValueType(malloc_func);
  return LLVMBuildCall2(ctx->builder, malloc_func_type, malloc_func, &size, 1,
                        "alloc");
}

// free(expr)
LLVMValueRef codegen_expr_free(CodeGenContext *ctx, AstNode *node) {
  LLVMValueRef ptr = codegen_expr(ctx, node->expr.free.ptr);
  if (!ptr)
    return NULL;

  // Get or declare free function
  LLVMModuleRef current_llvm_module =
      ctx->current_module ? ctx->current_module->module : ctx->module;
  LLVMValueRef free_func = LLVMGetNamedFunction(current_llvm_module, "free");

  if (!free_func) {
    // Declare free: void free(void* ptr)
    LLVMTypeRef void_type = LLVMVoidTypeInContext(ctx->context);
    LLVMTypeRef ptr_type =
        LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
    LLVMTypeRef free_type = LLVMFunctionType(void_type, &ptr_type, 1, 0);
    free_func = LLVMAddFunction(current_llvm_module, "free", free_type);
    LLVMSetLinkage(free_func, LLVMExternalLinkage);
  }

  // Cast pointer to void* if needed
  LLVMTypeRef void_ptr_type =
      LLVMPointerType(LLVMInt8TypeInContext(ctx->context), 0);
  LLVMValueRef void_ptr = LLVMBuildPointerCast(ctx->builder, ptr, void_ptr_type,
                                               "cast_to_void_ptr");

  // Call free with the void pointer (no name since it returns void)
  LLVMTypeRef free_func_type = LLVMGlobalGetValueType(free_func);
  LLVMBuildCall2(ctx->builder, free_func_type, free_func, &void_ptr, 1, "");

  // Return a void constant since free() doesn't return a value
  return LLVMConstNull(LLVMVoidTypeInContext(ctx->context));
}

LLVMValueRef codegen_expr_deref(CodeGenContext *ctx, AstNode *node) {
  LLVMValueRef ptr = codegen_expr(ctx, node->expr.deref.object);
  if (!ptr)
    return NULL;

  LLVMTypeRef ptr_type = LLVMTypeOf(ptr);

  // Ensure we have a pointer type
  if (LLVMGetTypeKind(ptr_type) != LLVMPointerTypeKind) {
    fprintf(stderr, "Error: Attempting to dereference non-pointer type\n");
    return NULL;
  }

  // Try to infer the element type from the variable's symbol information
  LLVMTypeRef element_type = NULL;

  // If the dereference target is an identifier, try to get type info from
  // symbol table
  if (node->expr.deref.object->type == AST_EXPR_IDENTIFIER) {
    const char *var_name = node->expr.deref.object->expr.identifier.name;
    LLVM_Symbol *sym = find_symbol(ctx, var_name);

    if (sym && !sym->is_function) {
      if (sym->element_type) {
        element_type = sym->element_type;
      } else {
        // Fallback: infer from variable name patterns
        if (strstr(var_name, "ptr") || strstr(var_name, "aligned_ptr")) {
          // Check if this looks like a void** -> void* case
          if (strstr(var_name, "aligned")) {
            element_type = LLVMPointerType(LLVMInt8TypeInContext(ctx->context),
                                           0); // void*
          } else if (strstr(var_name, "char") || strstr(var_name, "str")) {
            element_type = LLVMInt8TypeInContext(ctx->context); // char
          } else if (strstr(var_name, "int")) {
            element_type = LLVMInt64TypeInContext(ctx->context); // int
          } else if (strstr(var_name, "float")) {
            element_type = LLVMFloatTypeInContext(ctx->context); // float
          } else if (strstr(var_name, "double")) {
            element_type = LLVMDoubleTypeInContext(ctx->context);
          } else {
            // Default for unknown pointer types
            element_type = LLVMInt64TypeInContext(ctx->context);
          }
        } else {
          // Generic pointer dereference - assume int64 for safety
          element_type = LLVMInt64TypeInContext(ctx->context);
        }
      }
    }
  }

  // Final fallback if we couldn't determine the type
  if (!element_type) {
    fprintf(stderr, "Warning: Could not determine pointer element type for "
                    "dereference, defaulting to i64\n");
    element_type = LLVMInt64TypeInContext(ctx->context);
  }

  return LLVMBuildLoad2(ctx->builder, element_type, ptr, "deref");
}

// &expr - get address of expression
LLVMValueRef codegen_expr_addr(CodeGenContext *ctx, AstNode *node) {
  // The address-of operator should only work on lvalues (variables,
  // dereferenced pointers, etc.)
  AstNode *target = node->expr.addr.object;

  if (target->type == AST_EXPR_IDENTIFIER) {
    // Get address of a variable
    LLVM_Symbol *sym = find_symbol(ctx, target->expr.identifier.name);
    if (sym && !sym->is_function) {
      // Return the alloca/global directly (it's already a pointer)
      return sym->value;
    }
  } else if (target->type == AST_EXPR_DEREF) {
    // &(*ptr) == ptr
    return codegen_expr(ctx, target->expr.deref.object);
  } else if (target->type == AST_EXPR_INDEX) {
    // ADD THIS CASE: Handle &arr[i] for pointer indexing
    LLVMValueRef object = codegen_expr(ctx, target->expr.index.object);
    if (!object) {
      return NULL;
    }

    LLVMValueRef index = codegen_expr(ctx, target->expr.index.index);
    if (!index) {
      return NULL;
    }

    LLVMTypeRef object_type = LLVMTypeOf(object);
    LLVMTypeKind object_kind = LLVMGetTypeKind(object_type);

    if (object_kind == LLVMPointerTypeKind) {
      // For pointer indexing, determine element type
      LLVMTypeRef element_type = NULL;

      if (target->expr.index.object->type == AST_EXPR_IDENTIFIER) {
        const char *var_name = target->expr.index.object->expr.identifier.name;
        LLVM_Symbol *sym = find_symbol(ctx, var_name);
        if (sym && sym->element_type) {
          element_type = sym->element_type;
        }
      }

      // Fallback: infer from variable name (temporary)
      if (!element_type &&
          target->expr.index.object->type == AST_EXPR_IDENTIFIER) {
        const char *var_name = target->expr.index.object->expr.identifier.name;
        if (strstr(var_name, "int") && !strstr(var_name, "char")) {
          element_type = LLVMInt64TypeInContext(ctx->context);
        }
      }

      if (!element_type) {
        fprintf(
            stderr,
            "Error: Could not determine element type for pointer indexing\n");
        return NULL;
      }

      // Return the address directly using GEP (don't load)
      return LLVMBuildGEP2(ctx->builder, element_type, object, &index, 1,
                           "element_addr");
    } else if (object_kind == LLVMArrayTypeKind) {
      // For array indexing
      LLVMValueRef array_ptr;

      if (target->expr.index.object->type == AST_EXPR_IDENTIFIER) {
        LLVM_Symbol *sym =
            find_symbol(ctx, target->expr.index.object->expr.identifier.name);
        if (sym && !sym->is_function) {
          array_ptr = sym->value;
        } else {
          return NULL;
        }
      } else {
        array_ptr =
            LLVMBuildAlloca(ctx->builder, object_type, "temp_array_ptr");
        LLVMBuildStore(ctx->builder, object, array_ptr);
      }

      LLVMValueRef indices[2];
      indices[0] = LLVMConstInt(LLVMInt32TypeInContext(ctx->context), 0, false);
      indices[1] = index;

      return LLVMBuildGEP2(ctx->builder, object_type, array_ptr, indices, 2,
                           "array_element_addr");
    }
  } else if (target->type == AST_EXPR_MEMBER) {
    // Handle &(obj.field) - taking address of a member access expression
    // This is needed for method calls like obj.method() where the typechecker
    // injects &obj as the first argument

    // Just evaluate the member access and allocate temporary storage for it
    LLVMValueRef member_value = codegen_expr_struct_access(ctx, target);
    if (!member_value) {
      fprintf(stderr,
              "Error: Failed to evaluate member access for address-of\n");
      return NULL;
    }

    LLVMTypeRef member_type = LLVMTypeOf(member_value);

    // Allocate temporary storage and store the value
    LLVMValueRef temp_storage =
        LLVMBuildAlloca(ctx->builder, member_type, "member_addr_temp");
    LLVMBuildStore(ctx->builder, member_value, temp_storage);

    // Return pointer to the temporary storage
    return temp_storage;
  }

  fprintf(stderr, "Error: Cannot take address of this expression type\n");
  return NULL;
}
