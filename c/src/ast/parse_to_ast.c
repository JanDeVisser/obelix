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
#include <range.h>
#include <file.h>
#include <grammarparser.h>
#include <parser.h>


extern parser_t * make_block(parser_t *parser) {
  ast_Block_t *block = ast_Block_create((char *) "main");
  datastack_push(parser->stack, block);
  parser->data = ast_Block_copy(block);
  return parser;
}

extern parser_t * add_statement(parser_t *parser) {
  ast_Expr_t  *statement = data_as_ast_Expr(datastack_pop(parser->stack));
  ast_Block_t *block = data_as_ast_Block(datastack_pop(parser->stack));

  fflush(stderr);
  ast_Block_add_statement(block, statement);
  datastack_push(parser->stack, block);
  return parser;
}

/* -- P A S S  ( N O P )  S T A T E M E N T ------------------------------ */

extern parser_t *make_pass(parser_t *parser) {
  ast_Pass_t *pass = ast_Pass_create();

  return parser_pushval(parser, (data_t *) pass);
}

/* -- I F  S T A T E M E N T --------------------------------------------- */

extern parser_t * make_ternary(parser_t *parser) {
  ast_Expr_t    *condition = data_as_ast_Expr(datastack_pop(parser->stack));
  ast_Block_t   *block = ast_Block_create((char *) "if (true)");
  ast_Ternary_t *if_stmt = ast_Ternary_create(condition, data_as_ast_Expr(block), NULL);
  datastack_push(parser->stack, if_stmt);
  datastack_push(parser->stack, block);
  return parser;
}

extern parser_t * make_elif_ternary(parser_t *parser) {
  ast_Expr_t    *condition = data_as_ast_Expr(datastack_pop(parser->stack));
  ast_Ternary_t *if_stmt = data_as_ast_Ternary(datastack_pop(parser->stack));
  ast_Block_t   *true_block = ast_Block_create((char *) "if (true)");
  ast_Ternary_t *elif = ast_Ternary_create(condition, data_as_ast_Expr(true_block), NULL);

  datastack_push(parser->stack, if_stmt);
  datastack_push(parser->stack, elif);
  datastack_push(parser->stack, true_block);
  return parser;
}

extern parser_t * set_false_block(parser_t *parser) {
  ast_Ternary_t *if_stmt = data_as_ast_Ternary(datastack_pop(parser->stack));
  ast_Block_t   *block = ast_Block_create((char *) "if (false)");
  if_stmt->false_value = data_as_ast_Expr(block);

  datastack_push(parser->stack, if_stmt);
  datastack_push(parser->stack, block);
  return parser;
}

extern parser_t * set_empty_false_block(parser_t *parser) {
  ast_Ternary_t *if_stmt = data_as_ast_Ternary(datastack_pop(parser->stack));
  ast_Block_t   *block = ast_Block_create((char *) "if (false)(empty)");
  if_stmt->false_value = data_as_ast_Expr(block);
  datastack_push(parser->stack, if_stmt);
  return parser;
}

/* -- W H I L E  S T A T E M E N T --------------------------------------- */

extern parser_t * make_loop(parser_t *parser) {
  char        *name;
  ast_Expr_t  *condition;
  ast_Block_t *block;
  ast_Loop_t  *while_stmt;

  condition = data_as_ast_Expr(datastack_pop(parser->stack));
  asprintf(&name, "while (%s)", ast_Expr_tostring(condition));
  block = ast_Block_create((char *) "while (%s");
  free(name);
  while_stmt = ast_Loop_create(condition, data_as_ast_Expr(block));
  datastack_push(parser->stack, while_stmt);
  datastack_push(parser->stack, block);
  return parser;
}

/* -- F O R  S T A T E M E N T ------------------------------------------- */

