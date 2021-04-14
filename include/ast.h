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

#define AST_NODE_TYPE_DEF(t, base)               \
typedef struct _ast_ ## t {                      \
  ast_ ## base ## _t  base ;                     \
  CUSTOM_FIELDS ;                                \
} ast_ ## t ## _t;                               \
OBLAST_IMPEXP int AST ## t;                      \
type_skel(ast_ ## t, AST ## t, ast_ ## t ## _t);

#define CUSTOM_FIELDS
AST_NODE_TYPE_DEF(Expr, Node)
#undef CUSTOM_FIELDS


#define CUSTOM_FIELDS data_t *value
AST_NODE_TYPE_DEF(Const, Expr)
#undef CUSTOM_FIELDS

static inline ast_Const_t * ast_Const_create(data_t *value) {
  ast_init();
  return (ast_Const_t *) data_create(ASTConst, value);
}

#define CUSTOM_FIELDS   data_t *op; ast_Expr_t *left; ast_Expr_t *right;
AST_NODE_TYPE_DEF(Infix, Expr)
#undef CUSTOM_FIELDS

static inline ast_Infix_t * ast_Infix_create(ast_Expr_t *left, data_t *op, ast_Expr_t *right) {
  ast_init();
  return (ast_Infix_t *) data_create(ASTInfix, left, op, right);
}

#define CUSTOM_FIELDS   data_t *op; ast_Expr_t *operand;
AST_NODE_TYPE_DEF(Prefix, Expr)
#undef CUSTOM_FIELDS

static inline ast_Prefix_t * ast_Prefix_create(data_t *op, ast_Expr_t *operand) {
  ast_init();
  return (ast_Prefix_t *) data_create(ASTPrefix, op, operand);
}

#define CUSTOM_FIELDS   ast_Expr_t *condition; ast_Expr_t *true_value; ast_Expr_t *false_value;
AST_NODE_TYPE_DEF(Ternary, Expr)
#undef CUSTOM_FIELDS

static inline ast_Ternary_t * ast_Ternary_create(ast_Expr_t *condition,
                                                 ast_Expr_t *true_value,
                                                 ast_Expr_t *false_value) {
  ast_init();
  return (ast_Ternary_t *) data_create(ASTTernary, condition, true_value, false_value);
}

#define CUSTOM_FIELDS   name_t     *name;
AST_NODE_TYPE_DEF(Variable, Expr)
#undef CUSTOM_FIELDS

static inline ast_Variable_t * ast_Variable_create(name_t *name) {
  ast_init();
  return (ast_Variable_t *) data_create(ASTVariable, name);
}

#define CUSTOM_FIELDS     data_t *generator; data_t *iter;
AST_NODE_TYPE_DEF(Generator, Expr)
#undef CUSTOM_FIELDS

static inline ast_Generator_t * ast_Generator_create(data_t *generator) {
  ast_init();
  return (ast_Generator_t *) data_create(ASTGenerator, generator);
}

#define CUSTOM_FIELDS ast_Expr_t  *function; arguments_t *args;
AST_NODE_TYPE_DEF(Call, Expr)
#undef CUSTOM_FIELDS

static inline ast_Call_t * ast_Call_create(ast_Expr_t *function) {
  ast_init();
  return (ast_Call_t *) data_create(ASTCall, function);
}

/* ----------------------------------------------------------------------- */

#define CUSTOM_FIELDS
AST_NODE_TYPE_DEF(Statement, Node)
#undef CUSTOM_FIELDS
static inline ast_Statement_t * ast_Statement_create() {
  return (ast_Statement_t *) data_create(ASTStatement);
}

#define CUSTOM_FIELDS datalist_t *statements;
AST_NODE_TYPE_DEF(Block, Expr)
#undef CUSTOM_FIELDS

static inline ast_Block_t * ast_Block_create() {
  ast_init();
  return (ast_Block_t *) data_create(ASTBlock);
}

static inline ast_Block_t * ast_Block_append(data_t *b, ast_Statement_t *stmt) {
  ast_Block_t *block = data_as_ast_Block(b);
  datalist_push(block -> statements, data_copy(stmt));
  return block;
}

#define CUSTOM_FIELDS ast_Expr_t *condition; ast_Expr_t *block;
AST_NODE_TYPE_DEF(Loop, Expr)
#undef CUSTOM_FIELDS

static inline ast_Loop_t * ast_Loop_create(ast_Expr_t *condition, ast_Expr_t *block) {
  ast_init();
  return (ast_Loop_t *) data_create(ASTLoop, condition, block);
}

#define CUSTOM_FIELDS name_t *name;
AST_NODE_TYPE_DEF(Script, Block)
#undef CUSTOM_FIELDS

ast_Script_t *ast_Script_create(char *name) {
  ast_init();
  return (ast_Script_t *) data_create(ASTScript, name);
}

#define CUSTOM_FIELDS name_t *name; ast_Expr_t *value;
AST_NODE_TYPE_DEF(Assignment, Expr)
#undef CUSTOM_FIELDS

static inline ast_Assignment_t * ast_Assignment_create(name_t *name, ast_Expr_t *value) {
  ast_init();
  return (ast_Assignment_t *) data_create(ASTAssignment, name, value);
}

#define CUSTOM_FIELDS ast_Expr_t *expr; struct _ast_If *elif_block;
AST_NODE_TYPE_DEF(If, Block)
#undef CUSTOM_FIELDS

static inline ast_If_t * ast_If_create() {
  return (ast_If_t *) data_create(ASTIf);
}

#define CUSTOM_FIELDS
AST_NODE_TYPE_DEF(Pass, Statement)
#undef CUSTOM_FIELDS

static inline ast_Pass_t * ast_Pass_create() {
  return (ast_Pass_t *) data_create(ASTPass);
}

#define CUSTOM_FIELDS ast_Expr_t *expr;
AST_NODE_TYPE_DEF(While, Block)
#undef CUSTOM_FIELDS

static inline ast_While_t * ast_While_create() {
  return (ast_While_t *) data_create(ASTWhile);
}

typedef struct _ast_builder {
  data_t           _d;
  ast_Script_t    *script;
  ast_Node_t      *current_node;
} ast_builder_t;

OBLAST_IMPEXP int ASTBuilder;
type_skel(ast_builder, ASTBuilder, ast_builder_t);

static inline ast_builder_t *ast_builder_create(char *name) {
  ast_init();
  return (ast_builder_t *) data_create(ASTBuilder, name);
}

/* ----------------------------------------------------------------------- */

OBLAST_IMPEXP data_t *         ast_execute(void *, data_t *);
OBLAST_IMPEXP ast_Node_t *     ast_append(ast_Node_t *, ast_Node_t *);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __AST_H__ */

#pragma clang diagnostic pop