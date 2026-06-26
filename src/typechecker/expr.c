#include <stdio.h>
#include <string.h>

// #include "../ast/ast_utils.h"
#include "type.h"

AstNode *typecheck_binary_expr(AstNode *expr, Scope *scope,
                               ArenaAllocator *arena) {
  AstNode *left_type =
      typecheck_expression(expr->expr.binary.left, scope, arena);
  AstNode *right_type =
      typecheck_expression(expr->expr.binary.right, scope, arena);

  if (!left_type || !right_type) {
    tc_error(expr, "Type Error", "Failed to determine operand types");
    return NULL;
  }

  BinaryOp op = expr->expr.binary.op;

  // Arithmetic operators
  if (op >= BINOP_ADD && op <= BINOP_POW) {
    if (!is_numeric_type(left_type)) {
      tc_error_help(
          expr, "Type Error",
          "Arithmetic operations require numeric operands (int, float, double)",
          "Left operand has non-numeric type '%s'",
          type_to_string(left_type, arena));
      return NULL;
    }

    if (!is_numeric_type(right_type)) {
      tc_error_help(
          expr, "Type Error",
          "Arithmetic operations require numeric operands (int, float, double)",
          "Right operand has non-numeric type '%s'",
          type_to_string(right_type, arena));
      return NULL;
    }

    // Modulo requires integer types (not float/double)
    if (op == BINOP_MOD && (!is_integer_type(left_type) || !is_integer_type(right_type))) {
      tc_error_help(expr, "Type Error",
                    "Modulo requires integer operands (int, char)",
                    "Modulo operation with non-integer type");
      return NULL;
    }

    // Enhanced type promotion: double > float > int
    AstNode *double_type = create_basic_type(arena, "double", 0, 0);
    AstNode *float_type = create_basic_type(arena, "float", 0, 0);

    if (types_match(left_type, double_type) == TYPE_MATCH_EXACT ||
        types_match(right_type, double_type) == TYPE_MATCH_EXACT) {
      return create_basic_type(arena, "double", expr->line, expr->column);
    }

    if (types_match(left_type, float_type) == TYPE_MATCH_EXACT ||
        types_match(right_type, float_type) == TYPE_MATCH_EXACT) {
      return create_basic_type(arena, "float", expr->line, expr->column);
    }

    return create_basic_type(arena, "int", expr->line, expr->column);
  }

  // Comparison operators
  if (op >= BINOP_EQ && op <= BINOP_GE) {
    TypeMatchResult match = types_match(left_type, right_type);
    if (match == TYPE_MATCH_NONE) {
      tc_error_help(
          expr, "Type Error", "Comparison operands must be of compatible types",
          "Cannot compare incompatible types '%s' and '%s'",
          type_to_string(left_type, arena), type_to_string(right_type, arena));
      return NULL;
    }
    return create_basic_type(arena, "bool", expr->line, expr->column);
  }

  // Logical operators
  if (op == BINOP_AND || op == BINOP_OR) {
    if (!is_bool_type(left_type)) {
      tc_error_help(expr, "Type Error",
                    "Logical operators require boolean operands",
                    "Left operand has type '%s', expected 'bool'",
                    type_to_string(left_type, arena));
      return NULL;
    }
    if (!is_bool_type(right_type)) {
      tc_error_help(expr, "Type Error",
                    "Logical operators require boolean operands",
                    "Right operand has type '%s', expected 'bool'",
                    type_to_string(right_type, arena));
      return NULL;
    }
    return create_basic_type(arena, "bool", expr->line, expr->column);
  }

  // Range operators
  // In expr.c, update the range operator section:
  if (op == BINOP_RANGE) {
    // confirm that the left and right are of value int
    Type *elem_type = create_basic_type(arena, "int", 0, 0);
    if (types_match(left_type, elem_type) == TYPE_MATCH_NONE ||
        types_match(right_type, elem_type) == TYPE_MATCH_NONE) {
      tc_error_help(
          expr, "Type Error", "Range operator must take a type of int.",
          "Cannot create a range operation with two different types.");
    }

    int left = expr->expr.binary.left->expr.literal.value.int_val;
    int right = expr->expr.binary.right->expr.literal.value.int_val;

    long long size_val = 0;
    if (left < right) {
      size_val = right - left +
                 1; // FIXED: inclusive range (0..5 = 6 elements: 0,1,2,3,4,5)
    } else if (left > right) {
      size_val = left - right + 1; // FIXED: inclusive descending range
    } else {
      size_val = 1; // FIXED: single element (e.g., 5..5 = 1 element: 5)
    }

    Expr *size = create_literal_expr(arena, LITERAL_INT, &size_val, expr->line,
                                     expr->column);
    Type *array =
        create_array_type(arena, elem_type, size, expr->line, expr->column);
    return array;
  }

  // Bitwise operators (&, |, ^, <<, >>)
  if (op == BINOP_BIT_AND || op == BINOP_BIT_OR || op == BINOP_BIT_XOR ||
      op == BINOP_SHL || op == BINOP_SHR) {
    if (!is_numeric_type(left_type)) {
      tc_error_help(expr, "Type Error",
                    "Bitwise operations require integer operands",
                    "Left operand has non-integer type '%s'",
                    type_to_string(left_type, arena));
      return NULL;
    }

    if (!is_numeric_type(right_type)) {
      tc_error_help(expr, "Type Error",
                    "Bitwise operations require integer operands",
                    "Right operand has non-integer type '%s'",
                    type_to_string(right_type, arena));
      return NULL;
    }

    // Bitwise operations return int
    return create_basic_type(arena, "int", expr->line, expr->column);
  }

  tc_error(expr, "Unsupported Operation", "Unsupported binary operation");
  return NULL;
}

AstNode *typecheck_unary_expr(AstNode *expr, Scope *scope,
                              ArenaAllocator *arena) {
  AstNode *operand_type =
      typecheck_expression(expr->expr.unary.operand, scope, arena);
  if (!operand_type)
    return NULL;

  UnaryOp op = expr->expr.unary.op;

  if (op == UNOP_NEG) {
    if (!is_numeric_type(operand_type)) {
      tc_error(expr, "Type Error", "Unary negation on non-numeric type");
      return NULL;
    }
    return operand_type; // Negation does not change type
  }

  if (op == UNOP_POST_INC || op == UNOP_POST_DEC || op == UNOP_PRE_INC ||
      op == UNOP_PRE_DEC) {
    if (!is_numeric_type(operand_type)) {
      tc_error(expr, "Type Error", "Increment/decrement on non-numeric type");
      return NULL;
    }
    return operand_type; // Increment/decrement does not change type
  }

  if (op == UNOP_NOT) {
    // In many languages, logical NOT works with any type (truthy/falsy)
    return create_basic_type(arena, "bool", expr->line, expr->column);
  }

  if (op == UNOP_BIT_NOT) {
    if (!is_numeric_type(operand_type)) {
      tc_error(expr, "Type Error", "Bitwise NOT on non-integer type");
      return NULL;
    }
    return operand_type; // Bitwise NOT does not change type
  }

  tc_error(expr, "Type Error", "Unsupported unary operation");
  return NULL;
}

AstNode *typecheck_assignment_expr(AstNode *expr, Scope *scope,
                                   ArenaAllocator *arena) {
  AstNode *target_type =
      typecheck_expression(expr->expr.assignment.target, scope, arena);
  AstNode *value_type =
      typecheck_expression(expr->expr.assignment.value, scope, arena);

  if (!target_type || !value_type)
    return NULL;

  // Check that assignment target is mutable
  if (expr->expr.assignment.target->type == AST_EXPR_IDENTIFIER) {
    const char *var_name = expr->expr.assignment.target->expr.identifier.name;
    Symbol *sym = scope_lookup(scope, var_name);
    if (sym && !sym->is_mutable) {
      tc_error(expr, "Type Error",
               "Cannot assign to immutable variable '%s'", var_name);
      return NULL;
    }
  }

  // Determine function scope for ownership checks
  bool in_returns_ownership_func = false;
  Scope *func_scope = scope;
  while (func_scope && !func_scope->is_function_scope) {
    func_scope = func_scope->parent;
  }
  if (func_scope && func_scope->associated_node) {
    in_returns_ownership_func =
        func_scope->associated_node->stmt.func_decl.returns_ownership;
  }

  // Track allocations in assignments (e.g., a.buf = alloc(size))
  if (contains_alloc_expression(expr->expr.assignment.value)) {
    // Only track if NOT in a #returns_ownership function
    // AND NOT an indexed assignment (like array[i] = alloc(...))
    if (!in_returns_ownership_func &&
        expr->expr.assignment.target->type != AST_EXPR_INDEX) {
      StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
      if (analyzer) {
        const char *var_name =
            extract_variable_name(expr->expr.assignment.target);
        if (var_name) {
          const char *func_name = NULL;
          if (func_scope && func_scope->associated_node) {
            func_name = func_scope->associated_node->stmt.func_decl.name;
          }
          static_memory_track_alloc(analyzer, expr->line, expr->column,
                                    var_name, func_name, g_tokens,
                                    g_token_count, g_file_path);
        }
      }
    }
  }

  // Track ownership transfer from #returns_ownership calls in assignments
  if (expr->expr.assignment.value->type == AST_EXPR_CALL &&
      expr->expr.assignment.target->type == AST_EXPR_IDENTIFIER &&
      !in_returns_ownership_func) {
    AstNode *call_expr = expr->expr.assignment.value;
    AstNode *callee = call_expr->expr.call.callee;
    Symbol *func_symbol = NULL;

    if (callee->type == AST_EXPR_IDENTIFIER) {
      func_symbol = scope_lookup(scope, callee->expr.identifier.name);
    } else if (callee->type == AST_EXPR_MEMBER) {
      const char *base_name = callee->expr.member.object->expr.identifier.name;
      const char *member_name = callee->expr.member.member;
      if (callee->expr.member.is_compiletime) {
        func_symbol = lookup_qualified_symbol(scope, base_name, member_name);
      }
    }

    if (func_symbol && func_symbol->returns_ownership) {
      StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
      if (analyzer) {
        const char *var_name =
            extract_variable_name(expr->expr.assignment.target);
        const char *func_name = get_current_function_name(scope);
        if (var_name) {
          static_memory_track_alloc(analyzer, expr->line, expr->column,
                                    var_name, func_name, g_tokens,
                                    g_token_count, g_file_path);
        }
      }
    }
  }

  // Check for pointer assignment that might transfer ownership
  if (is_pointer_type(target_type) && is_pointer_type(value_type)) {
    // Extract variable names from both sides
    const char *target_var =
        extract_variable_name(expr->expr.assignment.target);
    const char *source_var = extract_variable_name(expr->expr.assignment.value);

    // CRITICAL FIX: Only track aliasing for direct variable-to-variable
    // assignments NOT for struct member assignments like node1.next = node2
    // Check if the target is a simple identifier (not a member access)
    bool is_direct_assignment =
        (expr->expr.assignment.target->type == AST_EXPR_IDENTIFIER);

    if (target_var && source_var && is_direct_assignment) {
      StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
      if (analyzer) {
        const char *func_name = get_current_function_name(scope);
        static_memory_track_alias(analyzer, target_var, source_var, func_name);
      }
    }
  }

  TypeMatchResult match = types_match(target_type, value_type);
  if (match == TYPE_MATCH_NONE) {
    if (target_type->type == AST_TYPE_FUNCTION &&
        is_pointer_to_function_type(value_type)) {
      tc_error_help(
          expr, "Function Type Mismatch",
          "The address-of operator creates a pointer to a function — "
          "assign the function directly without '&'",
          "Variable of type '%s' expects a function, "
          "but the address of a function of type '%s' was assigned",
          type_to_string(target_type, arena),
          type_to_string(value_type->type_data.pointer.pointee_type, arena));
    } else {
      const char *msg = "Type mismatch in assignment";
      tc_error_help(expr, "Type Mismatch", msg,
                    "Cannot assign value of type '%s' to variable of type '%s'",
                    type_to_string(value_type, arena),
                    type_to_string(target_type, arena));
    }
    return NULL;
  }

  return target_type;
}

