/*
 * /obelix/src/ast/ast.c - Copyright (c) 2020 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libast.h"

static ast_node_t *       _ast_node_new(ast_node_t *, va_list);
static void               _ast_node_free(ast_node_t *);

static ast_const_t *      _ast_const_new(ast_const_t *, va_list);
static void               _ast_const_free(ast_const_t *);
static data_t *           _ast_const_call(ast_const_t *, arguments_t *);
static char *             _ast_const_tostring(ast_const_t *);

static ast_infix_t *      _ast_infix_new(ast_infix_t *, va_list);
static void               _ast_infix_free(ast_infix_t *);
static data_t *           _ast_infix_call(ast_infix_t *, arguments_t *);
static char *             _ast_infix_tostring(ast_infix_t *);

static ast_prefix_t *     _ast_prefix_new(ast_prefix_t *, va_list);
static void               _ast_prefix_free(ast_prefix_t *);
static data_t *           _ast_prefix_call(ast_prefix_t *, arguments_t *);
static char *             _ast_prefix_tostring(ast_prefix_t *);

static ast_ternary_t *    _ast_ternary_new(ast_ternary_t *, va_list);
static void               _ast_ternary_free(ast_ternary_t *);
static data_t *           _ast_ternary_call(ast_ternary_t *, arguments_t *);
static char *             _ast_ternary_tostring(ast_ternary_t *);

static ast_variable_t *   _ast_variable_new(ast_variable_t *, va_list);
static void               _ast_variable_free(ast_variable_t *);
static data_t *           _ast_variable_call(ast_variable_t *, arguments_t *);
static char *             _ast_variable_tostring(ast_variable_t *);

static ast_generator_t *  _ast_generator_new(ast_generator_t *, va_list);
static void               _ast_generator_free(ast_generator_t *);
static data_t *           _ast_generator_call(ast_generator_t *, arguments_t *);
static char *             _ast_generator_tostring(ast_generator_t *);

static ast_call_t *       _ast_call_new(ast_call_t *, va_list);
static void               _ast_call_free(ast_call_t *);
static data_t *           _ast_call_call(ast_call_t *, arguments_t *);
static char *             _ast_call_tostring(ast_call_t *);

static ast_block_t *      _ast_block_new(ast_block_t *, va_list);
static void               _ast_block_free(ast_block_t *);
static data_t *           _ast_block_call(ast_block_t *, arguments_t *);
static char *             _ast_block_tostring(ast_block_t *);

static ast_assignment_t * _ast_assignment_new(ast_assignment_t *, va_list);
static void               _ast_assignment_free(ast_assignment_t *);
static data_t *           _ast_assignment_call(ast_assignment_t *, arguments_t *);
static char *             _ast_assignment_tostring(ast_assignment_t *);

static ast_loop_t *       _ast_loop_new(ast_loop_t *, va_list);
static void               _ast_loop_free(ast_loop_t *);
static data_t *           _ast_loop_call(ast_loop_t *, arguments_t *);
static char *             _ast_loop_tostring(ast_loop_t *);

static ast_script_t *   _ast_script_new(ast_script_t *, va_list);
static void             _ast_script_free(ast_script_t *);
static char *           _ast_script_tostring(ast_script_t *);

static ast_builder_t *  _ast_builder_new(ast_builder_t *, va_list);
static void             _ast_builder_free(ast_builder_t *);
static char *           _ast_builder_tostring(ast_builder_t *);

/* -- data_t type description structures ---------------------------------- */

static vtable_t _vtable_ASTNode[] = {
  { .id = FunctionNew, .fnc = (void_t) _ast_node_new },
  { .id = FunctionFree, .fnc = (void_t) _ast_node_free },
  { .id = FunctionNone, .fnc = NULL }
};

static vtable_t _vtable_ASTExpr[] = {
  { .id = FunctionNone, .fnc = NULL }
};

