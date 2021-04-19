#pragma clang diagnostic push
#pragma ide diagnostic ignored "altera-struct-pack-align"
/*
 * /obelix/include/ast.h - Copyright (c) 2020 Jan de Visser <jan@de-visser.net>
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

#ifndef __AST_H__
#define __AST_H__

#include <stdarg.h>

#include <oblconfig.h>
#include <data.h>
#include <datastack.h>
#include <exception.h>
#include <list.h>
#include <name.h>
#include <nvp.h>
#include <set.h>

#ifdef  __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef OBLAST_IMPEXP
#define OBLAST_IMPEXP	__DLL_IMPORT__
#endif /* OBLAST_IMPEXP */

OBLAST_IMPEXP void             ast_init(void);

/* ----------------------------------------------------------------------- */

typedef struct _ast_Node {
  data_t            _d;
  struct _ast_Node *parent;
  datalist_t       *children;
} ast_Node_t;
OBLAST_IMPEXP int ASTNode;
type_skel(ast_Node, ASTNode, ast_Node_t);

/* ----------------------------------------------------------------------- */

#define ENUMERATE_AST_NODE_TYPES                                                                                   \
  __ENUMERATE_AST_NODE_TYPE(Expr, Node)                                                                            \
  __ENUMERATE_AST_NODE_TYPE(Const, Expr, data_t *value)                                                            \
  __ENUMERATE_AST_NODE_TYPE(Infix, Expr, data_t *op; ast_Expr_t *left; ast_Expr_t *right)                          \
  __ENUMERATE_AST_NODE_TYPE(Prefix, Expr, data_t *op; ast_Expr_t *operand)                                         \
  __ENUMERATE_AST_NODE_TYPE(Ternary, Expr, ast_Expr_t *condition; ast_Expr_t *true_value; ast_Expr_t *false_value) \
  __ENUMERATE_AST_NODE_TYPE(Variable, Expr, name_t *name)                                                          \
  __ENUMERATE_AST_NODE_TYPE(Generator, Expr, data_t *generator; data_t *iter)                                      \
  __ENUMERATE_AST_NODE_TYPE(Loop, Expr, ast_Expr_t *condition; ast_Expr_t *block)                                  \
  __ENUMERATE_AST_NODE_TYPE(Call, Expr, ast_Expr_t *function; arguments_t *args)                                   \
  __ENUMERATE_AST_NODE_TYPE(Block, Expr, name_t *name; datalist_t *statements)                                     \
  __ENUMERATE_AST_NODE_TYPE(Script, Block)                                                                         \
  __ENUMERATE_AST_NODE_TYPE(Return, Expr, ast_Expr_t *expr)                                                        \
  __ENUMERATE_AST_NODE_TYPE(Assignment, Expr, name_t *name; ast_Expr_t *value)                                     \
  __ENUMERATE_AST_NODE_TYPE(ConstAssignment, Expr)                                                                 \
  __ENUMERATE_AST_NODE_TYPE(Pass, Expr)


#define __ENUMERATE_AST_NODE_TYPE(t, base, ...)            \
typedef struct _ast_ ## t {                                \
  ast_ ## base ## _t  base ;                               \
  __VA_ARGS__;                                             \
} ast_ ## t ## _t;                                         \
OBLAST_IMPEXP int AST ## t;                                \
type_skel(ast_ ## t, AST ## t, ast_ ## t ## _t);
ENUMERATE_AST_NODE_TYPES
#undef __ENUMERATE_AST_NODE_TYPE

static inline ast_Const_t * ast_Const_create(data_t *value) {
  ast_init();
  return (ast_Const_t *) data_create(ASTConst, value);
}

static inline ast_Infix_t * ast_Infix_create(ast_Expr_t *left, data_t *op, ast_Expr_t *right) {
  ast_init();
  return (ast_Infix_t *) data_create(ASTInfix, left, op, right);
}

static inline ast_Prefix_t * ast_Prefix_create(data_t *op, ast_Expr_t *operand) {
  ast_init();
  return (ast_Prefix_t *) data_create(ASTPrefix, op, operand);
}

static inline ast_Ternary_t * ast_Ternary_create(ast_Expr_t *condition,
                                                 ast_Expr_t *true_value,
                                                 ast_Expr_t *false_value) {
  ast_init();
  return (ast_Ternary_t *) data_create(ASTTernary, condition, true_value, false_value);
}

static inline ast_Variable_t * ast_Variable_create(name_t *name) {
  ast_init();
  return (ast_Variable_t *) data_create(ASTVariable, name);
}

static inline ast_Generator_t * ast_Generator_create(data_t *generator) {
  ast_init();
  return (ast_Generator_t *) data_create(ASTGenerator, generator);
}

static inline ast_Call_t * ast_Call_create(ast_Expr_t *function) {
  ast_init();
  return (ast_Call_t *) data_create(ASTCall, function);
}

void ast_Call_add_argument(ast_Call_t *, ast_Expr_t *);
void ast_Call_add_kwarg(ast_Call_t *, ast_Const_t *, ast_Expr_t *);

/* ----------------------------------------------------------------------- */

static inline ast_Block_t * ast_Block_create(char *name) {
  ast_init();
  return (ast_Block_t *) data_create(ASTBlock, name);
}

void ast_Block_add_statement(ast_Block_t *, ast_Expr_t *);

static inline ast_Loop_t * ast_Loop_create(ast_Expr_t *condition, ast_Expr_t *block) {
  ast_init();
  return (ast_Loop_t *) data_create(ASTLoop, condition, block);
}

static inline ast_Script_t *ast_Script_create(char *name) {
  ast_init();
  return (ast_Script_t *) data_create(ASTScript, name);
}

static inline ast_Assignment_t * ast_Assignment_create(name_t *name, ast_Expr_t *value) {
  ast_init();
  return (ast_Assignment_t *) data_create(ASTAssignment, name, value);
}

static inline ast_ConstAssignment_t * ast_ConstAssignment_create(name_t *name, ast_Expr_t *value) {
  ast_init();
  return (ast_ConstAssignment_t *) data_create(ASTConstAssignment, name, value);
}

static inline ast_Pass_t * ast_Pass_create() {
  return (ast_Pass_t *) data_create(ASTPass);
}

static inline ast_Return_t * ast_Return_create(ast_Expr_t *ret_expr) {
  return (ast_Return_t *) data_create(ASTReturn, ret_expr);
}

/* ----------------------------------------------------------------------- */

OBLAST_IMPEXP data_t *         ast_parse(void *, data_t *);
OBLAST_IMPEXP data_t *         ast_execute(void *, data_t *);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __AST_H__ */

#pragma clang diagnostic pop