AstNode *typecheck_call_expr(AstNode *expr, Scope *scope,
                             ArenaAllocator *arena) {
  AstNode *callee = expr->expr.call.callee;
  AstNode **arguments = expr->expr.call.args;
  size_t arg_count = expr->expr.call.arg_count;

  Symbol *func_symbol = NULL;
  const char *func_name = NULL;
  bool is_method_call = false;

  if (callee->type == AST_EXPR_IDENTIFIER) {
    func_name = callee->expr.identifier.name;
    func_symbol = scope_lookup(scope, func_name);
  } else if (callee->type == AST_EXPR_MEMBER) {
    const char *member_name = callee->expr.member.member;
    bool is_compiletime = callee->expr.member.is_compiletime;

    // CRITICAL FIX: Handle chained member access (e.g., p.instr.push_back)
    // First, typecheck the base expression to get its type
    AstNode *base_expr = callee->expr.member.object;
    const char *base_name = NULL;

    // Extract base_name for simple cases (still needed for module checking)
    if (base_expr->type == AST_EXPR_IDENTIFIER) {
      base_name = base_expr->expr.identifier.name;
    }

    if (is_compiletime) {
      // For compile-time access, we still need base_name
      if (!base_name) {
        tc_error(expr, "Compile-time Call Error",
                 "Compile-time access requires simple identifier base");
        return NULL;
      }

      // First try module lookup
      func_symbol = lookup_qualified_symbol(scope, base_name, member_name);
      func_name = member_name;

      // Try static method: "base::member"
      if (!func_symbol) {
        size_t static_ql =
            strlen(base_name) + strlen("::") + strlen(member_name) + 1;
        char *static_qn = arena_alloc(arena, static_ql, 1);
        snprintf(static_qn, static_ql, "%s::%s", base_name, member_name);
        func_symbol = scope_lookup(scope, static_qn);
      }

      // Try non-static method (accessible via :: but should error):
      // "base.member"
      if (!func_symbol) {
        size_t dot_ql = strlen(base_name) + strlen(member_name) + 2;
        char *dot_qn = arena_alloc(arena, dot_ql, 1);
        snprintf(dot_qn, dot_ql, "%s.%s", base_name, member_name);
        Symbol *dot_sym = scope_lookup(scope, dot_qn);
        if (dot_sym && dot_sym->type &&
            dot_sym->type->type == AST_TYPE_FUNCTION) {
          // Check if this is a non-static struct method
          Symbol *base_sym = scope_lookup(scope, base_name);
          if (base_sym && base_sym->type &&
              base_sym->type->type == AST_TYPE_STRUCT) {
            AstNode *mtype =
                get_struct_member_type(base_sym->type, member_name);
            if (mtype && mtype->type == AST_TYPE_FUNCTION) {
              tc_error(expr, "Compile-time Call Error",
                       "Method '%s' is not static, call it on an instance",
                       member_name);
              return NULL;
            }
          }
          // It's a callable (function-typed) enum member or module symbol
          func_symbol = dot_sym;
        }
      }

      if (!func_symbol) {
        tc_error(expr, "Compile-time Call Error",
                 "No compile-time callable '%s::%s' found", base_name,
                 member_name);
        return NULL;
      }
    } else {
      // Runtime access - check if it's a module access (only for simple
      // identifiers)
      bool is_module_access = false;
      if (base_name) {
        Scope *current = scope;
        while (current) {
          for (size_t i = 0; i < current->imported_modules.count; i++) {
            ModuleImport *import =
                (ModuleImport *)((char *)current->imported_modules.data +
                                 i * sizeof(ModuleImport));
            if (strcmp(import->alias, base_name) == 0) {
              is_module_access = true;
              break;
            }
          }
          if (is_module_access)
            break;
          current = current->parent;
        }

        if (is_module_access) {
          tc_error_help(
              expr, "Access Method Error",
              "Use '::' for compile-time access to module functions",
              "Cannot use runtime access '.' for module function - did "
              "you mean '%s::%s()'?",
              base_name, member_name);
          return NULL;
        }
      }

      // CRITICAL FIX: Typecheck the base expression to handle complex cases
      // like p.instr.push_back where base_expr is AST_EXPR_MEMBER
      AstNode *base_type = typecheck_expression(base_expr, scope, arena);
      if (!base_type) {
        tc_error(expr, "Runtime Access Error",
                 "Failed to determine type of base expression for method call");
        return NULL;
      }

      // Resolve base_type to actual struct type (handle pointers, basic types,
      // etc.)
      if (base_type->type == AST_TYPE_POINTER) {
        AstNode *pointee = base_type->type_data.pointer.pointee_type;
        if (pointee && pointee->type == AST_TYPE_BASIC) {
          Symbol *struct_symbol =
              scope_lookup(scope, pointee->type_data.basic.name);
          if (struct_symbol && struct_symbol->type &&
              struct_symbol->type->type == AST_TYPE_STRUCT) {
            base_type = struct_symbol->type;
          }
        } else if (pointee && pointee->type == AST_TYPE_STRUCT) {
          base_type = pointee;
        }
      }

      if (base_type->type == AST_TYPE_BASIC) {
        Symbol *struct_symbol =
            scope_lookup(scope, base_type->type_data.basic.name);
        if (struct_symbol && struct_symbol->type &&
            struct_symbol->type->type == AST_TYPE_STRUCT) {
          base_type = struct_symbol->type;
        }
      }

      if (base_type->type == AST_TYPE_STRUCT) {
        AstNode *member_type = get_struct_member_type(base_type, member_name);

        if (member_type && member_type->type == AST_TYPE_FUNCTION) {
          func_symbol = arena_alloc(arena, sizeof(Symbol), alignof(Symbol));
          if (!func_symbol) {
            tc_error(expr, "Memory Error",
                     "Failed to allocate symbol for method '%s'", member_name);
            return NULL;
          }

          func_symbol->name = member_name;
          func_symbol->type = member_type;
          func_symbol->is_public = true;
          func_symbol->is_mutable = false;
          func_symbol->scope_depth = 0;
          func_symbol->returns_ownership = false;
          func_symbol->takes_ownership = false;
          func_name = member_name;

          // CRITICAL FIX: Use the base expression directly, not just base_name
          // This handles complex expressions like p.instr.push_back
          if (!base_expr) {
            tc_error(expr, "Internal Error", "Method call has no object");
            return NULL;
          }

          size_t new_arg_count = arg_count + 1;
          AstNode **new_arguments = arena_alloc(
              arena, new_arg_count * sizeof(AstNode *), alignof(AstNode *));
          if (!new_arguments) {
            tc_error(expr, "Memory Error",
                     "Failed to allocate arguments array for method call");
            return NULL;
          }

          if (!member_type || member_type->type != AST_TYPE_FUNCTION) {
            tc_error(expr, "Internal Error", "Method type is not a function");
            return NULL;
          }

          AstNode **method_param_types =
              member_type->type_data.function.param_types;
          if (!method_param_types ||
              member_type->type_data.function.param_count == 0) {
            tc_error(expr, "Internal Error",
                     "Method has no parameters (missing self?)");
            return NULL;
          }

          AstNode *self_param_type = method_param_types[0];
          bool expects_pointer = (self_param_type->type == AST_TYPE_POINTER);
          bool have_pointer =
              (base_type->type == AST_TYPE_POINTER ||
               (base_expr->type == AST_EXPR_IDENTIFIER &&
                scope_lookup(scope, base_expr->expr.identifier.name) &&
                scope_lookup(scope, base_expr->expr.identifier.name)->type &&
                scope_lookup(scope, base_expr->expr.identifier.name)
                        ->type->type == AST_TYPE_POINTER));

          if (expects_pointer && !have_pointer) {
            AstNode *addr_expr =
                arena_alloc(arena, sizeof(AstNode), alignof(AstNode));
            addr_expr->type = AST_EXPR_ADDR;
            addr_expr->category = Node_Category_EXPR;
            addr_expr->line = base_expr->line;
            addr_expr->column = base_expr->column;
            addr_expr->expr.addr.object = base_expr;
            new_arguments[0] = addr_expr;
          } else {
            new_arguments[0] = base_expr;
          }

          for (size_t i = 0; i < arg_count; i++) {
            new_arguments[i + 1] = arguments[i];
          }

          arguments = new_arguments;
          arg_count = new_arg_count;
          is_method_call = true;
          expr->expr.call.args = new_arguments;
          expr->expr.call.arg_count = new_arg_count;
        } else if (member_type) {
          tc_error(expr, "Runtime Call Error",
                   "Cannot call non-function member '%s' on struct '%s'",
                   member_name, base_type->type_data.struct_type.name);
          return NULL;
        } else {
          // Check for privately inherited method
          const char *sc_name = base_type->type_data.struct_type.name;
          size_t iql = strlen(sc_name) + strlen(".") + strlen(member_name) + 1;
          char *iqn = arena_alloc(arena, iql, 1);
          snprintf(iqn, iql, "%s.%s", sc_name, member_name);
          Symbol *isym = scope_lookup(scope, iqn);
          if (isym && isym->type && isym->type->type == AST_TYPE_FUNCTION) {
            Symbol *self_sym = scope_lookup(scope, "self");
            if (self_sym && self_sym->type &&
                self_sym->type->type == AST_TYPE_POINTER) {
              AstNode *pointee = self_sym->type->type_data.pointer.pointee_type;
              if (pointee && pointee->type == AST_TYPE_STRUCT &&
                  strcmp(pointee->type_data.struct_type.name, sc_name) == 0) {
                func_symbol = isym;
                func_name = member_name;
                is_method_call = true;
                // Prepend self argument
                size_t nac = arg_count + 1;
                AstNode **na = arena_alloc(
                    arena, nac * sizeof(AstNode *), alignof(AstNode *));
                na[0] = base_expr;
                for (size_t j = 0; j < arg_count; j++)
                  na[j + 1] = arguments[j];
                arguments = na;
                arg_count = nac;
                goto process_call;
              }
            }
            tc_error_help(expr, "Runtime Call Error",
                          "This method is privately inherited and cannot "
                          "be called externally",
                          "Method '%s' is privately inherited by struct '%s'",
                          member_name, sc_name);
            return NULL;
          }
          tc_error(expr, "Runtime Call Error", "Struct '%s' has no member '%s'",
                   sc_name, member_name);
          return NULL;
        }
      } else {
        tc_error(expr, "Runtime Call Error",
                 "Cannot use runtime access '.' on non-struct type '%s'",
                 type_to_string(base_type, arena));
        return NULL;
      }
    }
  } else {
    tc_error(expr, "Call Error", "Unsupported callee type for function call");
    return NULL;
  }

  process_call:
  if (!func_symbol) {
    tc_error(expr, "Call Error", "Undefined function '%s'",
             func_name ? func_name : "unknown");
    return NULL;
  }

  AstNode *func_type = func_symbol->type;
  if (func_type->type != AST_TYPE_FUNCTION) {
    tc_error_help(expr, "Call Error",
                  "Only functions and function-typed variables can be called",
                  "'%s' is not callable — it has type '%s'",
                  func_name ? func_name : "expression",
                  type_to_string(func_type, arena));
    return NULL;
  }

  AstNode **param_types = func_type->type_data.function.param_types;
  size_t param_count = func_type->type_data.function.param_count;
  AstNode *return_type = func_type->type_data.function.return_type;

  if (arg_count != param_count) {
    tc_error(expr, "Call Error", "Function '%s' expects %zu arguments, got %zu",
             func_name, param_count, arg_count);
    return NULL;
  }

  size_t start_index = is_method_call ? 1 : 0;
  for (size_t i = start_index; i < arg_count; i++) {
    if (!arguments || !arguments[i]) {
      tc_error(expr, "Call Error", "Argument %zu in call to '%s' is NULL",
               i + 1, func_name);
      return NULL;
    }

    AstNode *arg_type = typecheck_expression(arguments[i], scope, arena);
    if (!arg_type) {
      tc_error(expr, "Call Error",
               "Failed to type-check argument %zu in call to '%s'", i + 1,
               func_name);
      return NULL;
    }

    if (!param_types || !param_types[i]) {
      tc_error(expr, "Call Error",
               "Parameter %zu type in function '%s' is NULL", i + 1, func_name);
      return NULL;
    }

    TypeMatchResult match = types_match(param_types[i], arg_type);

    // NEW: Special handling for array argument padding
    if (match == TYPE_MATCH_NONE && param_types[i]->type == AST_TYPE_ARRAY &&
        arg_type->type == AST_TYPE_ARRAY) {

      TypeMatchResult element_match =
          types_match(param_types[i]->type_data.array.element_type,
                      arg_type->type_data.array.element_type);

      if (element_match != TYPE_MATCH_NONE) {
        AstNode *param_size = param_types[i]->type_data.array.size;
        AstNode *arg_size = arg_type->type_data.array.size;

        if (param_size && arg_size && param_size->type == AST_EXPR_LITERAL &&
            arg_size->type == AST_EXPR_LITERAL &&
            param_size->expr.literal.lit_type == LITERAL_INT &&
            arg_size->expr.literal.lit_type == LITERAL_INT) {

          long long param_size_val = param_size->expr.literal.value.int_val;
          long long arg_size_val = arg_size->expr.literal.value.int_val;

          if (arg_size_val <= param_size_val) {
            if (arguments[i]->type == AST_EXPR_ARRAY) {
              arguments[i]->expr.array.target_size = (size_t)param_size_val;
            }
            continue;
          } else {
            tc_error_help(
                expr, "Array Size Mismatch",
                "Cannot pass larger array to function expecting smaller array",
                "Argument %zu: parameter expects array of size %lld, got size "
                "%lld",
                i + 1, param_size_val, arg_size_val);
            return NULL;
          }
        }
      }
    }

    if (match == TYPE_MATCH_NONE) {
      if (param_types[i]->type == AST_TYPE_FUNCTION &&
          arg_type->type == AST_TYPE_FUNCTION) {
        tc_error_help(expr, "Function Signature Mismatch",
                      "The function passed must have a matching signature",
                      "Argument %zu to '%s': expected '%s', got '%s'", i + 1,
                      func_name ? func_name : "function",
                      type_to_string(param_types[i], arena),
                      type_to_string(arg_type, arena));
      } else if (param_types[i]->type == AST_TYPE_FUNCTION) {
        // Expected a function, got something else entirely
        tc_error_help(
            expr, "Call Error",
            "This parameter expects a function — pass a function or "
            "function-typed variable",
            "Argument %zu to '%s': expected a function of type '%s', got '%s'",
            i + 1, func_name ? func_name : "function",
            type_to_string(param_types[i], arena),
            type_to_string(arg_type, arena));
      } else if (arg_type->type == AST_TYPE_FUNCTION) {
        // Passed a function where a plain value was expected
        tc_error_help(
            expr, "Call Error",
            "A function was passed where a value is expected",
            "Argument %zu to '%s': expected '%s', got function of type '%s'",
            i + 1, func_name ? func_name : "function",
            type_to_string(param_types[i], arena),
            type_to_string(arg_type, arena));
      } else {
        // Original generic message for non-function mismatches
        tc_error_help(expr, "Call Error",
                      "The argument type does not match the expected parameter type",
                      "Argument %zu to function '%s': expected '%s', got '%s'",
                      i + 1, func_name ? func_name : "function",
                      type_to_string(param_types[i], arena),
                      type_to_string(arg_type, arena));
      }
      return NULL;
    }
  }

  // Handle ownership transfer from caller to function
  // CRITICAL FIX: Properly handle defer blocks for #takes_ownership
  if (func_symbol->takes_ownership) {
    StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);

    for (size_t i = 0; i < arg_count; i++) {
      if (param_types[i] && is_pointer_type(param_types[i])) {
        const char *arg_var = extract_variable_name(arguments[i]);
        if (arg_var && analyzer) {

          // Check if we're in a defer block
          if (analyzer->skip_memory_tracking) {
            // We're in a defer block - add to deferred_frees list
            Scope *func_scope = scope;
            while (func_scope && !func_scope->is_function_scope) {
              func_scope = func_scope->parent;
            }

            if (func_scope) {
              // Check if already in deferred list (avoid duplicates)
              bool already_deferred = false;
              for (size_t j = 0; j < func_scope->deferred_frees.count; j++) {
                const char **existing =
                    (const char **)((char *)func_scope->deferred_frees.data +
                                    j * sizeof(const char *));
                if (*existing && strcmp(*existing, arg_var) == 0) {
                  already_deferred = true;
                  break;
                }
              }

              if (!already_deferred) {
                const char **slot = (const char **)growable_array_push(
                    &func_scope->deferred_frees);
                if (slot) {
                  *slot = arg_var;
                }
              }
            }
          } else {
            // Normal (non-defer) call - track immediately as freed
            const char *current_func = get_current_function_name(scope);
            static_memory_track_free(analyzer, arg_var, current_func,
                                     !scope->is_function_scope);
          }
        }
      }
    }
  }

  // ADDITION: For method calls with #takes_ownership, track ownership transfer
  // of 'self'
  if (is_method_call && func_symbol->takes_ownership) {
    if (arguments[0]) {
      const char *self_var = NULL;

      if (arguments[0]->type == AST_EXPR_ADDR) {
        self_var = extract_variable_name(arguments[0]->expr.addr.object);
      } else {
        self_var = extract_variable_name(arguments[0]);
      }

      if (self_var) {
        StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
        if (analyzer) {
          const char *current_func = get_current_function_name(scope);
          static_memory_track_free(analyzer, self_var, current_func,
                                   !scope->is_function_scope);
        }
      }
    }
  }

  return return_type;
}

