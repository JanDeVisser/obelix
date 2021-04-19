/* 
 * This file is part of the obelix distribution (https://github.com/JanDeVisser/obelix).
 * Copyright (c) 2021 Jan de Visser.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "libast.h"

/* ----------------------------------------------------------------------- */

ast_Pass_t *_ast_Pass_new(ast_Pass_t *node, va_list args) {
  return node;
}

void _ast_Pass_free(ast_Pass_t *node) {
}

data_t * _ast_Pass_call(ast_Pass_t *node, arguments_t *args) {
  debug(ast, "%s", data_tostring(node));
  return data_as_data(ast_Const_create(data_null()));
}

char * _ast_Pass_tostring(ast_Pass_t *node) {
  return "Pass";
}

/* ----------------------------------------------------------------------- */

ast_Block_t *_ast_Block_new(ast_Block_t *node, va_list args) {
  char *name = va_arg(args, char *);
  char anon[40];

  if (!(name && *name)) {
    (size_t) snprintf(anon, 40, "__anon__%d__", hashptr(node));
    name = anon;
  }
  node->name = name_parse(name);
  node -> statements = datalist_create(NULL);
  return node;
}

void _ast_Block_free(ast_Block_t *node) {
  if (node) {
    name_free(node->name);
    datalist_free(node->statements);
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
  asprintf(&ret, "%s %s [%d expression(s)]",
           data_typename(node),
           name_tostring(node->name),
           datalist_size(node->statements));
  return ret;
}

void ast_Block_add_statement(ast_Block_t *block, ast_Expr_t *statement) {
  datalist_push(block->statements, statement);
  data_invalidate_string(data_as_data(block));
  debug(ast, "Added expression '%s' to block '%s'",
        data_tostring(statement),
        data_tostring(block));
}

/* ----------------------------------------------------------------------- */

ast_Script_t *_ast_Script_new(ast_Script_t *script, va_list args) {
  return script;
}

char *_ast_Script_tostring(ast_Script_t *script) {
}

data_t * _ast_Script_call(ast_Script_t *node, arguments_t *args) {
  // FIXME implement
  return data_null();
}

void _ast_Script_free(ast_Script_t *script) {
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
  int          do_assign = data_intval(arguments_get_arg(args, 1));
  data_t      *val;
  data_t      *ret;
  exception_t *ex;

  val = ast_execute(node -> value, ctx);

  if (data_is_exception(val)) {
    ex = data_as_exception(val);
    if (ex->code == ErrorExhausted) {
      ret = data_as_data(ast_Const_create(data_false()));
    } else {
      ret = data_copy(val);
    }
  } else if (data_is_ast_Const(val) && do_assign) {
    ret = data_set(ctx, node -> name, val);
    if (!data_is_exception(ret)) {
      data_free(ret);
      ret = data_copy(val);
    }
  } else {
    ret = (data_t *) ast_Assignment_create(name_copy(node -> name), data_as_ast_Expr(val));
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

ast_ConstAssignment_t *_ast_ConstAssignment_new(ast_ConstAssignment_t *node, va_list args) {
  return node;
}


void _ast_ConstAssignment_free(ast_ConstAssignment_t *node) {
}

data_t * _ast_ConstAssignment_call(ast_ConstAssignment_t *node, arguments_t *args) {
  data_t      *ctx = arguments_get_arg(args, 0);
  int          do_assign = data_intval(arguments_get_arg(args, 1));
  data_t      *val;
  data_t      *ret;
  exception_t *ex;

  val = ast_execute(data_as_ast_Assignment(node)->value, ctx);

  if (data_is_exception(val)) {
    ex = data_as_exception(val);
    if (ex->code == ErrorExhausted) {
      ret = data_as_data(ast_Const_create(data_false()));
    } else {
      ret = data_copy(val);
    }
  } else if (data_is_ast_Const(val)) {
    ret = data_set(ctx, data_as_ast_Assignment(node)->name, val);
    if (!data_is_exception(ret)) {
      data_free(ret);
      ret = data_copy(val);
    }
  } else {
    ret = (data_t *) ast_ConstAssignment_create(name_copy(data_as_ast_Assignment(node)->name), data_as_ast_Expr(val));
  }
  debug(ast, "%s -> %s", data_tostring(val), data_tostring(ret));
  data_free(val);
  return ret;
}

char * _ast_ConstAssignment_tostring(ast_ConstAssignment_t *node) {
  char *assignment = _ast_Assignment_tostring(data_as_ast_Assignment(node));
  char *ret;
  asprintf(&ret, "const %s", assignment);
  free(assignment);
  return ret;
}

/* ----------------------------------------------------------------------- */

ast_Return_t *_ast_Return_new(ast_Return_t *node, va_list args) {
  node->expr = va_arg(args, ast_Expr_t *);
  return node;
}

void _ast_Return_free(ast_Return_t *node) {
  if (node != NULL) {
    ast_Expr_free(node->expr);
  }
}

data_t * _ast_Return_call(ast_Return_t *node, arguments_t *args) {
  data_t      *ret_expr;
  exception_t *ex;
  data_t      *ret;
  int          resolve_all = data_intval(arguments_get_arg(args, 1));

  ret_expr = data_call((data_t *) node->expr, args);
  if (data_is_ast_Const(ret_expr) && resolve_all) {
    ex = exception_create(ErrorReturn, "Return Value");
    ex->throwable = ret_expr;
    ret = (data_t *) ex;
  } else if (data_is_ast_Expr(ret_expr)) {
    ret = (data_t *) ast_Return_create(data_as_ast_Expr(ret_expr));
  } else {
    ret = ret_expr;
  }
  debug(ast, "%s", ret);
  return ret;
}

char * _ast_Return_tostring(ast_Return_t *node) {
  char *ret;
  asprintf(&ret, "return %s", ast_Expr_tostring(node->expr));
  return ret;
}