extern parser_t * make_for(parser_t *parser) {
  char             *name;
  data_t           *identifier;
  name_t           *id;
  ast_Expr_t       *generator;
  ast_Block_t      *block;
  ast_Loop_t       *for_stmt;
  ast_Assignment_t *assignment;

  generator = data_as_ast_Expr(datastack_pop(parser->stack));
  identifier = datastack_pop(parser->stack);
  if (data_is_ast_Const(identifier)) {
    id = name_create(1, data_tostring(data_as_ast_Const(identifier)->value));
  } else {
    id = name_create(1, data_tostring(identifier));
  }
  asprintf(&name, "for %s in %s", name_tostring(id), ast_Expr_tostring(generator));
  block = ast_Block_create((char *) name);
  free(name);
  assignment = ast_Assignment_create(id, generator);
  for_stmt = ast_Loop_create(data_as_ast_Expr(assignment), data_as_ast_Expr(block));
  name_free(id);
  data_free(identifier);
  datastack_push(parser->stack, for_stmt);
  datastack_push(parser->stack, block);
  return parser;
}

extern parser_t * make_assignment(parser_t *parser) {
  ast_Expr_t       *expr = data_as_ast_Expr(datastack_pop(parser->stack));
  ast_Const_t      *name_expr = data_as_ast_Const(datastack_pop(parser->stack));
  data_t           *name = name_expr->value;
  ast_Assignment_t *assign_stmt = ast_Assignment_create(name_create(1, data_tostring(name)), expr);
  datastack_push(parser->stack, assign_stmt);
  return parser;
}

extern parser_t * make_prefix(parser_t *parser) {
  ast_Expr_t *value;
  token_t *sign;
  ast_Prefix_t *expr;

  value = (ast_Expr_t *) datastack_pop(parser->stack);
  sign = (token_t *) datastack_pop(parser->stack);
  expr = ast_Prefix_create((data_t *) sign, value);
  debug(ast, "expr: %s", ast_Prefix_tostring(expr));
  datastack_push(parser->stack, expr);
  return parser;
}

extern parser_t * make_infix(parser_t *parser) {
  ast_Expr_t *left;
  ast_Expr_t *right;
  token_t *op;
  ast_Infix_t *expr;

  right = (ast_Expr_t *) datastack_pop(parser->stack);
  op = (token_t *) datastack_pop(parser->stack);
  left = (ast_Expr_t *) datastack_pop(parser->stack);
  expr = ast_Infix_create(left, (data_t *) op, right);
  debug(ast, "expr: %s", ast_Infix_tostring(expr));
  datastack_push(parser->stack, expr);
  return parser;
}

extern parser_t * make_const(parser_t *parser) {
  data_t *value = token_todata(parser->last_token);
  ast_Const_t *expr = ast_Const_create(value);

  return parser_pushval(parser, (data_t *) expr);
}

extern parser_t * make_variable(parser_t *parser) {
  data_t *name = token_todata(parser->last_token);
  name_t *n = name_create(1, data_tostring(name));
  ast_Variable_t *expr = ast_Variable_create(n);

  data_free(name);
  name_free(n);
  return parser_pushval(parser, (data_t *) expr);
}

/* -- L I S T  E X P R E S S I O N --------------------------------------- */

extern parser_t * start_list(parser_t *parser) {
  datalist_t *list = datalist_create(NULL);
  return parser_pushval(parser, data_as_data(list));
}

extern parser_t * add_to_list(parser_t *parser) {
  ast_Expr_t *entry = data_as_ast_Expr(datastack_pop(parser->stack));
  datalist_t *list = data_as_list(datastack_pop(parser->stack));
  datalist_push(list, entry);
  ast_Expr_free(entry);
  return parser_pushval(parser, data_as_data(list));
}

extern parser_t * make_generator(parser_t *parser) {
  datalist_t *list = data_as_list(datastack_pop(parser->stack));
  ast_Generator_t *generator = ast_Generator_create(data_as_data(list));
  datalist_free(list);
  return parser_pushval(parser, data_as_data(generator));
}