AstNode *typecheck_index_expr(AstNode *expr, Scope *scope,
                              ArenaAllocator *arena) {
  if (expr->type != AST_EXPR_INDEX) {
    tc_error(expr, "Internal Error", "Expected index expression node");
    return NULL;
  }

  if (expr->expr.index.object->type == AST_EXPR_IDENTIFIER) {
    StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
    if (analyzer && g_tokens && g_token_count > 0 && g_file_path) {
      const char *var_name = expr->expr.index.object->expr.identifier.name;

      // NEW: Get current function name
      const char *func_name = NULL;
      Scope *func_scope = scope;
      while (func_scope && !func_scope->is_function_scope) {
        func_scope = func_scope->parent;
      }
      if (func_scope && func_scope->associated_node) {
        func_name = func_scope->associated_node->stmt.func_decl.name;
      }

      static_memory_check_use_after_free(analyzer, var_name,
                                         expr->expr.index.object->line,
                                         expr->expr.index.object->column, arena,
                                         g_tokens, g_token_count, g_file_path,
                                         func_name); // PASS FUNC NAME
    }
  }

  AstNode *object_type =
      typecheck_expression(expr->expr.index.object, scope, arena);
  if (!object_type) {
    tc_error(expr, "Index Error",
             "Failed to determine type of indexed expression");
    return NULL;
  }

  AstNode *index_type =
      typecheck_expression(expr->expr.index.index, scope, arena);
  if (!index_type) {
    tc_error(expr, "Index Error",
             "Failed to determine type of index expression");
    return NULL;
  }

  if (!is_integer_type(index_type)) {
    tc_error_help(
        expr, "Index Type Error",
        "Array and pointer indices must be integer types (int or char)",
        "Index has type '%s', expected integer type",
        type_to_string(index_type, arena));
    return NULL;
  }

  if (object_type->type == AST_TYPE_ARRAY) {
    AstNode *element_type = object_type->type_data.array.element_type;
    if (!element_type) {
      tc_error(expr, "Array Index Error", "Array has invalid element type");
      return NULL;
    }
    return element_type;

  } else if (object_type->type == AST_TYPE_POINTER) {
    AstNode *pointee_type = object_type->type_data.pointer.pointee_type;
    if (!pointee_type) {
      tc_error(expr, "Pointer Index Error", "Pointer has invalid pointee type");
      return NULL;
    }
    return pointee_type;

  } else if (object_type->type == AST_TYPE_BASIC &&
             strcmp(object_type->type_data.basic.name, "string") == 0) {
    return create_basic_type(arena, "char", expr->line, expr->column);

  } else {
    tc_error_help(expr, "Index Error",
                  "Only arrays, pointers, and strings can be indexed",
                  "Cannot index expression of type '%s'",
                  type_to_string(object_type, arena));
    return NULL;
  }
}

