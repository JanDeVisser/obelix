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

typedef struct _ast_node {
  data_t            _d;
  struct _ast_node *parent;
  datalist_t       *children;
} ast_node_t;
OBLAST_IMPEXP int ASTNode;
type_skel(ast_node, ASTNode, ast_node_t);

/* ----------------------------------------------------------------------- */

typedef struct _ast_expr {
  ast_node_t  node;
} ast_expr_t;
OBLAST_IMPEXP int ASTExpr;
type_skel(ast_expr, ASTExpr, ast_expr_t);

typedef struct _ast_const {
  ast_expr_t  expr;
  data_t     *value;
} ast_const_t;

OBLAST_IMPEXP int ASTConst;
type_skel(ast_const, ASTConst, ast_const_t);

static inline ast_const_t * ast_const_create(data_t *value) {
  ast_init();
  return (ast_const_t *) data_create(ASTConst, value);
}

typedef struct _ast_infix {
  ast_expr_t  expr;
  data_t     *op;
  ast_expr_t *left;
  ast_expr_t *right;
} ast_infix_t;

OBLAST_IMPEXP int ASTInfix;
type_skel(ast_infix, ASTInfix, ast_infix_t);

static inline ast_infix_t * ast_infix_create(ast_expr_t *left, data_t *op, ast_expr_t *right) {
  ast_init();
  return (ast_infix_t *) data_create(ASTInfix, left, op, right);
}

typedef struct _ast_prefix {
  ast_expr_t  expr;
  data_t     *op;
  ast_expr_t *operand;
} ast_prefix_t;

OBLAST_IMPEXP int ASTPrefix;
type_skel(ast_prefix, ASTPrefix, ast_prefix_t);

static inline ast_prefix_t * ast_prefix_create(data_t *op, ast_expr_t *operand) {
  ast_init();
  return (ast_prefix_t *) data_create(ASTPrefix, op, operand);
}

typedef struct _ast_ternary {
  ast_expr_t  expr;
  ast_expr_t *condition;
  ast_expr_t *true_value;
  ast_expr_t *false_value;
} ast_ternary_t;

OBLAST_IMPEXP int ASTTernary;
type_skel(ast_ternary, ASTTernary, ast_ternary_t);

static inline ast_ternary_t * ast_ternary_create(ast_expr_t *condition,
                                                 ast_expr_t *true_value,
                                                 ast_expr_t *false_value) {
  ast_init();
  return (ast_ternary_t *) data_create(ASTTernary, condition, true_value, false_value);
}

typedef struct _ast_variable {
  ast_expr_t  expr;
  name_t     *name;
} ast_variable_t;

OBLAST_IMPEXP int ASTVariable;
type_skel(ast_variable, ASTVariable, ast_variable_t);

static inline ast_variable_t * ast_variable_create(name_t *name) {
  ast_init();
  return (ast_variable_t *) data_create(ASTVariable, name);
}

typedef struct _ast_generator {
  ast_expr_t  expr;
  data_t     *generator;
  data_t     *iter;
} ast_generator_t;

OBLAST_IMPEXP int ASTGenerator;
type_skel(ast_generator, ASTGenerator, ast_generator_t);

static inline ast_generator_t * ast_generator_create(data_t *generator) {
  ast_init();
  return (ast_generator_t *) data_create(ASTGenerator, generator);
}

typedef struct _ast_call {
  ast_expr_t   expr;
  ast_expr_t  *function;
  arguments_t *args;
} ast_call_t;

OBLAST_IMPEXP int ASTCall;
type_skel(ast_call, ASTCall, ast_call_t);

static inline ast_call_t * ast_call_create(ast_expr_t *function) {
  ast_init();
  return (ast_call_t *) data_create(ASTCall, function);
}

/* ----------------------------------------------------------------------- */

typedef struct _ast_block {
  ast_expr_t  expr;
  datalist_t *statements;
} ast_block_t;

OBLAST_IMPEXP int ASTBlock;
type_skel(ast_block, ASTBlock, ast_block_t);

static inline ast_block_t * ast_block_create() {
  ast_init();
  return (ast_block_t *) data_create(ASTBlock);
}

typedef struct _ast_loop {
  ast_expr_t  expr;
  ast_expr_t *condition;
  ast_expr_t *block;
} ast_loop_t;

OBLAST_IMPEXP int ASTLoop;
type_skel(ast_loop, ASTLoop, ast_loop_t);

static inline ast_loop_t * ast_loop_create(ast_expr_t *condition, ast_expr_t *block) {
  ast_init();
  return (ast_loop_t *) data_create(ASTLoop, condition, block);
}

typedef struct _ast_script {
  ast_block_t  block;
  name_t      *name;
} ast_script_t;

OBLAST_IMPEXP int ASTScript;
type_skel(ast_script, ASTScript, ast_script_t)

ast_script_t *ast_script_create(char *name) {
  ast_init();
  return (ast_script_t *) data_create(ASTScript, name);
}

typedef struct _ast_statement {
  ast_node_t   node;
} ast_statement_t;

OBLAST_IMPEXP int ASTStatement;
type_skel(ast_statement, ASTStatement, ast_statement_t)

static inline ast_statement_t * ast_statement_create() {
  return (ast_statement_t *) data_create(ASTStatement);
}

typedef struct _ast_assignment {
  ast_expr_t  expr;
  name_t     *name;
  ast_expr_t *value;
} ast_assignment_t;

OBLAST_IMPEXP int ASTAssignment;
type_skel(ast_assignment, ASTAssignment, ast_assignment_t)

static inline ast_assignment_t  * ast_assignment_create(name_t *name, ast_expr_t *value) {
  ast_init();
  return (ast_assignment_t *) data_create(ASTAssignment, name, value);
}

typedef struct _ast_if {
  ast_block_t     block;
  ast_expr_t     *expr;
  struct _ast_if *elif_block;
} ast_if_t;

OBLAST_IMPEXP int ASTIf;
type_skel(ast_if, ASTIf, ast_if_t)

static inline ast_if_t * ast_if_create() {
  return (ast_if_t *) data_create(ASTIf);
}

typedef struct _ast_pass {
  ast_node_t  node;
} ast_pass_t;

OBLAST_IMPEXP int ASTPass;
type_skel(ast_pass, ASTPass, ast_pass_t)

static inline ast_pass_t * ast_pass_create() {
  return (ast_pass_t *) data_create(ASTPass);
}

typedef struct _ast_while {
  ast_block_t  block;
  ast_expr_t  *expr;
} ast_while_t;

OBLAST_IMPEXP int ASTWhile;
type_skel(ast_while, ASTWhile, ast_if_t)

static inline ast_while_t * ast_while_create() {
  return (ast_while_t *) data_create(ASTWhile);
}

typedef struct _ast_builder {
  data_t           _d;
  ast_script_t    *script;
  ast_node_t      *current_node;
} ast_builder_t;

OBLAST_IMPEXP int ASTBuilder;
type_skel(ast_builder, ASTBuilder, ast_builder_t);

static inline ast_builder_t *ast_builder_create(char *name) {
  ast_init();
  return (ast_builder_t *) data_create(ASTBuilder, name);
}

/* ----------------------------------------------------------------------- */

OBLAST_IMPEXP data_t *         ast_execute(void *, data_t *);
OBLAST_IMPEXP ast_node_t *     ast_append(ast_node_t *, ast_node_t *);

#ifdef  __cplusplus
}
#endif /* __cplusplus */

#endif /* __AST_H__ */
