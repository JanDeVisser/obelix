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

static ast_Node_t *       _ast_Node_new(ast_Node_t *, va_list);
static void               _ast_Node_free(ast_Node_t *);

static ast_builder_t *    _ast_builder_new(ast_builder_t *, va_list);
static void               _ast_builder_free(ast_builder_t *);
static char *             _ast_builder_tostring(ast_builder_t *);

/* -- data_t type description structures ---------------------------------- */

static vtable_t _vtable_ASTNode[] = {
  { .id = FunctionNew, .fnc = (void_t) _ast_Node_new },
  { .id = FunctionFree, .fnc = (void_t) _ast_Node_free },
  { .id = FunctionNone, .fnc = NULL }
};

#define AST_NODE_TYPE(t)                                                           \
static ast_ ## t ## _t * _ast_ ## t  ## _new(ast_ ## t  ## _t *, va_list);         \
static void              _ast_ ## t  ## _free(ast_ ## t ## _t *);                  \
static data_t *          _ast_ ## t  ## _call(ast_ ## t  ## _t *, arguments_t *);  \
static char *            _ast_ ## t  ## _tostring(ast_ ## t  ## _t *);             \
                                                                                   \
static vtable_t _vtable_AST ## t[] = {                                             \
  { .id = FunctionNew,         .fnc = (void_t) _ast_ ## t ## _new },               \
  { .id = FunctionFree,        .fnc = (void_t) _ast_ ## t ## _free },              \
  { .id = FunctionCall,        .fnc = (void_t) _ast_ ## t ## _call },              \
  { .id = FunctionAllocString, .fnc = (void_t) _ast_ ## t ## _tostring },          \
  { .id = FunctionNone,        .fnc = NULL }                                       \
};                                                                                 \
int AST ## t = -1;

#define AST_DUMMY_NODE_TYPE(t)                                                     \
static vtable_t _vtable_AST ## t[] = {                                             \
  { .id = FunctionNone,        .fnc = NULL }                                       \
};                                                                                 \
int AST ## t = -1;

static vtable_t _vtable_ASTExpr[] = {
  { .id = FunctionNone, .fnc = NULL }
};
int ASTExpr = -1;

AST_NODE_TYPE(Const);
AST_NODE_TYPE(Infix);
AST_NODE_TYPE(Prefix);
AST_NODE_TYPE(Ternary);
AST_NODE_TYPE(Variable);
AST_NODE_TYPE(Generator);
AST_NODE_TYPE(Call);
AST_NODE_TYPE(Block);
AST_NODE_TYPE(Assignment);
AST_NODE_TYPE(Loop);
AST_NODE_TYPE(Script);
AST_DUMMY_NODE_TYPE(Statement);
AST_DUMMY_NODE_TYPE(If);
AST_DUMMY_NODE_TYPE(Pass);
AST_DUMMY_NODE_TYPE(While);

static vtable_t _vtable_ASTBuilder[] = {
  { .id = FunctionNew, .fnc = (void_t) _ast_builder_new },
  { .id = FunctionFree, .fnc = (void_t) _ast_builder_free },
  { .id = FunctionToString, .fnc = (void_t) _ast_builder_tostring },
  { .id = FunctionNone, .fnc = NULL }
};

int ASTNode = -1;

int ASTBuilder = -1;

int ast_debug = 0;

/* ------------------------------------------------------------------------ */

void ast_init(void) {
  if (ASTNode < 0) {
    logging_register_module(ast);
    typedescr_register(ASTNode, ast_Node_t);

    typedescr_register(ASTExpr, ast_Expr_t);
    typedescr_assign_inheritance(ASTExpr, ASTNode);
    typedescr_register(ASTConst, ast_Const_t);
    typedescr_assign_inheritance(ASTConst, ASTExpr);
    typedescr_register(ASTInfix, ast_Infix_t);
    typedescr_assign_inheritance(ASTInfix, ASTExpr);
    typedescr_register(ASTPrefix, ast_Prefix_t);
    typedescr_assign_inheritance(ASTPrefix, ASTExpr);
    typedescr_register(ASTTernary, ast_Ternary_t);
    typedescr_assign_inheritance(ASTTernary, ASTExpr);
    typedescr_register(ASTVariable, ast_Variable_t);
    typedescr_assign_inheritance(ASTVariable, ASTExpr);
    typedescr_register(ASTGenerator, ast_Generator_t);
    typedescr_assign_inheritance(ASTGenerator, ASTExpr);
    typedescr_register(ASTCall, ast_Call_t);
    typedescr_assign_inheritance(ASTCall, ASTExpr);

    typedescr_register(ASTBlock, ast_Block_t);
    typedescr_assign_inheritance(ASTBlock, ASTExpr);
    typedescr_register(ASTAssignment, ast_Assignment_t);
    typedescr_assign_inheritance(ASTAssignment, ASTExpr);
    typedescr_register(ASTLoop, ast_Loop_t);
    typedescr_assign_inheritance(ASTLoop, ASTExpr);
  }
}

/* -- A S T  S T A T I C  F U N C T I O N S  ----------------------------- */

ast_Node_t *_ast_Node_new(ast_Node_t *node, va_list args) {
  node->children = datalist_create(NULL);
  return node;
}

void _ast_Node_free(ast_Node_t *node) {
  if (node) {
    ast_Node_free(node->parent);
  }
}

/* ----------------------------------------------------------------------- */

ast_Const_t *_ast_Const_new(ast_Const_t *node, va_list args) {
  data_t *value = va_arg(args, data_t *);

  if (!value) {
    value = data_null();
  }
  node -> value = data_copy(value);
  return node;
}

void _ast_Const_free(ast_Const_t *node) {
  if (node) {
    data_free(node -> value);
  }
}

data_t * _ast_Const_call(ast_Const_t *node, arguments_t *args) {
  debug(ast, "%s", data_tostring(node));
  return data_copy(node);
}

char * _ast_Const_tostring(ast_Const_t *node) {
  char *ret;
  asprintf(&ret, "'%s':%s", data_tostring(node -> value), data_typename(node -> value));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Infix_t *_ast_Infix_new(ast_Infix_t *node, va_list args) {
  ast_Expr_t *left = va_arg(args, ast_Expr_t *);
  data_t     *op = va_arg(args, data_t *);
  ast_Expr_t *right = va_arg(args, ast_Expr_t *);

  node -> left = ast_Expr_copy(left);
  if (data_is_token(op)) {
    node -> op = str_to_data(token_token((token_t *) op));
  } else if (data_is_string(op)){
    node -> op = data_copy(op);
  } else {
    node -> op = str_to_data(data_tostring(op));
  }
  node -> right = ast_Expr_copy(right);
  return node;
}

void _ast_Infix_free(ast_Infix_t *node) {
  if (node) {
    ast_Expr_free(node -> left);
    data_free(node -> op);
    ast_Expr_free(node -> right);
  }
}

data_t * _ast_Infix_call(ast_Infix_t *node, arguments_t *args) {
  data_t      *left_val;
  data_t      *right_val;
  arguments_t *op_args;
  data_t      *ret_val;
  data_t      *ret;

  left_val = data_call((data_t *) node -> left, args);
  right_val = data_call((data_t *) node -> right, args);

  if (!data_is_ast_Const(left_val) || !data_is_ast_Const(right_val)) {
    ret = (data_t *) ast_Infix_create(
      data_as_ast_Expr(left_val),
      data_copy(node -> op),
      data_as_ast_Expr(right_val));
  } else {
    op_args = arguments_create_args(1, data_as_ast_Const(right_val)->value);
    ret_val = data_execute(data_as_ast_Const(left_val)->value,
                           data_tostring(node->op),
                           op_args);
    arguments_free(op_args);
    ret = (data_t *) ast_Const_create(ret_val);
    data_free(ret_val);
  }
  debug(ast, "%s %s %s = %s",
        data_tostring(node ->left),
        data_tostring(node -> op),
        data_tostring(node -> right),
        data_tostring(ret));
  return ret;
}

char * _ast_Infix_tostring(ast_Infix_t *node) {
  char *ret;
  asprintf(&ret, "(%s %s %s)",
           data_tostring(node -> left),
           data_tostring(node -> op),
           data_tostring(node -> right));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Prefix_t *_ast_Prefix_new(ast_Prefix_t *node, va_list args) {
  data_t     *op = va_arg(args, data_t *);
  ast_Expr_t *operand = va_arg(args, ast_Expr_t *);

  if (data_is_token(op)) {
    node -> op = str_to_data(token_token((token_t *) op));
  } else if (data_is_string(op)){
    node -> op = data_copy(op);
  } else {
    node -> op = str_to_data(data_tostring(op));
  }
  node -> operand = ast_Expr_copy(operand);
  return node;
}

void _ast_Prefix_free(ast_Prefix_t *node) {
  if (node) {
    data_free(node -> op);
    ast_Expr_free(node -> operand);
  }
}

data_t * _ast_Prefix_call(ast_Prefix_t *node, arguments_t *args) {
  data_t      *operand_val;
  arguments_t *op_args;
  data_t      *ret_val;
  data_t      *ret;

  operand_val = data_call((data_t *) node->operand, args);
  if (!strcmp(data_tostring(node->op), "+")) {
    ret = operand_val;
  } else {
    if (!data_is_ast_Const(operand_val)) {
      ret = (data_t *) ast_Prefix_create(
        data_copy(node->op),
        data_as_ast_Expr(operand_val));
    } else {
      op_args = arguments_create_args(0);
      ret_val = data_execute(data_as_ast_Const(operand_val)->value,
                             data_tostring(node->op),
                             op_args);
      arguments_free(op_args);
      ret = (data_t *) ast_Const_create(ret_val);
      data_free(ret_val);
    }
  }
  debug(ast, "%s %s = %s",
        data_tostring(node -> op),
        data_tostring(node -> operand),
        data_tostring(ret));
  return ret;
}

char * _ast_Prefix_tostring(ast_Prefix_t *node) {
  char *ret;
  asprintf(&ret, "%s (%s)",
           data_tostring(node -> op),
           data_tostring(node -> operand));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Ternary_t *_ast_Ternary_new(ast_Ternary_t *node, va_list args) {
  ast_Expr_t *condition = va_arg(args, ast_Expr_t *);
  ast_Expr_t *true_value = va_arg(args, ast_Expr_t *);
  ast_Expr_t *false_value = va_arg(args, ast_Expr_t *);

  node -> condition = ast_Expr_copy(condition);
  node -> true_value = ast_Expr_copy(true_value);
  node -> false_value = ast_Expr_copy(false_value);
  return node;
}

void _ast_Ternary_free(ast_Ternary_t *node) {
  if (node) {
    ast_Expr_free(node -> condition);
    ast_Expr_free(node -> true_value);
    ast_Expr_free(node -> false_value);
  }
}

data_t * _ast_Ternary_call(ast_Ternary_t *node, arguments_t *args) {
  data_t      *condition_val;
  data_t      *casted = NULL;
  data_t      *ret;

  condition_val = data_call((data_t *) node -> condition, args);

  if (!data_is_ast_Const(condition_val)) {
    ret = (data_t *) ast_Ternary_create(
      data_as_ast_Expr(condition_val),
      data_as_ast_Expr(node -> true_value),
      data_as_ast_Expr(node -> false_value));
  } else {
    casted = data_cast(data_as_ast_Const(condition_val) -> value, Bool);
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

char * _ast_Ternary_tostring(ast_Ternary_t *node) {
  char *ret;
  asprintf(&ret, "(%s) ? (%s) : (%s)",
           data_tostring(node -> condition),
           data_tostring(node -> true_value),
           data_tostring(node -> false_value));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Variable_t *_ast_Variable_new(ast_Variable_t *node, va_list args) {
  name_t     *name = va_arg(args, name_t *);

  node -> name = name_copy(name);
  return node;
}

void _ast_Variable_free(ast_Variable_t *node) {
  if (node) {
    name_free(node -> name);
  }
}

data_t * _ast_Variable_call(ast_Variable_t *node, arguments_t *args) {
  data_t *ctx = arguments_get_arg(args, 0);
  data_t *val;
  data_t *ret;

  val = data_get(ctx, node -> name);

  if (!data_is_exception(val)) {
    ret = (data_t *) ast_Const_create(val);
    data_free(val);
  } else {
    ret = val;
  }
  debug(ast, "%s = %s", name_tostring(node -> name), data_tostring(ret));
  return ret;
}

char * _ast_Variable_tostring(ast_Variable_t *node) {
  char *ret;
  asprintf(&ret, "[%s]",
           name_tostring(node -> name));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Generator_t *_ast_Generator_new(ast_Generator_t *node, va_list args) {
  data_t *generator = va_arg(args, data_t *);

  node -> generator = data_copy(generator);
  node -> iter = NULL;
  return node;
}

void _ast_Generator_free(ast_Generator_t *node) {
  if (node) {
    data_free(node -> generator);
  }
}

data_t * _ast_Generator_call(ast_Generator_t *node, arguments_t *args) {
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
    ret = (data_t *) ast_Const_create(val);
    data_free(val);
  } else {
    ret = val;
  }
  debug(ast, "%s = %s", ast_Generator_tostring(node), data_tostring(ret));
  return ret;
}

char * _ast_Generator_tostring(ast_Generator_t *node) {
  char *ret;
  asprintf(&ret, " .. %s .. ", data_tostring(node -> generator));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Call_t *_ast_Call_new(ast_Call_t *node, va_list args) {
  ast_Expr_t *function = va_arg(args, ast_Expr_t *);

  node -> function = ast_Expr_copy(function);
  node -> args = NULL;
  return node;
}

void _ast_Call_free(ast_Call_t *node) {
  if (node) {
    ast_Expr_free(node -> function);
    arguments_free(node -> args);
  }
}

typedef struct _arg_reduce_ctx {
  arguments_t *arg_vals;
  arguments_t *arg_atoms;
  arguments_t *args;
  int          all_resolved;
} arg_reduce_ctx_t;

arg_reduce_ctx_t * _ast_Call_execute_arg(ast_Expr_t *arg, arg_reduce_ctx_t *ctx) {
  data_t    *arg_val;

  arg_val = data_call((data_t *) arg, ctx -> args);
  ctx -> all_resolved &= data_is_ast_Const(arg_val);
  arguments_push(ctx -> arg_vals, arg_val);
  arguments_push(ctx -> arg_atoms,
                 data_is_ast_Const(arg_val) ? data_as_ast_Const(arg_val) -> value : data_null());
  return ctx;
}

data_t *_ast_Call_call(ast_Call_t *node, arguments_t *args) {
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
  if (!data_is_ast_Const(fnc_val)) {
    ret = (data_t *) ast_Call_create(ast_Expr_copy(node->function));
  }

  if (!ret) {
    reduce_ctx.arg_vals = arguments_create(NULL, NULL);
    reduce_ctx.arg_atoms = arguments_create(NULL, NULL);
    reduce_ctx.all_resolved = TRUE;
    arguments_reduce_args(node->args, (reduce_t) _ast_Call_execute_arg, &reduce_ctx);
    if (!reduce_ctx.all_resolved) {
      ret = (data_t *) ast_Call_create(data_as_ast_Expr(fnc));
    }
  }

  if (!ret) {
    fnc = data_as_ast_Const(fnc_val) -> value;
    if (data_is_callable(fnc)) {
      ret_val = data_call(fnc, reduce_ctx.arg_atoms);
      if (!data_is_exception(ret_val)) {
        ret = (data_t *) ast_Const_create(ret_val);
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

  if (data_is_ast_Call(ret)) {
    data_as_ast_Call(ret)->args = arguments_copy(reduce_ctx.arg_vals);
  }
  debug(ast, "%s = %s",
        ast_Call_tostring(node),
        data_tostring(ret));
  return ret;
}

char * _ast_Call_tostring(ast_Call_t *node) {
  char *ret;
  asprintf(&ret, "%s(%s)",
           ast_Expr_tostring(node -> function),
           arguments_tostring(node->args));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Block_t *_ast_Block_new(ast_Block_t *node, va_list args) {
  node -> statements = datalist_create(NULL);
  return node;
}

void _ast_Block_free(ast_Block_t *node) {
  if (node) {
    datalist_free(node -> statements);
  }
}

typedef struct _ast_Block_execute_ctx {
  arguments_t *args;
  data_t      *ret;
} ast_Block_execute_ctx_t;

ast_Block_execute_ctx_t * _ast_Block_execute_statement(ast_Expr_t *stmt, ast_Block_execute_ctx_t *ctx) {
  if (!data_is_exception(ctx -> ret)) {
    data_free(ctx -> ret);
    ctx -> ret = data_call((data_t *) stmt, ctx -> args);
  }
  return ctx;
}

data_t * _ast_Block_call(ast_Block_t *node, arguments_t *args) {
  array_t                 *statements;
  ast_Block_execute_ctx_t  ctx;

  debug(ast, "%s", ast_Block_tostring(node));
  statements = datalist_to_array(node -> statements);
  ctx.args = args;
  ctx.ret = data_null();
  array_reduce(statements, (reduce_t) _ast_Block_execute_statement, &ctx);
  array_free(statements);
  debug(ast, "%s -> %s",
        ast_Block_tostring(node),
        data_tostring(ctx.ret));
  return ctx.ret;
}

char * _ast_Block_tostring(ast_Block_t *node) {
  char *ret;
  asprintf(&ret, "{ %d node(s) }", datalist_size(node -> statements));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Assignment_t *_ast_Assignment_new(ast_Assignment_t *node, va_list args) {
  name_t     *name = va_arg(args, name_t *);
  ast_Expr_t *value = va_arg(args, ast_Expr_t *);

  node -> name = name_copy(name);
  node -> value = ast_Expr_copy(value);
  return node;
}

void _ast_Assignment_free(ast_Assignment_t *node) {
  if (node) {
    name_free(node -> name);
    ast_Expr_free(node -> value);
  }
}

data_t * _ast_Assignment_call(ast_Assignment_t *node, arguments_t *args) {
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
  } else if (data_is_ast_Expr(val)) {
    ret = (data_t *) ast_Assignment_create(name_copy(node -> name), data_as_ast_Expr(val));
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

char * _ast_Assignment_tostring(ast_Assignment_t *node) {
  char *ret;
  asprintf(&ret, "[%s] := %s",
           name_tostring(node -> name),
           ast_Expr_tostring(node -> value));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Loop_t *_ast_Loop_new(ast_Loop_t *node, va_list args) {
  ast_Expr_t *condition = va_arg(args, ast_Expr_t *);
  ast_Expr_t *block = va_arg(args, ast_Expr_t *);

  node -> condition = ast_Expr_copy(condition);
  node -> block = ast_Expr_copy(block);
  return node;
}

void _ast_Loop_free(ast_Loop_t *node) {
  if (node) {
    ast_Expr_free(node -> condition);
    ast_Expr_free(node -> block);
  }
}

typedef struct _ast_Loop_ctx {
  ast_Loop_t *node;
  data_t     *ctx;
  data_t     *cond_val;
} ast_Loop_ctx_t;

int _ast_Loop_eval_condition(ast_Loop_ctx_t *ctx) {
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
  } else if (data_is_ast_Expr(val)) {
    ctx -> cond_val = (data_t *) ast_Loop_create(ctx -> node -> condition,
                                                 ctx -> node -> block);
    ret = 0;
  } else {
    ctx -> cond_val = data_copy(val);
    ret = data_intval(val);
  }
  data_free(val);
  return ret;
}

data_t * _ast_Loop_call(ast_Loop_t *node, arguments_t *args) {
  ast_Loop_ctx_t  ctx;
  int             cond;
  data_t         *val;
  data_t         *ret;

  ctx.node = node;
  ctx.cond_val = NULL;
  ctx.ctx = arguments_get_arg(args, 0);
  for (cond = _ast_Loop_eval_condition(&ctx); cond; cond = _ast_Loop_eval_condition(&ctx)) {
    val = ast_execute(node -> block, ctx.ctx);
    if (data_is_exception(val)) {
      ctx.cond_val = data_copy(val);
    }
    data_free(val);
  }
  debug(ast, "%s -> %s", ast_Loop_tostring(node), data_tostring(ctx.cond_val));
  if (data_is_exception(ctx.cond_val)) {
    ret = ctx.cond_val;
  } else if (data_is_ast_Expr(ctx.cond_val)) {
    ret = ctx.cond_val;
  } else {
    ret = (data_t *) ast_Const_create(ctx.cond_val);
    data_free(ctx.cond_val);
  }
  return ret;
}

char * _ast_Loop_tostring(ast_Loop_t *node) {
  char *ret;
  asprintf(&ret, "for ( %s ) %s",
           ast_Expr_tostring(node -> condition),
           ast_Expr_tostring(node -> block));
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Script_t *_ast_Script_new(ast_Script_t *script, va_list args) {
  char *name = va_arg(args, char *);
  char anon[40];

  if (!name) {
    (size_t) snprintf(anon, 40, "__anon__%d__", hashptr(script));
    name = anon;
  }

  script->name = name_parse(name);
  return script;
}

char *_ast_Script_tostring(ast_Script_t *script) {
  return name_tostring(script->name);
}

data_t * _ast_Script_call(ast_Script_t *node, arguments_t *args) {
  // FIXME implement
  return data_null();
}

void _ast_Script_free(ast_Script_t *script) {
  if (script) {
    name_free(script->name);
  }
}

/* ----------------------------------------------------------------------- */

ast_builder_t *_ast_builder_new(ast_builder_t *builder, va_list args) {
  builder->script = ast_Script_create(va_arg(args, char *));
  builder->current_node = data_as_ast_Node(builder->script);
  return builder;
}

char *_ast_builder_tostring(ast_builder_t *builder) {
  return ast_Script_tostring(builder->script);
}

void _ast_builder_free(ast_builder_t *builder) {
  if (builder) {
    ast_Script_free(builder->script);
  }
}

/* -- A S T  P U B L I C  F U N C T I O N S  ----------------------------- */

data_t * ast_execute(void *ast, data_t *ctx) {
  data_t      *node;
  arguments_t *args;
  data_t      *ret;
  data_t      *r;

  if (!data_is_ast_Node(ast)) {
    return data_exception(ErrorType, "ast_execute called with %s (%s)",
                          data_tostring(ast), data_typename(ast));
  }
  node = (data_t *) ast;
  args = arguments_create_args(1, ctx);
  ret = data_call(node, args);
  if (data_is_ast_Const(ret)) {
    // Maybe we need a more generic ast_node_unwrap method?
    r = ret;
    ret = data_copy(data_as_ast_Const(r) -> value);
    data_free(r);
  }
  arguments_free(args);
  return ret;
}

ast_Node_t *ast_append(ast_Node_t *node, ast_Node_t *child) {
  datalist_push(node->children, child);
  child->parent = node;
  return node;
}