AstNode *typecheck_member_expr(AstNode *expr, Scope *scope,
                               ArenaAllocator *arena) {
  // Get the base object and member name
  AstNode *base_object = expr->expr.member.object;
  const char *member_name = expr->expr.member.member;
  bool is_compiletime = expr->expr.member.is_compiletime;

  if (is_compiletime) {
    // Compile-time access (::) - for modules, enums, static members

    // CRITICAL FIX: Handle chained access (e.g., ast::ExprKind::EXPR_NUMBER)
    // First, resolve the base to its type
    AstNode *base_type = NULL;
    const char *base_name = NULL;

    if (base_object->type == AST_EXPR_IDENTIFIER) {
      // Simple case: module::symbol or EnumType::Member
      base_name = base_object->expr.identifier.name;

      // First try module-qualified symbol lookup
      Symbol *module_symbol =
          lookup_qualified_symbol(scope, base_name, member_name);
      if (module_symbol) {
        return module_symbol->type;
      }

      // Try direct symbol lookup (for enum types in current scope)
      Symbol *base_symbol = scope_lookup(scope, base_name);
      if (base_symbol && base_symbol->type) {
        base_type = base_symbol->type;
        // base_name already set above
      } else {
        // Check if it's an undefined module
        bool is_module = false;
        Scope *current = scope;
        while (current && !is_module) {
          for (size_t i = 0; i < current->imported_modules.count; i++) {
            ModuleImport *import =
                (ModuleImport *)((char *)current->imported_modules.data +
                                 i * sizeof(ModuleImport));
            if (strcmp(import->alias, base_name) == 0) {
              is_module = true;
              break;
            }
          }
          current = current->parent;
        }

        if (is_module) {
          tc_error(expr, "Compile-time Access Error",
                   "Module '%s' has no exported symbol '%s'", base_name,
                   member_name);
        } else {
          tc_error(expr, "Compile-time Access Error",
                   "Undefined identifier '%s' in compile-time access",
                   base_name);
        }
        return NULL;
      }

    } else if (base_object->type == AST_EXPR_MEMBER) {
      // Chained case: ast::ExprKind::EXPR_NUMBER
      // First resolve the inner member expression recursively
      base_type = typecheck_member_expr(base_object, scope, arena);
      if (!base_type) {
        tc_error(expr, "Compile-time Access Error",
                 "Failed to resolve base expression in chained access");
        return NULL;
      }

      // CRITICAL FIX: Extract the name from the resolved type
      // The resolved type should be a basic type (enum type name)
      if (base_type->type == AST_TYPE_BASIC) {
        base_name = base_type->type_data.basic.name;
      } else if (base_type->type == AST_TYPE_STRUCT) {
        base_name = base_type->type_data.struct_type.name;
      } else {
        tc_error(expr, "Compile-time Access Error",
                 "Cannot use compile-time access '::' on complex type '%s'",
                 type_to_string(base_type, arena));
        return NULL;
      }

      // Look up the qualified symbol (e.g., "EnumType.MEMBER")
      // First try "base.member" (enum members, non-static methods)
      size_t dot_qualified_len = strlen(base_name) + strlen(member_name) + 2;
      char *dot_qualified_name = arena_alloc(arena, dot_qualified_len, 1);
      snprintf(dot_qualified_name, dot_qualified_len, "%s.%s", base_name,
               member_name);

      Symbol *member_symbol = scope_lookup(scope, dot_qualified_name);

      if (!member_symbol) {
        Scope *current = scope;
        while (current && !member_symbol) {
          for (size_t i = 0; i < current->imported_modules.count; i++) {
            ModuleImport *import =
                (ModuleImport *)((char *)current->imported_modules.data +
                                 i * sizeof(ModuleImport));

            member_symbol = scope_lookup_current_only_with_visibility(
                import->module_scope, dot_qualified_name, scope);

            if (member_symbol) {
              break;
            }
          }
          current = current->parent;
        }
      }

      if (member_symbol) {
        if (member_symbol->type &&
            member_symbol->type->type == AST_TYPE_FUNCTION &&
            base_type && base_type->type == AST_TYPE_STRUCT) {
          AstNode *mtype = get_struct_member_type(base_type, member_name);
          if (mtype && mtype->type == AST_TYPE_FUNCTION) {
            tc_error(expr, "Compile-time Access Error",
                     "Method '%s' is not static, call it on an instance",
                     member_name);
            return NULL;
          }
        }
        return member_symbol->type;
      }

      // Try "base::member" for static methods
      size_t static_qualified_len =
          strlen(base_name) + strlen("::") + strlen(member_name) + 1;
      char *static_qualified_name =
          arena_alloc(arena, static_qualified_len, 1);
      snprintf(static_qualified_name, static_qualified_len, "%s::%s",
               base_name, member_name);

      Symbol *static_symbol = scope_lookup(scope, static_qualified_name);
      if (static_symbol) {
        return static_symbol->type;
      }

      tc_error(expr, "Compile-time Access Error",
               "Type '%s' has no compile-time member '%s'", base_name,
               member_name);
      return NULL;

    } else {
      tc_error(expr, "Compile-time Access Error",
               "Compile-time access '::' requires an identifier or member "
               "expression on the left");
      return NULL;
    }

    // Now we have base_name and member_name - look up the qualified symbol
    // Try enum-style lookup (EnumName::Member or Module::Type)
    // For structs, this also finds non-static methods (StructName.methodName)
    // Non-static methods found via :: should error — use dot access instead

    // First try "base_name.member_name" (non-static methods, enum members)
    size_t dot_qualified_len = strlen(base_name) + strlen(member_name) + 2;
    char *dot_qualified_name = arena_alloc(arena, dot_qualified_len, 1);
    snprintf(dot_qualified_name, dot_qualified_len, "%s.%s", base_name,
             member_name);

    Symbol *member_symbol = scope_lookup(scope, dot_qualified_name);

    if (!member_symbol) {
      // Try looking in imported modules explicitly
      Scope *current = scope;
      while (current && !member_symbol) {
        for (size_t i = 0; i < current->imported_modules.count; i++) {
          ModuleImport *import =
              (ModuleImport *)((char *)current->imported_modules.data +
                               i * sizeof(ModuleImport));

          member_symbol = scope_lookup_current_only_with_visibility(
              import->module_scope, dot_qualified_name, scope);

          if (member_symbol) {
            break;
          }
        }
        current = current->parent;
      }
    }

    if (member_symbol) {
      // If we found a non-static method via :: access, that's an error
      // (non-static methods must be called via dot on an instance)
      if (member_symbol->type &&
          member_symbol->type->type == AST_TYPE_FUNCTION) {
        // Check if this is a non-static struct method
        // (rather than an enum member or module symbol)
        if (base_type && base_type->type == AST_TYPE_STRUCT) {
          AstNode *member_type =
              get_struct_member_type(base_type, member_name);
          if (member_type && member_type->type == AST_TYPE_FUNCTION) {
            // This is a non-static method accessed via ::
            tc_error(expr, "Compile-time Access Error",
                     "Method '%s' is not static, call it on an instance",
                     member_name);
            return NULL;
          }
        }
      }
      // Enum members and other compile-time symbols are fine
      return member_symbol->type;
    }

    // Try "base_name::member_name" for static methods
    size_t static_qualified_len =
        strlen(base_name) + strlen("::") + strlen(member_name) + 1;
    char *static_qualified_name =
        arena_alloc(arena, static_qualified_len, 1);
    snprintf(static_qualified_name, static_qualified_len, "%s::%s", base_name,
             member_name);

    Symbol *static_symbol = scope_lookup(scope, static_qualified_name);
    if (static_symbol) {
      return static_symbol->type;
    }

    // Error handling - symbol not found
    tc_error(expr, "Compile-time Access Error",
             "Type '%s' has no compile-time member '%s'", base_name,
             member_name);
    return NULL;

  } else {
    // Runtime access (.) - for struct members, instance data

    // CRITICAL FIX: Typecheck the base expression first
    // This handles complex expressions like lex.list[i]
    AstNode *base_type = typecheck_expression(base_object, scope, arena);
    if (!base_type) {
      tc_error(expr, "Runtime Access Error",
               "Failed to determine type of base expression");
      return NULL;
    }

    // Check use-after-free for pointer-to-struct member access
    if (base_type->type == AST_TYPE_POINTER &&
        base_object->type == AST_EXPR_IDENTIFIER) {
      StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
      if (analyzer && g_tokens && g_token_count > 0 && g_file_path) {
        const char *var_name = base_object->expr.identifier.name;
        const char *func_name = NULL;
        Scope *func_scope = scope;
        while (func_scope && !func_scope->is_function_scope) {
          func_scope = func_scope->parent;
        }
        if (func_scope && func_scope->associated_node) {
          func_name = func_scope->associated_node->stmt.func_decl.name;
        }
        static_memory_check_use_after_free(analyzer, var_name,
                                           base_object->line,
                                           base_object->column, arena,
                                           g_tokens, g_token_count,
                                           g_file_path, func_name);
      }
    }

    // Handle pointer dereference: if we have a pointer to struct, automatically
    // dereference it
    if (base_type->type == AST_TYPE_POINTER) {
      AstNode *pointee = base_type->type_data.pointer.pointee_type;
      if (pointee && pointee->type == AST_TYPE_BASIC) {
        const char *type_name = pointee->type_data.basic.name;

        // Try lookup through normal scope chain first
        Symbol *struct_symbol = scope_lookup(scope, type_name);

        // If not found through normal lookup, explicitly search imported
        // modules
        if (!struct_symbol) {
          Scope *current = scope;
          while (current && !struct_symbol) {
            for (size_t i = 0; i < current->imported_modules.count; i++) {
              ModuleImport *import =
                  (ModuleImport *)((char *)current->imported_modules.data +
                                   i * sizeof(ModuleImport));

              // CRITICAL FIX: Use full scope_lookup with visibility
              struct_symbol = scope_lookup_with_visibility(import->module_scope,
                                                           type_name, scope);

              if (struct_symbol && struct_symbol->type &&
                  struct_symbol->type->type == AST_TYPE_STRUCT) {
                break;
              }
              struct_symbol = NULL;
            }
            current = current->parent;
          }
        }

        if (struct_symbol && struct_symbol->type &&
            struct_symbol->type->type == AST_TYPE_STRUCT) {
          base_type = struct_symbol->type; // Use the struct type
        }
      } else if (pointee && pointee->type == AST_TYPE_STRUCT) {
        base_type = pointee;
      }
    }

    // Handle case where base_type is a basic type that references a struct
    if (base_type->type == AST_TYPE_BASIC) {
      const char *type_name = base_type->type_data.basic.name;

      // Try lookup through normal scope chain first
      Symbol *struct_symbol = scope_lookup(scope, type_name);

      // If not found through normal lookup, explicitly search imported modules
      if (!struct_symbol) {
        Scope *current = scope;
        while (current && !struct_symbol) {
          for (size_t i = 0; i < current->imported_modules.count; i++) {
            ModuleImport *import =
                (ModuleImport *)((char *)current->imported_modules.data +
                                 i * sizeof(ModuleImport));

            // CRITICAL FIX: Use full scope_lookup with visibility
            struct_symbol = scope_lookup_with_visibility(import->module_scope,
                                                         type_name, scope);

            if (struct_symbol && struct_symbol->type &&
                struct_symbol->type->type == AST_TYPE_STRUCT) {
              break;
            }
            struct_symbol = NULL;
          }
          current = current->parent;
        }
      }

      if (struct_symbol && struct_symbol->type &&
          struct_symbol->type->type == AST_TYPE_STRUCT) {
        base_type = struct_symbol->type; // Use the actual struct type
      }
    }

    // Now check if it's a struct type and get the member
    if (base_type->type == AST_TYPE_STRUCT) {
      AstNode *member_type = get_struct_member_type(base_type, member_name);
      if (member_type) {
        return member_type;
      }

      // Check for privately inherited method (registered as "Struct.method")
      const char *struct_name_c = base_type->type_data.struct_type.name;
      size_t inh_ql = strlen(struct_name_c) + strlen(".") + strlen(member_name) + 1;
      char *inh_qn = arena_alloc(arena, inh_ql, 1);
      snprintf(inh_qn, inh_ql, "%s.%s", struct_name_c, member_name);
      Symbol *inh_sym = scope_lookup(scope, inh_qn);
      if (inh_sym && inh_sym->type && inh_sym->type->type == AST_TYPE_FUNCTION) {
        // Check if we're inside a method of this struct (private inheritance)
        Symbol *self_sym = scope_lookup(scope, "self");
        if (self_sym && self_sym->type &&
            self_sym->type->type == AST_TYPE_POINTER) {
          AstNode *pointee = self_sym->type->type_data.pointer.pointee_type;
          if (pointee && pointee->type == AST_TYPE_STRUCT &&
              strcmp(pointee->type_data.struct_type.name, struct_name_c) == 0) {
            return inh_sym->type;
          }
        }
        // External access to privately inherited method
        tc_error_help(expr, "Runtime Access Error",
                      "This method is privately inherited and cannot be called "
                      "externally",
                      "Method '%s' is privately inherited by struct '%s' and "
                      "cannot be called from outside",
                      member_name, struct_name_c);
        return NULL;
      }

      // Member doesn't exist — check if it's a static method (hint)
      size_t hint_len = strlen(struct_name_c) + strlen("::") +
                        strlen(member_name) + 1;
      char *hint_qualified = arena_alloc(arena, hint_len, 1);
      snprintf(hint_qualified, hint_len, "%s::%s", struct_name_c, member_name);
      Symbol *hint_symbol = scope_lookup(scope, hint_qualified);

      if (hint_symbol) {
        tc_error_help(expr, "Runtime Access Error",
                      "Use '::' to call static methods",
                      "Member '%s' is a static method on struct '%s' — use "
                      "'%s::%s' instead",
                      member_name, struct_name_c, struct_name_c, member_name);
      } else {
        tc_error(expr, "Runtime Access Error",
                 "Struct '%s' has no member '%s'", struct_name_c, member_name);
      }
      return NULL;

    } else {
      // Base is not a struct - runtime access is invalid
      tc_error(expr, "Runtime Access Error",
               "Cannot use runtime access '.' on non-struct type '%s'",
               type_to_string(base_type, arena));
      return NULL;
    }
  }
}
AstNode *typecheck_deref_expr(AstNode *expr, Scope *scope,
                              ArenaAllocator *arena) {
  if (expr->expr.deref.object->type == AST_EXPR_IDENTIFIER) {
    StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
    if (analyzer && g_tokens && g_token_count > 0 && g_file_path) {
      const char *var_name = expr->expr.deref.object->expr.identifier.name;

      // Get current function name
      const char *func_name = NULL;
      Scope *func_scope = scope;
      while (func_scope && !func_scope->is_function_scope) {
        func_scope = func_scope->parent;
      }
      if (func_scope && func_scope->associated_node) {
        func_name = func_scope->associated_node->stmt.func_decl.name;
      }

      static_memory_check_use_after_free(analyzer, var_name,
                                         expr->expr.deref.object->line,
                                         expr->expr.deref.object->column, arena,
                                         g_tokens, g_token_count, g_file_path,
                                         func_name); // ADD THIS PARAMETER
    }
  }

  AstNode *pointer_type =
      typecheck_expression(expr->expr.deref.object, scope, arena);
  if (!pointer_type) {
    tc_error(expr, "Type Error",
             "Failed to type-check dereferenced expression");
    return NULL;
  }
  if (pointer_type->type != AST_TYPE_POINTER) {
    tc_error(expr, "Type Error", "Cannot dereference non-pointer type");
    return NULL;
  }
  return pointer_type->type_data.pointer.pointee_type;
}