static vtable_t _vtable_ASTConst[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_const_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_const_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_const_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_const_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTInfix[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_infix_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_infix_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_infix_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_infix_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTPrefix[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_prefix_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_prefix_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_prefix_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_prefix_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTTernary[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_ternary_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_ternary_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_ternary_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_ternary_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTVariable[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_variable_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_variable_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_variable_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_variable_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTGenerator[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_generator_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_generator_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_generator_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_generator_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTCall[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_call_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_call_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_call_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_call_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTBlock[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_block_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_block_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_block_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_block_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTAssignment[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_assignment_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_assignment_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_assignment_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_assignment_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTLoop[] = {
  { .id = FunctionNew,         .fnc = (void_t) _ast_loop_new },
  { .id = FunctionFree,        .fnc = (void_t) _ast_loop_free },
  { .id = FunctionCall,        .fnc = (void_t) _ast_loop_call },
  { .id = FunctionAllocString, .fnc = (void_t) _ast_loop_tostring },
  { .id = FunctionNone,        .fnc = NULL }
};

static vtable_t _vtable_ASTScript[] = {
  { .id = FunctionNew,      .fnc = (void_t) _ast_script_new },
  { .id = FunctionFree,     .fnc = (void_t) _ast_script_free },
  { .id = FunctionToString, .fnc = (void_t) _ast_script_tostring },
  { .id = FunctionCall,     .fnc = NULL },
  { .id = FunctionNone,     .fnc = NULL }
};

static vtable_t _vtable_ASTStatement[] = {
  { .id = FunctionNone, .fnc = NULL }
};

static vtable_t _vtable_ASTIf[] = {
  { .id = FunctionNone, .fnc = NULL }
};

static vtable_t _vtable_ASTPass[] = {
  { .id = FunctionNone, .fnc = NULL }
};

static vtable_t _vtable_ASTWhile[] = {
  { .id = FunctionNone, .fnc = NULL }
};

static vtable_t _vtable_ASTBuilder[] = {
  { .id = FunctionNew, .fnc = (void_t) _ast_builder_new },
  { .id = FunctionFree, .fnc = (void_t) _ast_builder_free },
  { .id = FunctionToString, .fnc = (void_t) _ast_builder_tostring },
  { .id = FunctionNone, .fnc = NULL }
};

int ASTNode = -1;

int ASTExpr = -1;
int ASTConst = -1;
int ASTInfix = -1;
int ASTPrefix = -1;
int ASTTernary = -1;
int ASTVariable = -1;
int ASTGenerator = -1;
int ASTCall = -1;

int ASTAssignment = -1;
int ASTBlock = -1;
int ASTLoop = -1;

int ASTScript = -1;
int ASTStatement = -1;
int ASTBuilder = -1;

int ASTIf = -1;
int ASTPass = -1;
int ASTWhile = -1;

int ast_debug = 0;

/* ------------------------------------------------------------------------ */

void ast_init(void) {
  if (ASTNode < 0) {
    logging_register_module(ast);
    typedescr_register(ASTNode, ast_node_t);

    typedescr_register(ASTExpr, ast_expr_t);
    typedescr_assign_inheritance(ASTExpr, ASTNode);
    typedescr_register(ASTConst, ast_const_t);
    typedescr_assign_inheritance(ASTConst, ASTExpr);
    typedescr_register(ASTInfix, ast_infix_t);
    typedescr_assign_inheritance(ASTInfix, ASTExpr);
    typedescr_register(ASTPrefix, ast_prefix_t);
    typedescr_assign_inheritance(ASTPrefix, ASTExpr);
    typedescr_register(ASTTernary, ast_ternary_t);
    typedescr_assign_inheritance(ASTTernary, ASTExpr);
    typedescr_register(ASTVariable, ast_variable_t);
    typedescr_assign_inheritance(ASTVariable, ASTExpr);
    typedescr_register(ASTGenerator, ast_generator_t);
    typedescr_assign_inheritance(ASTGenerator, ASTExpr);
    typedescr_register(ASTCall, ast_call_t);
    typedescr_assign_inheritance(ASTCall, ASTExpr);

    typedescr_register(ASTBlock, ast_block_t);
    typedescr_assign_inheritance(ASTBlock, ASTExpr);
    typedescr_register(ASTAssignment, ast_assignment_t);
    typedescr_assign_inheritance(ASTAssignment, ASTExpr);
    typedescr_register(ASTLoop, ast_loop_t);
    typedescr_assign_inheritance(ASTLoop, ASTExpr);
  }
}

/* -- A S T  S T A T I C  F U N C T I O N S  ----------------------------- */

ast_node_t *_ast_node_new(ast_node_t *node, va_list args) {
  node->children = datalist_create(NULL);
  return node;
}

void _ast_node_free(ast_node_t *node) {
  if (node) {
    ast_node_free(node->parent);
  }
}

/* ----------------------------------------------------------------------- */

ast_const_t *_ast_const_new(ast_const_t *node, va_list args) {
  data_t *value = va_arg(args, data_t *);

  if (!value) {
    value = data_null();
  }
  node -> value = data_copy(value);
  return node;
}

void _ast_const_free(ast_const_t *node) {
  if (node) {
    data_free(node -> value);
  }
}

data_t * _ast_const_call(ast_const_t *node, arguments_t *args) {
  debug(ast, "%s", data_tostring(node));
  return data_copy(node);
}

char * _ast_const_tostring(ast_const_t *node) {
  char *ret;
  asprintf(&ret, "'%s':%s", data_tostring(node -> value), data_typename(node -> value));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_infix_t *_ast_infix_new(ast_infix_t *node, va_list args) {
  ast_expr_t *left = va_arg(args, ast_expr_t *);
  data_t     *op = va_arg(args, data_t *);
  ast_expr_t *right = va_arg(args, ast_expr_t *);

  node -> left = ast_expr_copy(left);
  if (data_is_token(op)) {
    node -> op = str_to_data(token_token((token_t *) op));
  } else if (data_is_string(op)){
    node -> op = data_copy(op);
  } else {
    node -> op = str_to_data(data_tostring(op));
  }
  node -> right = ast_expr_copy(right);
  return node;
}

void _ast_infix_free(ast_infix_t *node) {
  if (node) {
    ast_expr_free(node -> left);
    data_free(node -> op);
    ast_expr_free(node -> right);
  }
}

data_t * _ast_infix_call(ast_infix_t *node, arguments_t *args) {
  data_t      *left_val;
  data_t      *right_val;
  arguments_t *op_args;
  data_t      *ret_val;
  data_t      *ret;

  left_val = data_call((data_t *) node -> left, args);
  right_val = data_call((data_t *) node -> right, args);

  if (!data_is_ast_const(left_val) || !data_is_ast_const(right_val)) {
    ret = (data_t *) ast_infix_create(
      data_as_ast_expr(left_val),
      data_copy(node -> op),
      data_as_ast_expr(right_val));
  } else {
    op_args = arguments_create_args(1, data_as_ast_const(right_val)->value);
    ret_val = data_execute(data_as_ast_const(left_val)->value,
                           data_tostring(node->op),
                           op_args);
    arguments_free(op_args);
    ret = (data_t *) ast_const_create(ret_val);
    data_free(ret_val);
  }
  debug(ast, "%s %s %s = %s",
        data_tostring(node ->left),
        data_tostring(node -> op),
        data_tostring(node -> right),
        data_tostring(ret));
  return ret;
}

char * _ast_infix_tostring(ast_infix_t *node) {
  char *ret;
  asprintf(&ret, "(%s %s %s)",
           data_tostring(node -> left),
           data_tostring(node -> op),
           data_tostring(node -> right));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_prefix_t *_ast_prefix_new(ast_prefix_t *node, va_list args) {
  data_t     *op = va_arg(args, data_t *);
  ast_expr_t *operand = va_arg(args, ast_expr_t *);

  if (data_is_token(op)) {
    node -> op = str_to_data(token_token((token_t *) op));
  } else if (data_is_string(op)){
    node -> op = data_copy(op);
  } else {
    node -> op = str_to_data(data_tostring(op));
  }
  node -> operand = ast_expr_copy(operand);
  return node;
}

void _ast_prefix_free(ast_prefix_t *node) {
  if (node) {
    data_free(node -> op);
    ast_expr_free(node -> operand);
  }
}

data_t * _ast_prefix_call(ast_prefix_t *node, arguments_t *args) {
  data_t      *operand_val;
  arguments_t *op_args;
  data_t      *ret_val;
  data_t      *ret;

  operand_val = data_call((data_t *) node->operand, args);
  if (!strcmp(data_tostring(node->op), "+")) {
    ret = operand_val;
  } else {
    if (!data_is_ast_const(operand_val)) {
      ret = (data_t *) ast_prefix_create(
        data_copy(node->op),
        data_as_ast_expr(operand_val));
    } else {
      op_args = arguments_create_args(0);
      ret_val = data_execute(data_as_ast_const(operand_val)->value,
                             data_tostring(node->op),
                             op_args);
      arguments_free(op_args);
      ret = (data_t *) ast_const_create(ret_val);
      data_free(ret_val);
    }
  }
  debug(ast, "%s %s = %s",
        data_tostring(node -> op),
        data_tostring(node -> operand),
        data_tostring(ret));
  return ret;
}

char * _ast_prefix_tostring(ast_prefix_t *node) {
  char *ret;
  asprintf(&ret, "%s (%s)",
           data_tostring(node -> op),
           data_tostring(node -> operand));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_ternary_t *_ast_ternary_new(ast_ternary_t *node, va_list args) {
  ast_expr_t *condition = va_arg(args, ast_expr_t *);
  ast_expr_t *true_value = va_arg(args, ast_expr_t *);
  ast_expr_t *false_value = va_arg(args, ast_expr_t *);

  node -> condition = ast_expr_copy(condition);
  node -> true_value = ast_expr_copy(true_value);
  node -> false_value = ast_expr_copy(false_value);
  return node;
}

void _ast_ternary_free(ast_ternary_t *node) {
  if (node) {
    ast_expr_free(node -> condition);
    ast_expr_free(node -> true_value);
    ast_expr_free(node -> false_value);
  }
}

data_t * _ast_ternary_call(ast_ternary_t *node, arguments_t *args) {
  data_t      *condition_val;
  data_t      *casted = NULL;
  data_t      *ret;

  condition_val = data_call((data_t *) node -> condition, args);

  if (!data_is_ast_const(condition_val)) {
    ret = (data_t *) ast_ternary_create(
      data_as_ast_expr(condition_val),
      data_as_ast_expr(node -> true_value),
      data_as_ast_expr(node -> false_value));
  } else {
    casted = data_cast(data_as_ast_const(condition_val) -> value, Bool);
    if (!casted) {
      ret = data_exception(ErrorType, "Cannot convert %s '%s' to boolean",
                           data_typename(condition_val),
                           data_tostring(condition_val));
    } else {
      ret = data_call((data_intval(casted)
          ? (data_t *) node -> true_value
          : (data_t *) node -> false_value),
        args);
    }
  }
  debug(ast, "%s = %s", data_tostring(node), data_tostring(ret));
  data_free(condition_val);
  return ret;
}

char * _ast_ternary_tostring(ast_ternary_t *node) {
  char *ret;
  asprintf(&ret, "(%s) ? (%s) : (%s)",
           data_tostring(node -> condition),
           data_tostring(node -> true_value),
           data_tostring(node -> false_value));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_variable_t *_ast_variable_new(ast_variable_t *node, va_list args) {
  name_t     *name = va_arg(args, name_t *);

  node -> name = name_copy(name);
  return node;
}

void _ast_variable_free(ast_variable_t *node) {
  if (node) {
    name_free(node -> name);
  }
}

data_t * _ast_variable_call(ast_variable_t *node, arguments_t *args) {
  data_t *ctx = arguments_get_arg(args, 0);
  data_t *val;
  data_t *ret;

  val = data_get(ctx, node -> name);

  if (!data_is_exception(val)) {
    ret = (data_t *) ast_const_create(val);
    data_free(val);
  } else {
    ret = val;
  }
  debug(ast, "%s = %s", name_tostring(node -> name), data_tostring(ret));
  return ret;
}

char * _ast_variable_tostring(ast_variable_t *node) {
  char *ret;
  asprintf(&ret, "[%s]",
           name_tostring(node -> name));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_generator_t *_ast_generator_new(ast_generator_t *node, va_list args) {
  data_t *generator = va_arg(args, data_t *);

  node -> generator = data_copy(generator);
  node -> iter = NULL;
  return node;
}

void _ast_generator_free(ast_generator_t *node) {
  if (node) {
    data_free(node -> generator);
  }
}

data_t * _ast_generator_call(ast_generator_t *node, arguments_t *args) {
  data_t *ctx = arguments_get_arg(args, 0);
  data_t *val;
  data_t *ret;

  if (!node -> iter) {
    ret = data_iter(node -> generator);
    if (data_is_exception(ret)) {
      return ret;
    }
    node -> iter = ret;
  }

  val = data_next(node -> iter);

  if (!data_is_exception(val)) {
    ret = (data_t *) ast_const_create(val);
    data_free(val);
  } else {
    ret = val;
  }
  debug(ast, "%s = %s", ast_generator_tostring(node), data_tostring(ret));
  return ret;
}

char * _ast_generator_tostring(ast_generator_t *node) {
  char *ret;
  asprintf(&ret, " .. %s .. ", data_tostring(node -> generator));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_call_t *_ast_call_new(ast_call_t *node, va_list args) {
  ast_expr_t *function = va_arg(args, ast_expr_t *);

  node -> function = ast_expr_copy(function);
  node -> args = NULL;
  return node;
}

void _ast_call_free(ast_call_t *node) {
  if (node) {
    ast_expr_free(node -> function);
    arguments_free(node -> args);
  }
}

typedef struct _arg_reduce_ctx {
  arguments_t *arg_vals;
  arguments_t *arg_atoms;
  arguments_t *args;
  int          all_resolved;
} arg_reduce_ctx_t;

arg_reduce_ctx_t * _ast_call_execute_arg(ast_expr_t *arg, arg_reduce_ctx_t *ctx) {
  data_t    *arg_val;

  arg_val = data_call((data_t *) arg, ctx -> args);
  ctx -> all_resolved &= data_is_ast_const(arg_val);
  arguments_push(ctx -> arg_vals, arg_val);
  arguments_push(ctx -> arg_atoms,
                 data_is_ast_const(arg_val) ? data_as_ast_const(arg_val) -> value : data_null());
  return ctx;
}

data_t *_ast_call_call(ast_call_t *node, arguments_t *args) {
  data_t           *fnc_val;
  data_t           *fnc;
  data_t           *ret = NULL;
  data_t           *ret_val = NULL;
  arg_reduce_ctx_t  reduce_ctx = {
    .arg_vals = NULL,
    .args = NULL,
    .all_resolved = FALSE
  };

  fnc_val = data_call((data_t *) node->function, args);
  if (!data_is_ast_const(fnc_val)) {
    ret = (data_t *) ast_call_create(ast_expr_copy(node->function));
  }

  if (!ret) {
    reduce_ctx.arg_vals = arguments_create(NULL, NULL);
    reduce_ctx.arg_atoms = arguments_create(NULL, NULL);
    reduce_ctx.all_resolved = TRUE;
    arguments_reduce_args(node->args, (reduce_t) _ast_call_execute_arg, &reduce_ctx);
    if (!reduce_ctx.all_resolved) {
      ret = (data_t *) ast_call_create(data_as_ast_expr(fnc));
    }
  }

  if (!ret) {
    fnc = data_as_ast_const(fnc_val) -> value;
    if (data_is_callable(fnc)) {
      ret_val = data_call(fnc, reduce_ctx.arg_atoms);
      if (!data_is_exception(ret_val)) {
        ret = (data_t *) ast_const_create(ret_val);
        data_free(ret_val);
      } else {
        ret = ret_val;
      }
    } else {
      ret = data_exception(ErrorNotCallable,
                           "Atom %s of type %s is not callable",
                           data_tostring(fnc),
                           data_typename(fnc));
    }
  }

  if (data_is_ast_call(ret)) {
    data_as_ast_call(ret)->args = arguments_copy(reduce_ctx.arg_vals);
  }
  debug(ast, "%s = %s",
        ast_call_tostring(node),
        data_tostring(ret));
  return ret;
}

char * _ast_call_tostring(ast_call_t *node) {
  char *ret;
  asprintf(&ret, "%s(%s)",
           ast_expr_tostring(node -> function),
           arguments_tostring(node->args));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_block_t *_ast_block_new(ast_block_t *node, va_list args) {
  node -> statements = datalist_create(NULL);
  return node;
}

void _ast_block_free(ast_block_t *node) {
  if (node) {
    datalist_free(node -> statements);
  }
}

typedef struct _ast_block_execute_ctx {
  arguments_t *args;
  data_t      *ret;
} ast_block_execute_ctx_t;

ast_block_execute_ctx_t * _ast_block_execute_statement(ast_expr_t *stmt, ast_block_execute_ctx_t *ctx) {
  if (!data_is_exception(ctx -> ret)) {
    data_free(ctx -> ret);
    ctx -> ret = data_call((data_t *) stmt, ctx -> args);
  }
  return ctx;
}

data_t * _ast_block_call(ast_block_t *node, arguments_t *args) {
  array_t                 *statements;
  ast_block_execute_ctx_t  ctx;

  debug(ast, "%s", ast_block_tostring(node));
  statements = datalist_to_array(node -> statements);
  ctx.args = args;
  ctx.ret = data_null();
  array_reduce(statements, (reduce_t) _ast_block_execute_statement, &ctx);
  array_free(statements);
  debug(ast, "%s -> %s",
        ast_block_tostring(node),
        data_tostring(ctx.ret));
  return ctx.ret;
}

char * _ast_block_tostring(ast_block_t *node) {
  char *ret;
  asprintf(&ret, "{ %d node(s) }", datalist_size(node -> statements));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_assignment_t *_ast_assignment_new(ast_assignment_t *node, va_list args) {
  name_t     *name = va_arg(args, name_t *);
  ast_expr_t *value = va_arg(args, ast_expr_t *);

  node -> name = name_copy(name);
  node -> value = ast_expr_copy(value);
  return node;
}

void _ast_assignment_free(ast_assignment_t *node) {
  if (node) {
    name_free(node -> name);
    ast_expr_free(node -> value);
  }
}

data_t * _ast_assignment_call(ast_assignment_t *node, arguments_t *args) {
  data_t      *ctx = arguments_get_arg(args, 0);
  data_t      *val;
  data_t      *ret;
  exception_t *ex;

  val = ast_execute(node -> value, ctx);

  if (data_is_exception(val)) {
    ex = data_as_exception(val);
    if (ex->code == ErrorExhausted) {
      ret = data_false();
    } else {
      ret = data_copy(val);
    }
  } else if (data_is_ast_expr(val)) {
    ret = (data_t *) ast_assignment_create(name_copy(node -> name), data_as_ast_expr(val));
  } else {
    ret = data_set(ctx, node -> name, val);
    if (!data_is_exception(ret)) {
      data_free(ret);
      ret = data_true();
    }
  }
  debug(ast, "%s := %s -> %s",
        name_tostring(node -> name),
        data_tostring(val),
        data_tostring(ret));
  data_free(val);
  return ret;
}

char * _ast_assignment_tostring(ast_assignment_t *node) {
  char *ret;
  asprintf(&ret, "[%s] := %s",
           name_tostring(node -> name),
           ast_expr_tostring(node -> value));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_loop_t *_ast_loop_new(ast_loop_t *node, va_list args) {
  ast_expr_t *condition = va_arg(args, ast_expr_t *);
  ast_expr_t *block = va_arg(args, ast_expr_t *);

  node -> condition = ast_expr_copy(condition);
  node -> block = ast_expr_copy(block);
  return node;
}

void _ast_loop_free(ast_loop_t *node) {
  if (node) {
    ast_expr_free(node -> condition);
    ast_expr_free(node -> block);
  }
}

typedef struct _ast_loop_ctx {
  ast_loop_t *node;
  data_t     *ctx;
  data_t     *cond_val;
} ast_loop_ctx_t;

int _ast_loop_eval_condition(ast_loop_ctx_t *ctx) {
  data_t      *val;
  exception_t *ex;
  int          ret;

  if (data_is_exception(ctx -> cond_val)) {
    return 0;
  } else {
    data_free(ctx -> cond_val);
  }

  val = ast_execute(ctx -> node -> condition, ctx -> ctx);

  if (data_is_exception(val)) {
    ctx->cond_val = data_copy(val);
    ret = 0;
  } else if (data_is_ast_expr(val)) {
    ctx -> cond_val = (data_t *) ast_loop_create(ctx -> node -> condition,
                                                 ctx -> node -> block);
    ret = 0;
  } else {
    ctx -> cond_val = data_copy(val);
    ret = data_intval(val);
  }
  data_free(val);
  return ret;
}

data_t * _ast_loop_call(ast_loop_t *node, arguments_t *args) {
  ast_loop_ctx_t  ctx;
  int             cond;
  data_t         *val;
  data_t         *ret;

  ctx.node = node;
  ctx.cond_val = NULL;
  ctx.ctx = arguments_get_arg(args, 0);
  for (cond = _ast_loop_eval_condition(&ctx); cond; cond = _ast_loop_eval_condition(&ctx)) {
    val = ast_execute(node -> block, ctx.ctx);
    if (data_is_exception(val)) {
      ctx.cond_val = data_copy(val);
    }
    data_free(val);
  }
  debug(ast, "%s -> %s", ast_loop_tostring(node), data_tostring(ctx.cond_val));
  if (data_is_exception(ctx.cond_val)) {
    ret = ctx.cond_val;
  } else if (data_is_ast_expr(ctx.cond_val)) {
    ret = ctx.cond_val;
  } else {
    ret = (data_t *) ast_const_create(ctx.cond_val);
    data_free(ctx.cond_val);
  }
  return ret;
}

char * _ast_loop_tostring(ast_loop_t *node) {
  char *ret;
  asprintf(&ret, "for ( %s ) %s",
           ast_expr_tostring(node -> condition),
           ast_expr_tostring(node -> block));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_script_t *_ast_script_new(ast_script_t *script, va_list args) {
  char *name = va_arg(args, char *);
  char anon[40];

  if (!name) {
    (size_t) snprintf(anon, 40, "__anon__%d__", hashptr(script));
    name = anon;
  }

  script->name = name_parse(name);
  return script;
}

char *_ast_script_tostring(ast_script_t *script) {
  return name_tostring(script->name);
}

void _ast_script_free(ast_script_t *script) {
  if (script) {
    name_free(script->name);
  }
}

/* ----------------------------------------------------------------------- */

ast_builder_t *_ast_builder_new(ast_builder_t *builder, va_list args) {
  builder->script = ast_script_create(va_arg(args, char *));
  builder->current_node = data_as_ast_node(builder->script);
  return builder;
}

char *_ast_builder_tostring(ast_builder_t *builder) {
  return ast_script_tostring(builder->script);
}

void _ast_builder_free(ast_builder_t *builder) {
  if (builder) {
    ast_script_free(builder->script);
  }
}

/* -- A S T  P U B L I C  F U N C T I O N S  ----------------------------- */

data_t * ast_execute(void *ast, data_t *ctx) {
  data_t      *node;
  arguments_t *args;
  data_t      *ret;
  data_t      *r;

  if (!data_is_ast_node(ast)) {
    return data_exception(ErrorType, "ast_execute called with %s (%s)",
                          data_tostring(ast), data_typename(ast));
  }
  node = (data_t *) ast;
  args = arguments_create_args(1, ctx);
  ret = data_call(node, args);
  if (data_is_ast_const(ret)) {
    // Maybe we need a more generic ast_node_unwrap method?
    r = ret;
    ret = data_copy(data_as_ast_const(r) -> value);
    data_free(r);
  }
  arguments_free(args);
  return ret;
}

ast_node_t *ast_append(ast_node_t *node, ast_node_t *child) {
  datalist_push(node->children, child);
  child->parent = node;
  return node;
}

