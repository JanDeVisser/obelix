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

static data_t *           _ast_parse(void *, data_t *, int resolve_all);

/* -- data_t type description structures ---------------------------------- */

static vtable_t _vtable_ASTNode[] = {
  { .id = FunctionNew, .fnc = (void_t) _ast_Node_new },
  { .id = FunctionFree, .fnc = (void_t) _ast_Node_free },
  { .id = FunctionNone, .fnc = NULL }
};

int ASTNode = -1;

#define __ENUMERATE_AST_NODE_TYPE(t, base, ...)                                    \
static vtable_t _vtable_AST ## t[] = {                                             \
  { .id = FunctionNew,         .fnc = (void_t) _ast_ ## t ## _new },               \
  { .id = FunctionFree,        .fnc = (void_t) _ast_ ## t ## _free },              \
  { .id = FunctionCall,        .fnc = (void_t) _ast_ ## t ## _call },              \
  { .id = FunctionAllocString, .fnc = (void_t) _ast_ ## t ## _tostring },          \
  { .id = FunctionNone,        .fnc = NULL }                                       \
};                                                                                 \
int AST ## t = -1;
ENUMERATE_AST_NODE_TYPES
#undef __ENUMERATE_AST_NODE_TYPE

int ast_debug = 0;

/* ------------------------------------------------------------------------ */

void ast_init(void) {
  if (ASTNode < 0) {
    logging_register_module(ast);
    typedescr_register(ASTNode, ast_Node_t);
#define __ENUMERATE_AST_NODE_TYPE(t, base, ...)            \
    typedescr_register(AST ## t, ast_ ## t ## _t);         \
    typedescr_assign_inheritance(AST ## t, AST ## base);
ENUMERATE_AST_NODE_TYPES
#undef __ENUMERATE_AST_NODE_TYPE
  }
}

/* -- A S T  S T A T I C  F U N C T I O N S  ----------------------------- */

ast_Node_t *_ast_Node_new(ast_Node_t *node, va_list args) {
  node->children = datalist_create(NULL);
  data_set_string_semantics(node, StrSemanticsCache);
  return node;
}

void _ast_Node_free(ast_Node_t *node) {
  if (node) {
    ast_Node_free(node->parent);
  }
}

/* ----------------------------------------------------------------------- */

ast_Expr_t *_ast_Expr_new(ast_Expr_t *node, va_list args) {
  return node;
}

void _ast_Expr_free(ast_Expr_t *node) {
}

data_t * _ast_Expr_call(ast_Expr_t *node, arguments_t *args) {
  return data_as_data(ast_Const_create(data_null()));
}

char * _ast_Expr_tostring(ast_Expr_t *node) {
  return "Expr";
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
  int     resolve_all = data_intval(arguments_get_arg(args, 1));
  data_t *val;
  data_t *ret;

  val = data_get(ctx, node -> name);

  if (data_is_exception(val)) {
    if ((data_as_exception(val)->code == ErrorName) && !resolve_all) {
      ret = data_copy(node);
    } else {
      ret = val;
    }
  } else {
    ret = (data_t *) ast_Const_create(val);
    data_free(val);
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
    if (data_is_ast_Expr(val)) {
      ret = ast_execute(data_as_ast_Expr(val), args);
    } else {
      ret = (data_t *) ast_Const_create(val);
    }
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
  arguments_t *args;
  arguments_t *args_processed;
  arguments_t *args_atoms;
  exception_t *error;
  int          all_resolved;
} arg_reduce_ctx_t;

arg_reduce_ctx_t * _ast_Call_execute_arg(ast_Expr_t *arg, arg_reduce_ctx_t *ctx) {
  data_t    *arg_val;

  if (ctx->error) {
    return ctx;
  }
  arg_val = data_call((data_t *) arg, ctx -> args);
  if (data_is_exception(arg_val)) {
    ctx->error = data_as_exception(arg_val);
  } else {
    ctx->all_resolved &= data_is_ast_Const(arg_val);
    arguments_push(ctx->args_processed, arg_val);
    arguments_push(ctx->args_atoms,
                   data_is_ast_Const(arg_val) ? data_as_ast_Const(arg_val)->value : data_null());
  }
  return ctx;
}

arg_reduce_ctx_t * _ast_Call_execute_kwarg(entry_t *e, arg_reduce_ctx_t *ctx) {
  data_t *arg = data_as_data(e->value);
  data_t *arg_val;

  if (ctx->error) {
    return ctx;
  }
  arg_val = data_call((data_t *) arg, ctx -> args);
  if (data_is_exception(arg_val)) {
    ctx->error = data_as_exception(arg_val);
  } else {
    ctx->all_resolved &= data_is_ast_Const(arg_val);
    arguments_set_kwarg(ctx->args_processed, e->key, arg_val);
    if (ctx->all_resolved) {
      arguments_set_kwarg(ctx->args_atoms, e->key, arg_val);
    }
  }
  return ctx;
}

data_t *_ast_Call_call(ast_Call_t *node, arguments_t *args) {
  data_t           *fnc;
  ast_Expr_t       *fnc_expr;
  data_t           *ret = NULL;
  data_t           *ret_val = NULL;
  arg_reduce_ctx_t  reduce_ctx = {
    .args_processed = NULL,
    .args = NULL,
    .args_atoms = NULL,
    .all_resolved = FALSE
  };

  fnc = data_call((data_t *) node->function, args);
  if (data_is_ast_Expr(fnc)) {
    fnc_expr = data_as_ast_Expr(fnc);
    if (!data_is_ast_Const(fnc_expr)) {
      ret = (data_t *) ast_Call_create(fnc_expr);
    }
  } else if (data_is_exception(fnc)) {
    ret = fnc;
  }

  if (!ret) {
    reduce_ctx.args = args;
    reduce_ctx.args_processed = arguments_create(NULL, NULL);
    reduce_ctx.args_atoms = arguments_create(NULL, NULL);
    reduce_ctx.error = NULL;
    reduce_ctx.all_resolved = TRUE;
    arguments_reduce_args(node->args, (reduce_t) _ast_Call_execute_arg, &reduce_ctx);
    arguments_reduce_kwargs(node->args, (reduce_t) _ast_Call_execute_kwarg, &reduce_ctx);
    if (reduce_ctx.error) {
      ret = (data_t *) reduce_ctx.error;
    } else if (!reduce_ctx.all_resolved) {
      ret = (data_t *) ast_Call_create(data_as_ast_Expr(fnc));
    }
  }

  if (!ret) {
    fnc = data_as_ast_Const(fnc) -> value;
    if (data_is_callable(fnc)) {
      ret_val = data_call(fnc, reduce_ctx.args_atoms);
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
    data_as_ast_Call(ret)->args = arguments_copy(reduce_ctx.args_processed);
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

void ast_Call_add_argument(ast_Call_t *call, ast_Expr_t *arg) {
  if (call->args == NULL) {
    call->args = arguments_create(NULL, NULL);
  }
  arguments_push(call->args, arg);
}

void ast_Call_add_kwarg(ast_Call_t *call, ast_Const_t *name, ast_Expr_t *value) {
  if (call->args == NULL) {
    call->args = arguments_create(NULL, NULL);
  }
  arguments_set_kwarg(call->args, data_tostring(name->value), data_copy(value));
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
} __attribute__((aligned(32))) ast_Loop_ctx_t;

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

/* -- A S T  P U B L I C  F U N C T I O N S  ----------------------------- */

data_t * _ast_parse(void *ast, data_t *ctx, int resolve_all) {
  data_t      *node;
  arguments_t *args;
  data_t      *ret;
  data_t      *r;

  if (!data_is_ast_Node(ast)) {
    return data_exception(ErrorType, "ast_execute called with %s (%s)",
                          data_tostring(ast), data_typename(ast));
  }
  node = (data_t *) ast;
  args = arguments_create_args(2, ctx, int_as_bool(resolve_all));
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

data_t * ast_parse(void *ast, data_t *ctx) {
  data_t *ret;

  ret = _ast_parse(ast, ctx, FALSE);
  return ret;
}


data_t * ast_execute(void *ast, data_t *ctx) {
  data_t *ret;

  ret = _ast_parse(ast, ctx, TRUE);
  return ret;
}