static bool is_lvalue(AstNode *expr) {
  if (!expr)
    return false;
  switch (expr->type) {
  case AST_EXPR_IDENTIFIER:
  case AST_EXPR_MEMBER:
  case AST_EXPR_INDEX:
  case AST_EXPR_DEREF:
    return true;
  default:
    return false;
  }
}

AstNode *typecheck_addr_expr(AstNode *expr, Scope *scope,
                             ArenaAllocator *arena) {
  if (!is_lvalue(expr->expr.addr.object)) {
    tc_error(expr, "Type Error",
             "Cannot take address of non-lvalue expression");
    return NULL;
  }

  // Track that a tracked variable's address was taken
  // (means ownership might have been transferred to a container/function)
  if (expr->expr.addr.object->type == AST_EXPR_IDENTIFIER) {
    const char *var_name = expr->expr.addr.object->expr.identifier.name;
    StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);
    if (analyzer) {
      const char *func_name = get_current_function_name(scope);
      static_memory_mark_addr_taken(analyzer, var_name, func_name);
    }
  }

  AstNode *base_type =
      typecheck_expression(expr->expr.addr.object, scope, arena);
  if (!base_type) {
    tc_error(expr, "Type Error", "Failed to type-check address-of expression");
    return NULL;
  }
  AstNode *pointer_type =
      create_pointer_type(arena, base_type, expr->line, expr->column);
  return pointer_type;
}

AstNode *typecheck_alloc_expr(AstNode *expr, Scope *scope,
                              ArenaAllocator *arena) {
  // Verify size argument is numeric
  AstNode *size_type =
      typecheck_expression(expr->expr.alloc.size, scope, arena);
  if (!size_type) {
    tc_error(expr, "Type Error", "Cannot determine type for alloc size");
    return NULL;
  }

  if (!is_numeric_type(size_type)) {
    tc_error(expr, "Type Error", "alloc size must be numeric type");
    return NULL;
  }

  // alloc returns void* (generic pointer)
  AstNode *void_type =
      create_basic_type(arena, "void", expr->line, expr->column);
  return create_pointer_type(arena, void_type, expr->line, expr->column);
}

AstNode *typecheck_free_expr(AstNode *expr, Scope *scope,
                             ArenaAllocator *arena) {
  AstNode *ptr_type = typecheck_expression(expr->expr.free.ptr, scope, arena);
  if (!ptr_type) {
    tc_error(expr, "Type Error", "Failed to type-check free expression");
    return NULL;
  }

  if (ptr_type->type != AST_TYPE_POINTER) {
    tc_error(expr, "Type Error", "Cannot free non-pointer type");
    return NULL;
  }

  StaticMemoryAnalyzer *analyzer = get_static_analyzer(scope);

  // Get the variable name early
  const char *var_name = NULL;
  bool is_indexed_free = false;

  if (expr->expr.free.ptr->type == AST_EXPR_IDENTIFIER) {
    var_name = expr->expr.free.ptr->expr.identifier.name;
    is_indexed_free = false;
  } else if (expr->expr.free.ptr->type == AST_EXPR_INDEX) {
    // For indexed expressions like moves[j], extract the base name
    var_name = extract_variable_name(expr->expr.free.ptr);
    is_indexed_free = true;
  } else {
    // Fallback: try extract_variable_name for addr/member/cast expressions
    var_name = extract_variable_name(expr->expr.free.ptr);
  }

  if (analyzer && analyzer->skip_memory_tracking && var_name) {
    // We're in a defer block - find the containing FUNCTION scope
    Scope *func_scope = scope;
    while (func_scope && !func_scope->is_function_scope) {
      func_scope = func_scope->parent;
    }

    if (func_scope) {
      // IMPORTANT: Only track non-indexed frees in defer blocks
      // Indexed frees like free(moves[j]) in loops are for freeing array
      // elements, not the array itself. We only care about tracking free(moves)
      // for leak detection.
      if (!is_indexed_free) {
        // Check if already in deferred list (avoid duplicates)
        bool already_deferred = false;
        for (size_t i = 0; i < func_scope->deferred_frees.count; i++) {
          const char **existing =
              (const char **)((char *)func_scope->deferred_frees.data +
                              i * sizeof(const char *));
          if (*existing && strcmp(*existing, var_name) == 0) {
            already_deferred = true;
            break;
          }
        }

        if (!already_deferred) {
          const char **slot =
              (const char **)growable_array_push(&func_scope->deferred_frees);
          if (slot) {
            *slot = var_name;
          }
        }
      }
    }
  } else if (analyzer && var_name) {
    // Normal free - always track it (both indexed and non-indexed)
    const char *func_name = get_current_function_name(scope);

    // Only warn about non-allocated free when it's &variable (reliably wrong)
    if (expr->expr.free.ptr->type == AST_EXPR_ADDR) {
      static_memory_check_free_nonalloc(analyzer, var_name, expr->line,
                                         expr->column, g_tokens,
                                         g_token_count, g_file_path,
                                         func_name, arena);
    }
    static_memory_track_free(analyzer, var_name, func_name,
                             !scope->is_function_scope);
  }

  return create_basic_type(arena, "void", expr->line, expr->column);
}

AstNode *typecheck_memcpy_expr(AstNode *expr, Scope *scope,
                               ArenaAllocator *arena) {
  (void)expr;
  (void)scope;
  (void)arena;
  return NULL;
}

AstNode *typecheck_cast_expr(AstNode *expr, Scope *scope,
                             ArenaAllocator *arena) {
  AstNode *castee_type =
      typecheck_expression(expr->expr.cast.castee, scope, arena);
  if (!castee_type) {
    tc_error(expr, "Type Error", "Cannot determine type of cast operand");
    return NULL;
  }

  AstNode *target_type = expr->expr.cast.type;
  if (!target_type) {
    tc_error(expr, "Type Error", "Cast target type is unknown");
    return NULL;
  }

  if (!is_cast_valid(castee_type, target_type)) {
    const char *from_name = type_to_string(castee_type, arena);
    const char *to_name = type_to_string(target_type, arena);
    char message[256];
    snprintf(message, sizeof(message),
             "Cannot cast '%s' to '%s'", from_name, to_name);
    tc_error(expr, "Invalid Cast", "%s", message);
    return NULL;
  }

  return target_type;
}

// Add this function to expr.c

AstNode *typecheck_input_expr(AstNode *expr, Scope *scope,
                              ArenaAllocator *arena) {
  if (expr->type != AST_EXPR_INPUT) {
    tc_error(expr, "Internal Error", "Expected input expression node");
    return NULL;
  }

  AstNode *input_type = expr->expr.input.type;
  AstNode *msg = expr->expr.input.msg;

  // Validate that a type is provided
  if (!input_type) {
    tc_error(expr, "Input Error", "Input expression must specify a type");
    return NULL;
  }

  // Validate that the type is actually a type node
  if (input_type->category != Node_Category_TYPE) {
    tc_error(expr, "Input Error", "Input type parameter must be a valid type");
    return NULL;
  }

  // Validate the message expression if provided
  if (msg) {
    AstNode *msg_type = typecheck_expression(msg, scope, arena);
    if (!msg_type) {
      tc_error(expr, "Input Error",
               "Failed to determine type of input message");
      return NULL;
    }

    // Check that message is a string type
    AstNode *expected_string = create_basic_type(arena, "str", 0, 0);
    TypeMatchResult match = types_match(expected_string, msg_type);

    if (match == TYPE_MATCH_NONE) {
      tc_error_help(
          expr, "Input Message Type Error", "Input message must be a string",
          "Expected 'string', got '%s'", type_to_string(msg_type, arena));
      return NULL;
    }
  }

  // Validate that the input type is a supported type for input operations
  // Typically, input should work with basic types (int, float, string, etc.)
  if (input_type->type == AST_TYPE_BASIC) {
    const char *type_name = input_type->type_data.basic.name;

    // Check if it's a supported input type
    bool is_supported =
        strcmp(type_name, "int") == 0 || strcmp(type_name, "float") == 0 ||
        strcmp(type_name, "double") == 0 || strcmp(type_name, "string") == 0 ||
        strcmp(type_name, "char") == 0 || strcmp(type_name, "bool") == 0;

    if (!is_supported) {
      tc_error_help(expr, "Unsupported Input Type",
                    "Input only supports basic types (int, float, double, "
                    "string, char, bool)",
                    "Cannot read input as type '%s'", type_name);
      return NULL;
    }
  } else {
    // Complex types (pointers, arrays, structs) are not supported for input
    tc_error_help(expr, "Unsupported Input Type",
                  "Input only supports basic types",
                  "Cannot read input as complex type '%s'",
                  type_to_string(input_type, arena));
    return NULL;
  }

  // Return the specified input type
  return input_type;
}

AstNode *typecheck_system_expr(AstNode *expr, Scope *scope,
                               ArenaAllocator *arena) {
  if (expr->type != AST_EXPR_SYSTEM) {
    tc_error(expr, "Internal Error", "Expected system expression node");
    return NULL;
  }

  AstNode *command = expr->expr._system.command;

  // Validate that a command expression is provided
  if (!command) {
    tc_error(expr, "System Error", "System expression must have a command");
    return NULL;
  }

  // Typecheck the command expression
  AstNode *command_type = typecheck_expression(command, scope, arena);
  if (!command_type) {
    tc_error(expr, "System Error",
             "Failed to determine type of system command");
    return NULL;
  }

  // Verify the command is a string type
  AstNode *expected_string = create_basic_type(arena, "str", 0, 0);
  TypeMatchResult match = types_match(expected_string, command_type);

  if (match == TYPE_MATCH_NONE) {
    tc_error_help(
        expr, "System Command Type Error", "System command must be a string",
        "Expected 'string', got '%s'", type_to_string(command_type, arena));
    return NULL;
  }

  // system() returns an int (the exit status of the command)
  return create_basic_type(arena, "int", expr->line, expr->column);
}

/**
 * @brief Type check a syscall expression
 *
 * Syscall requires:
 * - First argument: syscall number (int)
 * - Remaining arguments: syscall parameters (typically int or pointer types)
 *
 * Returns: int (the return value from the syscall)
 */
AstNode *typecheck_syscall_expr(AstNode *expr, Scope *scope,
                                ArenaAllocator *arena) {
  if (expr->type != AST_EXPR_SYSCALL) {
    tc_error(expr, "Internal Error", "Expected syscall expression node");
    return NULL;
  }

  AstNode **args = expr->expr.syscall.args;
  size_t arg_count = expr->expr.syscall.count;

  // Syscall requires at least one argument (the syscall number)
  if (arg_count == 0) {
    tc_error(expr, "Syscall Error",
             "syscall() requires at least one argument (syscall number)");
    return NULL;
  }

  // Type check the syscall number (first argument)
  AstNode *syscall_num_type = typecheck_expression(args[0], scope, arena);
  if (!syscall_num_type) {
    tc_error(expr, "Syscall Error",
             "Failed to determine type of syscall number");
    return NULL;
  }

  // Verify syscall number is numeric (typically int)
  if (!is_numeric_type(syscall_num_type)) {
    tc_error_help(expr, "Syscall Number Type Error",
                  "The syscall number must be a numeric type (typically int)",
                  "Syscall number has type '%s', expected numeric type",
                  type_to_string(syscall_num_type, arena));
    return NULL;
  }

  // Type check all remaining arguments
  for (size_t i = 1; i < arg_count; i++) {
    if (!args[i]) {
      tc_error(expr, "Syscall Error", "Argument %zu is NULL", i + 1);
      return NULL;
    }

    AstNode *arg_type = typecheck_expression(args[i], scope, arena);
    if (!arg_type) {
      tc_error(expr, "Syscall Error", "Failed to type-check argument %zu",
               i + 1);
      return NULL;
    }

    // Syscall arguments should typically be numeric or pointer types
    // We allow any type but warn about non-standard types
    bool is_valid_syscall_arg =
        is_numeric_type(arg_type) || is_pointer_type(arg_type) ||
        (arg_type->type == AST_TYPE_BASIC &&
         strcmp(arg_type->type_data.basic.name, "void") == 0);

    if (!is_valid_syscall_arg) {
      tc_error_help(
          expr, "Syscall Argument Type Warning",
          "Syscall arguments are typically numeric or pointer types",
          "Argument %zu has type '%s' which may not be valid for syscalls",
          i + 1, type_to_string(arg_type, arena));
      // Don't return false - this is a warning, not an error
    }
  }

  // syscall() returns a long or int (platform dependent)
  // For simplicity, we return int here
  return create_basic_type(arena, "int", expr->line, expr->column);
}

AstNode *typecheck_sizeof_expr(AstNode *expr, Scope *scope,
                               ArenaAllocator *arena) {
  // sizeof always returns size_t (or int in simplified systems)
  AstNode *object_type = NULL;

  // Check if it is a type or an expression
  if (expr->expr.size_of.is_type) {
    object_type = expr->expr.size_of.object;
  } else {
    object_type = typecheck_expression(expr->expr.size_of.object, scope, arena);
  }

  if (!object_type) {
    tc_error(expr, "Type Error", "Cannot determine type for sizeof operand");
    return NULL;
  }

  return create_basic_type(arena, "int", expr->line, expr->column);
}

AstNode *typecheck_array_expr(AstNode *expr, Scope *scope,
                              ArenaAllocator *arena) {
  if (expr->type != AST_EXPR_ARRAY) {
    tc_error(expr, "Internal Error", "Expected array expression node");
    return NULL;
  }

  AstNode **elements = expr->expr.array.elements;
  size_t element_count = expr->expr.array.element_count;

  // Empty array - cannot infer type
  if (element_count == 0) {
    tc_error(expr, "Array Type Error",
             "Cannot infer type of empty array literal - provide explicit type "
             "annotation");
    return NULL;
  }

  // Type check the first element to establish the array's element type
  AstNode *first_element_type = typecheck_expression(elements[0], scope, arena);
  if (!first_element_type) {
    tc_error(expr, "Array Type Error",
             "Failed to determine type of first array element");
    return NULL;
  }

  // Check all remaining elements match the first element's type
  for (size_t i = 1; i < element_count; i++) {
    AstNode *element_type = NULL;

    // CRITICAL FIX: If this element is an anonymous struct expression,
    // pass the first element's type as expected_type to ensure they match
    if (elements[i]->type == AST_EXPR_STRUCT &&
        !elements[i]->expr.struct_expr.name) {
      // Anonymous struct - use internal function with expected type
      element_type = typecheck_struct_expr_internal(elements[i], scope, arena,
                                                    first_element_type);
    } else {
      // Regular expression - typecheck normally
      element_type = typecheck_expression(elements[i], scope, arena);
    }

    if (!element_type) {
      tc_error(expr, "Array Type Error",
               "Failed to determine type of array element %zu", i);
      return NULL;
    }

    TypeMatchResult match = types_match(first_element_type, element_type);
    if (match == TYPE_MATCH_NONE) {
      tc_error_help(
          expr, "Array Element Type Mismatch",
          "All array elements must have the same type",
          "Element %zu has type '%s', but first element has type '%s'", i,
          type_to_string(element_type, arena),
          type_to_string(first_element_type, arena));
      return NULL;
    }
  }

  // NEW: Check if this array is being used in a context with an expected size
  // This happens during function call argument checking
  // We'll set a flag on the array expression to indicate it should be padded

  // For now, just create the array type with the actual element count
  // The padding will be handled during code generation or in a separate pass
  long long size_val = (long long)element_count;
  AstNode *size_expr = create_literal_expr(arena, LITERAL_INT, &size_val,
                                           expr->line, expr->column);

  // Create and return array type
  AstNode *array_type = create_array_type(arena, first_element_type, size_expr,
                                          expr->line, expr->column);
  return array_type;
}

AstNode *typecheck_struct_expr_internal(AstNode *expr, Scope *scope,
                                        ArenaAllocator *arena,
                                        AstNode *expected_type) {
  if (expr->type != AST_EXPR_STRUCT) {
    tc_error(expr, "Internal Error", "Expected struct expression node");
    return NULL;
  }

  const char *struct_name = expr->expr.struct_expr.name;
  char **field_names = expr->expr.struct_expr.field_names;
  AstNode **field_values = expr->expr.struct_expr.field_value;
  size_t field_count = expr->expr.struct_expr.field_count;

  if (struct_name) {
    // Named struct initialization: Point { x: 10, y: 20 }

    // Look up the struct type directly by name
    Symbol *struct_symbol = scope_lookup(scope, struct_name);
    if (!struct_symbol) {
      tc_error_id(expr, struct_name, "Undefined Type",
                  "Struct type '%s' not found", struct_name);
      return NULL;
    }

    AstNode *struct_type = struct_symbol->type;
    if (!struct_type) {
      tc_error_id(expr, struct_name, "Type Error",
                  "Type '%s' has no type information", struct_name);
      return NULL;
    }

    if (struct_type->type != AST_TYPE_STRUCT) {
      tc_error_id(expr, struct_name, "Type Error",
                  "'%s' is not a struct type (it's a %s)", struct_name,
                  struct_type->type == AST_TYPE_BASIC ? "basic type"
                                                      : "other type");
      return NULL;
    }

    // Track already-resolved field names to handle spreads and overrides
    GrowableArray resolved_fields;
    growable_array_init(&resolved_fields, arena, field_count, sizeof(char *));

    // Validate each initialized field (including spreads)
    for (size_t i = 0; i < field_count; i++) {
      const char *field_name = field_names[i];
      AstNode *field_value = field_values[i];

      // Handle spread expression: ...expr
      if (!field_name) {
        if (!field_value || field_value->type != AST_EXPR_SPREAD) {
          tc_error(expr, "Invalid Field",
                   "Invalid field entry in struct literal");
          return NULL;
        }

        // Typecheck the spread's inner expression
        AstNode *spread_source =
            typecheck_expression(field_value->expr.spread.expr, scope, arena);
        if (!spread_source) {
          tc_error(expr, "Spread Error",
                   "Failed to evaluate spread expression");
          return NULL;
        }

        // Resolve to struct type if needed
        AstNode *source_struct = spread_source;
        if (source_struct->type == AST_TYPE_BASIC) {
          Symbol *src_sym =
              scope_lookup(scope, source_struct->type_data.basic.name);
          if (src_sym && src_sym->type &&
              src_sym->type->type == AST_TYPE_STRUCT) {
            source_struct = src_sym->type;
          }
        } else if (source_struct->type == AST_TYPE_POINTER) {
          AstNode *pointee = source_struct->type_data.pointer.pointee_type;
          if (pointee && pointee->type == AST_TYPE_BASIC) {
            Symbol *src_sym = scope_lookup(scope, pointee->type_data.basic.name);
            if (src_sym && src_sym->type &&
                src_sym->type->type == AST_TYPE_STRUCT) {
              source_struct = src_sym->type;
            }
          } else if (pointee && pointee->type == AST_TYPE_STRUCT) {
            source_struct = pointee;
          }
        }

        if (!source_struct || source_struct->type != AST_TYPE_STRUCT) {
          tc_error(expr, "Spread Error",
                   "Spread expression must evaluate to a struct type");
          return NULL;
        }

        // Verify the spread type is compatible with the target struct
        TypeMatchResult compat =
            types_match(struct_type, source_struct);
        if (compat == TYPE_MATCH_NONE) {
          tc_error_help(expr, "Spread Error",
                        "The spread expression must resolve to a compatible "
                        "struct type",
                        "Spread expression resolves to '%s', but the literal "
                        "is for struct '%s'",
                        type_to_string(source_struct, arena), struct_name);
          return NULL;
        }

        // Flatten fields from the source struct
        // Fields already in resolved_fields are overrides — skip them
        for (size_t j = 0;
             j < source_struct->type_data.struct_type.member_count; j++) {
          const char *src_fname =
              source_struct->type_data.struct_type.member_names[j];
          AstNode *src_ftype =
              source_struct->type_data.struct_type.member_types[j];

          // Skip methods
          if (src_ftype && src_ftype->type == AST_TYPE_FUNCTION)
            continue;

          // Check if field exists in target struct
          AstNode *target_ftype =
              get_struct_member_type(struct_type, src_fname);
          if (!target_ftype) {
            tc_error_help(
                expr, "Spread Error",
                "Field brought in by spread does not exist in target struct",
                "Struct '%s' has no field '%s' brought in by spread from '%s'",
                struct_name, src_fname,
                source_struct->type_data.struct_type.name);
            return NULL;
          }

          if (target_ftype && target_ftype->type == AST_TYPE_FUNCTION)
            continue;

          // Check if field is already set (override from earlier explicit field)
          bool already_set = false;
          for (size_t k = 0; k < resolved_fields.count; k++) {
            char **existing =
                (char **)((char *)resolved_fields.data + k * sizeof(char *));
            if (strcmp(*existing, src_fname) == 0) {
              already_set = true;
              break;
            }
          }
          if (already_set)
            continue;

          // Mark as resolved
          char **slot = (char **)growable_array_push(&resolved_fields);
          *slot = (char *)src_fname;
        }
        continue;
      }

      // Regular field — check duplicate against resolved fields
      bool already_resolved = false;
      for (size_t j = 0; j < resolved_fields.count; j++) {
        char **existing =
            (char **)((char *)resolved_fields.data + j * sizeof(char *));
        if (strcmp(*existing, field_name) == 0) {
          already_resolved = true;
          break;
        }
      }

      if (!already_resolved) {
        char **slot = (char **)growable_array_push(&resolved_fields);
        *slot = (char *)field_name;
      }

      // Check if the field exists in the struct definition
      AstNode *expected_field_type =
          get_struct_member_type(struct_type, field_name);
      if (!expected_field_type) {
        tc_error_help(expr, "Unknown Field",
                      "Check the struct definition for valid field names",
                      "Struct '%s' has no field named '%s'", struct_name,
                      field_name);
        return NULL;
      }

      // Don't allow initializing methods
      if (expected_field_type->type == AST_TYPE_FUNCTION) {
        tc_error_help(expr, "Invalid Field Initialization",
                      "Methods cannot be initialized in struct literals",
                      "Cannot initialize method '%s' in struct '%s'",
                      field_name, struct_name);
        return NULL;
      }

      // Type check the field value
      AstNode *actual_type = typecheck_expression(field_value, scope, arena);
      if (!actual_type) {
        tc_error(expr, "Type Error",
                 "Failed to determine type of value for field '%s'",
                 field_name);
        return NULL;
      }

      // Check type compatibility
      TypeMatchResult match = types_match(expected_field_type, actual_type);
      if (match == TYPE_MATCH_NONE) {
        tc_error_help(expr, "Field Type Mismatch",
                      "The value type must match the struct field type",
                      "Field '%s' expects type '%s', got '%s'", field_name,
                      type_to_string(expected_field_type, arena),
                      type_to_string(actual_type, arena));
        return NULL;
      }
    }

    // Return a basic type that references the struct name
    return create_basic_type(arena, struct_name, expr->line, expr->column);

  } else {
    // Anonymous struct initialization: { x: 10, y: 20 }

    // If we have an expected type, try to match against it
    if (expected_type) {
      // Resolve the expected type to an actual struct type
      AstNode *target_struct_type = expected_type;
      const char *target_struct_name = NULL;

      // If expected type is a basic type, resolve it to the struct
      if (expected_type->type == AST_TYPE_BASIC) {
        target_struct_name = expected_type->type_data.basic.name;
        Symbol *struct_symbol = scope_lookup(scope, target_struct_name);

        if (struct_symbol && struct_symbol->type &&
            struct_symbol->type->type == AST_TYPE_STRUCT) {
          target_struct_type = struct_symbol->type;
        } else {
          // Expected type is not a struct, fall through to create anonymous
          target_struct_type = NULL;
        }
      } else if (expected_type->type != AST_TYPE_STRUCT) {
        target_struct_type = NULL;
      }

      // If we successfully resolved to a struct type, validate against it
      if (target_struct_type && target_struct_type->type == AST_TYPE_STRUCT) {
        // Track resolved field names for spread handling
        GrowableArray resolved_fields;
        growable_array_init(&resolved_fields, arena, field_count, sizeof(char *));

        // Validate each field in the anonymous struct
        for (size_t i = 0; i < field_count; i++) {
          const char *field_name = field_names[i];
          AstNode *field_value = field_values[i];

          // Handle spread expression: ...expr
          if (!field_name) {
            if (!field_value || field_value->type != AST_EXPR_SPREAD) {
              tc_error(expr, "Invalid Field",
                       "Invalid field entry in struct literal");
              return NULL;
            }

            AstNode *spread_source =
                typecheck_expression(field_value->expr.spread.expr, scope, arena);
            if (!spread_source) {
              tc_error(expr, "Spread Error",
                       "Failed to evaluate spread expression");
              return NULL;
            }

            AstNode *source_struct = spread_source;
            if (source_struct->type == AST_TYPE_BASIC) {
              Symbol *src_sym =
                  scope_lookup(scope, source_struct->type_data.basic.name);
              if (src_sym && src_sym->type &&
                  src_sym->type->type == AST_TYPE_STRUCT) {
                source_struct = src_sym->type;
              }
            } else if (source_struct->type == AST_TYPE_POINTER) {
              AstNode *pointee = source_struct->type_data.pointer.pointee_type;
              if (pointee && pointee->type == AST_TYPE_BASIC) {
                Symbol *src_sym = scope_lookup(scope, pointee->type_data.basic.name);
                if (src_sym && src_sym->type &&
                    src_sym->type->type == AST_TYPE_STRUCT) {
                  source_struct = src_sym->type;
                }
              } else if (pointee && pointee->type == AST_TYPE_STRUCT) {
                source_struct = pointee;
              }
            }

            if (!source_struct || source_struct->type != AST_TYPE_STRUCT) {
              tc_error(expr, "Spread Error",
                       "Spread expression must evaluate to a struct type");
              return NULL;
            }

            TypeMatchResult compat =
                types_match(target_struct_type, source_struct);
            if (compat == TYPE_MATCH_NONE) {
              tc_error_help(expr, "Spread Error",
                            "Spread expression must resolve to a compatible struct type",
                            "Spread expression resolves to '%s', but expected struct '%s'",
                            type_to_string(source_struct, arena),
                            target_struct_name ? target_struct_name : "?");
              return NULL;
            }

            for (size_t j = 0;
                 j < source_struct->type_data.struct_type.member_count; j++) {
              const char *src_fname =
                  source_struct->type_data.struct_type.member_names[j];
              AstNode *src_ftype =
                  source_struct->type_data.struct_type.member_types[j];

              if (src_ftype && src_ftype->type == AST_TYPE_FUNCTION)
                continue;

              AstNode *target_ftype =
                  get_struct_member_type(target_struct_type, src_fname);
              if (!target_ftype) {
                tc_error_help(expr, "Spread Error",
                              "Field brought in by spread does not exist in target struct",
                              "Struct '%s' has no field '%s' brought in by spread",
                              target_struct_name ? target_struct_name : "?",
                              src_fname);
                return NULL;
              }

              if (target_ftype && target_ftype->type == AST_TYPE_FUNCTION)
                continue;

              bool already_set = false;
              for (size_t k = 0; k < resolved_fields.count; k++) {
                char **existing =
                    (char **)((char *)resolved_fields.data + k * sizeof(char *));
                if (strcmp(*existing, src_fname) == 0) {
                  already_set = true;
                  break;
                }
              }
              if (already_set)
                continue;

              char **slot = (char **)growable_array_push(&resolved_fields);
              *slot = (char *)src_fname;
            }
            continue;
          }

          // Regular field
          bool already_resolved = false;
          for (size_t j = 0; j < resolved_fields.count; j++) {
            char **existing =
                (char **)((char *)resolved_fields.data + j * sizeof(char *));
            if (strcmp(*existing, field_name) == 0) {
              already_resolved = true;
              break;
            }
          }

          if (!already_resolved) {
            char **slot = (char **)growable_array_push(&resolved_fields);
            *slot = (char *)field_name;
          }

          // Check if the field exists in the expected struct
          AstNode *expected_field_type =
              get_struct_member_type(target_struct_type, field_name);
          if (!expected_field_type) {
            tc_error_help(expr, "Unknown Field",
                          "Anonymous struct field does not match expected type",
                          "Type '%s' has no field named '%s'",
                          target_struct_name ? target_struct_name : "struct",
                          field_name);
            return NULL;
          }

          // Don't allow initializing methods
          if (expected_field_type->type == AST_TYPE_FUNCTION) {
            tc_error_help(expr, "Invalid Field Initialization",
                          "Methods cannot be initialized in struct literals",
                          "Cannot initialize method '%s'", field_name);
            return NULL;
          }

          // Type check the field value
          AstNode *actual_type =
              typecheck_expression(field_value, scope, arena);
          if (!actual_type) {
            tc_error(expr, "Type Error",
                     "Failed to determine type of value for field '%s'",
                     field_name);
            return NULL;
          }

          // Check type compatibility
          TypeMatchResult match = types_match(expected_field_type, actual_type);
          if (match == TYPE_MATCH_NONE) {
            tc_error_help(expr, "Field Type Mismatch",
                          "The value type must match the struct field type",
                          "Field '%s' expects type '%s', got '%s'", field_name,
                          type_to_string(expected_field_type, arena),
                          type_to_string(actual_type, arena));
            return NULL;
          }
        }

        // Return the expected type (as basic type reference)
        if (target_struct_name) {
          return create_basic_type(arena, target_struct_name, expr->line,
                                   expr->column);
        } else {
          return expected_type;
        }
      }
    }

    // No expected type or couldn't resolve - create true anonymous struct
    // First pass: resolve all fields (handling spreads and explicit fields)
    GrowableArray anon_names;
    growable_array_init(&anon_names, arena, field_count, sizeof(const char *));
    GrowableArray anon_types;
    growable_array_init(&anon_types, arena, field_count, sizeof(AstNode *));

    for (size_t i = 0; i < field_count; i++) {
      const char *field_name = field_names[i];
      AstNode *field_value = field_values[i];

      if (!field_name) {
        // Spread expression
        if (!field_value || field_value->type != AST_EXPR_SPREAD) {
          tc_error(expr, "Invalid Field",
                   "Invalid field entry in struct literal");
          return NULL;
        }

        AstNode *spread_source =
            typecheck_expression(field_value->expr.spread.expr, scope, arena);
        if (!spread_source) {
          tc_error(expr, "Spread Error",
                   "Failed to evaluate spread expression");
          return NULL;
        }

        // Resolve to struct type
        AstNode *source_struct = spread_source;
        if (source_struct->type == AST_TYPE_BASIC) {
          Symbol *src_sym =
              scope_lookup(scope, source_struct->type_data.basic.name);
          if (src_sym && src_sym->type &&
              src_sym->type->type == AST_TYPE_STRUCT) {
            source_struct = src_sym->type;
          }
        } else if (source_struct->type == AST_TYPE_POINTER) {
          AstNode *pointee = source_struct->type_data.pointer.pointee_type;
          if (pointee && pointee->type == AST_TYPE_BASIC) {
            Symbol *src_sym =
                scope_lookup(scope, pointee->type_data.basic.name);
            if (src_sym && src_sym->type &&
                src_sym->type->type == AST_TYPE_STRUCT) {
              source_struct = src_sym->type;
            }
          } else if (pointee && pointee->type == AST_TYPE_STRUCT) {
            source_struct = pointee;
          }
        }

        if (!source_struct || source_struct->type != AST_TYPE_STRUCT) {
          tc_error(expr, "Spread Error",
                   "Spread expression must evaluate to a struct type");
          return NULL;
        }

        // Flatten non-method fields from source struct
        for (size_t j = 0;
             j < source_struct->type_data.struct_type.member_count; j++) {
          const char *src_fname =
              source_struct->type_data.struct_type.member_names[j];
          AstNode *src_ftype =
              source_struct->type_data.struct_type.member_types[j];

          if (src_ftype && src_ftype->type == AST_TYPE_FUNCTION)
            continue;

          // Check for duplicate against already-resolved fields
          bool already_set = false;
          for (size_t k = 0; k < anon_names.count; k++) {
            const char **existing =
                (const char **)((char *)anon_names.data + k * sizeof(const char *));
            if (strcmp(*existing, src_fname) == 0) {
              already_set = true;
              break;
            }
          }
          if (already_set)
            continue;

          const char **name_slot =
              (const char **)growable_array_push(&anon_names);
          *name_slot = src_fname;
          AstNode **type_slot =
              (AstNode **)growable_array_push(&anon_types);
          *type_slot = src_ftype;
        }
        continue;
      }

      // Regular field - check for duplicate
      bool already_set = false;
      for (size_t j = 0; j < anon_names.count; j++) {
        const char **existing =
            (const char **)((char *)anon_names.data + j * sizeof(const char *));
        if (strcmp(*existing, field_name) == 0) {
          already_set = true;
          break;
        }
      }

      AstNode *field_type = typecheck_expression(field_value, scope, arena);
      if (!field_type) {
        tc_error(expr, "Type Error",
                 "Failed to determine type of value for field '%s'",
                 field_name);
        return NULL;
      }

      if (already_set) {
        continue;
      }

      const char **name_slot =
          (const char **)growable_array_push(&anon_names);
      *name_slot = field_name;
      AstNode **type_slot =
          (AstNode **)growable_array_push(&anon_types);
      *type_slot = field_type;
    }

    // Create an anonymous struct type with the inferred field types
    size_t anon_name_len = 50;
    char *anon_name = arena_alloc(arena, anon_name_len, alignof(char));
    snprintf(anon_name, anon_name_len, "__anon_struct_%zu_%zu", expr->line,
             expr->column);

    AstNode *anon_struct_type = create_struct_type(
        arena, anon_name, (AstNode **)anon_types.data,
        (const char **)anon_names.data, anon_names.count,
        expr->line, expr->column);

    if (!anon_struct_type) {
      tc_error(expr, "Type Creation Error",
               "Failed to create anonymous struct type");
      return NULL;
    }

    return anon_struct_type;
  }
}

// Public wrapper that doesn't expose expected_type
AstNode *typecheck_struct_expr(AstNode *expr, Scope *scope,
                               ArenaAllocator *arena) {
  return typecheck_struct_expr_internal(expr, scope, arena, NULL);
}
