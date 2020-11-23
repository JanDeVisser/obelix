/*
 * obelix/src/lexer/test/tlexer.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of Obelix.
 *
 * Obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <range.h>
#include <file.h>
#include <grammarparser.h>
#include <parser.h>
#include "tast.h"

/* ----------------------------------------------------------------------- */

int tast_debug = 0;

/* ----------------------------------------------------------------------- */

START_TEST(test_ast_const_create)
  ast_const_t *expr;

  expr = ast_const_create(int_to_data(2));
  ck_assert(expr);
  ck_assert_int_eq(data_type(expr), ASTConst);
  ck_assert_int_eq(data_type(expr -> value), Int);
  ck_assert_int_eq(data_intval(expr -> value), 2);
  debug(tast, "%s", data_tostring(expr));
END_TEST

START_TEST(test_ast_infix_create)
  ast_infix_t *expr;
  ast_expr_t *left;
  ast_expr_t *right;
  data_t     *op;

  left = data_as_ast_expr(ast_const_create(int_to_data(3)));
  op = (data_t *) str_to_data("+");
  right = data_as_ast_expr(ast_const_create(int_to_data(2)));
  expr = ast_infix_create(left, op, right);
  ck_assert(expr);
  debug(tast, "%s", data_tostring(expr));
END_TEST

START_TEST(test_ast_prefix_create)
  ast_prefix_t *expr;
  ast_expr_t   *operand;
  data_t       *op;

  op = (data_t *) str_to_data("-");
  operand = data_as_ast_expr(ast_const_create(int_to_data(2)));
  expr = ast_prefix_create(op, operand);
  ck_assert(expr);
  debug(tast, "%s", data_tostring(expr));
END_TEST

START_TEST(test_ast_ternary_create)
  ast_expr_t    *condition;
  ast_expr_t    *true_value;
  ast_expr_t    *false_value;
  name_t        *name;
  ast_ternary_t *expr;

  name = name_create(1, "x");
  condition = (ast_expr_t *) ast_infix_create(
    (ast_expr_t *) ast_variable_create(name),
    str_to_data("=="),
    (ast_expr_t  *) ast_const_create(int_to_data(3)));
  name_free(name);
  true_value = ast_const_create(str_to_data("TRUE"));
  false_value = ast_const_create(str_to_data("FALSE"));
  expr = ast_ternary_create(condition, true_value, false_value);
  ck_assert(expr);
  ast_expr_free(condition);
  ast_expr_free(true_value);
  ast_expr_free(false_value);

  debug(tast, "%s", data_tostring(expr));
END_TEST

START_TEST(test_ast_variable_create)
  ast_variable_t *expr;
  name_t         *name;

  name = name_create(1, "x");
  expr = ast_variable_create(name);
  ck_assert(expr);
  debug(tast, "%s", data_tostring(expr));
END_TEST

START_TEST(test_ast_const_call)
  data_t *expr;
  data_t *ret;

  expr = (data_t *) ast_const_create(int_to_data(2));
  ck_assert(expr);
  debug(tast, "%s", data_tostring(expr));
  ret = ast_execute(expr, data_null());
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_int_eq(data_intval(ret), 2);
END_TEST

START_TEST(test_ast_infix_call)
  ast_infix_t *expr;
  ast_expr_t *left;
  ast_expr_t *right;
  data_t     *op;
  data_t     *ret;

  left = data_as_ast_expr(ast_const_create(int_to_data(3)));
  op = (data_t *) str_to_data("+");
  right = data_as_ast_expr(ast_const_create(int_to_data(2)));
  expr = ast_infix_create(left, op, right);
  ck_assert(expr);
  debug(tast, "%s", data_tostring(expr));
  ret = ast_execute(expr, data_null());
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_eq(data_type(ret), Int);
  ck_assert_int_eq(data_intval(ret), 5);
END_TEST

START_TEST(test_ast_prefix_call)
  ast_prefix_t *expr;
  ast_expr_t   *operand;
  data_t       *op;
  data_t       *ret;

  op = (data_t *) str_to_data("-");
  operand = data_as_ast_expr(ast_const_create(int_to_data(2)));
  expr = ast_prefix_create(op, operand);
  ck_assert(expr);
  debug(tast, "%s", data_tostring(expr));
  ret = ast_execute(expr, data_null());
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_eq(data_type(ret), Int);
  ck_assert_int_eq(data_intval(ret), -2);
END_TEST

START_TEST(test_ast_ternary_call)
  ast_expr_t    *condition;
  ast_expr_t    *true_value;
  ast_expr_t    *false_value;
  name_t        *name;
  ast_ternary_t *expr;
  dictionary_t  *ctx;
  data_t        *ret;

  name = name_create(1, "x");
  condition = (ast_expr_t *) ast_infix_create(
    (ast_expr_t *) ast_variable_create(name),
    str_to_data("=="),
    (ast_expr_t  *) ast_const_create(int_to_data(3)));
  name_free(name);
  true_value = ast_const_create(str_to_data("TRUE"));
  false_value = ast_const_create(str_to_data("FALSE"));
  expr = ast_ternary_create(condition, true_value, false_value);
  ck_assert(expr);
  ast_expr_free(condition);
  ast_expr_free(true_value);
  ast_expr_free(false_value);

  debug(tast, "%s", data_tostring(expr));

  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(3));
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_str_eq(data_tostring(ret), "TRUE");
END_TEST

START_TEST(test_ast_variable_call)
  data_t       *expr;
  name_t       *name;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(1, "x");
  expr = (data_t *) ast_variable_create(name);
  ck_assert(expr);
  name_free(name);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(2));
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_int_eq(data_intval(ret), 2);
END_TEST

START_TEST(test_ast_variable_doesnt_exist_call)
  data_t       *expr;
  name_t       *name;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(1, "x");
  expr = (data_t *) ast_variable_create(name);
  ck_assert(expr);
  name_free(name);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "y", int_to_data(2));
  ret = ast_execute(expr, ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_eq(data_type(ret), Exception);
END_TEST

START_TEST(test_ast_tree_call)
  ast_expr_t   *var;
  ast_expr_t   *two;
  ast_expr_t   *three;
  ast_expr_t   *infix_1;
  ast_expr_t   *infix_2;

  name_t       *name;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(1, "x");
  var = (ast_expr_t *) ast_variable_create(name);
  name_free(name);

  two = (ast_expr_t *) ast_const_create(int_to_data(2));
  infix_1 = (ast_expr_t *) ast_infix_create(var, (data_t *) str_to_data("*"), two);

  three = (ast_expr_t *) ast_const_create(int_to_data(3));
  infix_2 = (ast_expr_t *) ast_infix_create(infix_1, (data_t *) str_to_data("+"), three);

  debug(tast, "%s", data_tostring(infix_2));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(4));
  ret = ast_execute(infix_2, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_eq(data_type(ret), Int);
  ck_assert_int_eq(data_intval(ret), 11);
END_TEST

START_TEST(test_ast_call_call)
  ast_call_t   *expr;
  name_t       *name;
  ast_expr_t   *self;
  ast_expr_t   *arg;
  data_t       *ret;
  dictionary_t *ctx;

  name = name_create(2, "x", "sum");
  self = data_as_ast_expr(ast_variable_create(name));
  arg = data_as_ast_expr(ast_const_create(int_to_data(2)));
  expr = ast_call_create(self);
  ck_assert(expr);
  expr -> args = arguments_create_args(1, arg);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(3));
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_int_eq(data_intval(ret), 5);
END_TEST

START_TEST(test_ast_assignment_call)
  ast_assignment_t *expr;
  name_t           *name;
  ast_expr_t       *value;
  dictionary_t     *ctx;
  data_t           *ret;

  value = (ast_expr_t *) ast_const_create(int_to_data(2));
  name = name_create(1, "x");
  expr = ast_assignment_create(name, value);
  ck_assert(expr);
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_int_eq(data_intval(dictionary_get(ctx, "x")), 2);
END_TEST

START_TEST(test_ast_block_call)
  ast_block_t      *expr;
  ast_assignment_t *ass;
  name_t           *name;
  ast_expr_t       *value;
  ast_expr_t       *arg;
  data_t           *ret;
  dictionary_t     *ctx;
  datalist_t       *statements;

  statements = datalist_create(NULL);
  value = (ast_expr_t *) ast_const_create(int_to_data(2));
  name = name_create(1, "x");
  ass = ast_assignment_create(name, value);
  datalist_push(statements, ass);
  ast_assignment_free(ass);
  value = (ast_expr_t *) ast_const_create(int_to_data(3));
  name = name_create(1, "y");
  ass = ast_assignment_create(name, value);
  datalist_push(statements, ass);
  ast_assignment_free(ass);
  expr = ast_block_create();
  debug(tast, "%s", data_tostring(expr));
  expr -> statements = statements;
  debug(tast, "%s", data_tostring(expr));
  ctx = dictionary_create(NULL);
  ret = ast_execute(expr, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_int_eq(data_intval(dictionary_get(ctx, "x")), 2);
  ck_assert_int_eq(data_intval(dictionary_get(ctx, "y")), 3);
END_TEST

START_TEST(test_ast_loop_call)
  ast_infix_t      *cond;
  ast_infix_t      *inc;
  ast_loop_t       *loop;
  ast_assignment_t *ass_x;
  ast_assignment_t *ass_y;
  ast_block_t      *block;
  datalist_t       *statements;
  name_t           *x;
  name_t           *y;
  ast_expr_t       *value;
  ast_expr_t       *arg;
  data_t           *ret;
  dictionary_t     *ctx;

  x = name_create(1, "x");
  y = name_create(1, "y");
  cond = ast_infix_create((ast_expr_t *) ast_variable_create(x),
                          (data_t *) str_copy_chars("<"),
                          (ast_expr_t *) ast_const_create(int_to_data(10)));

  block = ast_block_create();
  statements = datalist_create(NULL);
  ass_y = ast_assignment_create(y, (ast_expr_t *) ast_variable_create(x));
  datalist_push(statements, ass_y);
  inc = ast_infix_create((ast_expr_t *) ast_variable_create(x),
                          (data_t *) str_copy_chars("+"),
                          (ast_expr_t *) ast_const_create(int_to_data(1)));
  ass_x = ast_assignment_create(x, (ast_expr_t *) inc);
  datalist_push(statements, ass_x);
  block -> statements = statements;
  loop = ast_loop_create((ast_expr_t *) cond, (ast_expr_t *) block);
  debug(tast, "%s", data_tostring(loop));
  ctx = dictionary_create(NULL);
  dictionary_set(ctx, "x", int_to_data(0));
  ret = ast_execute(loop, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_int_eq(data_intval(dictionary_get(ctx, "x")), 10);
  ck_assert_int_eq(data_intval(dictionary_get(ctx, "y")), 9);
END_TEST

START_TEST(test_ast_generator_loop)
  ast_generator_t  *gen;
  ast_infix_t      *cond;
  ast_infix_t      *inc;
  ast_loop_t       *loop;
  ast_assignment_t *ass_x;
  ast_assignment_t *ass_y;
  name_t           *x;
  name_t           *y;
  data_t           *ret;
  dictionary_t     *ctx;

  x = name_create(1, "x");
  y = name_create(1, "y");

  gen = ast_generator_create(range_create(int_to_data(0), int_to_data(10)));
  ass_x = ast_assignment_create(x, (ast_expr_t *) gen);

  ass_y = ast_assignment_create(y, (ast_expr_t *) ast_variable_create(x));
  loop = ast_loop_create((ast_expr_t *) ass_x, (ast_expr_t *) ass_y);
  debug(tast, "%s", data_tostring(loop));
  ctx = dictionary_create(NULL);
  ret = ast_execute(loop, (data_t *) ctx);
  debug(tast, "Call result: %s", data_tostring(ret));
  ck_assert_int_ne(data_type(ret), Exception);
  ck_assert_int_eq(data_intval(dictionary_get(ctx, "x")), 9);
  ck_assert_int_eq(data_intval(dictionary_get(ctx, "y")), 9);
END_TEST

/* ----------------------------------------------------------------------- */

static file_t           *file = NULL;
static grammar_parser_t *gp = NULL;
static grammar_t        *grammar = NULL;
static parser_t         *parser = NULL;

static data_t           *result = NULL;

static void create_parser() {
  char *grammar_path = "./ast.grammar";

  file = file_open(grammar_path);
  ck_assert(file_isopen(file));
  gp = grammar_parser_create((data_t *) file);
  ck_assert(gp);
  grammar = grammar_parser_parse(gp);
  ck_assert(grammar);
  parser = parser_create(gp -> grammar);
  ck_assert(parser);
}

static data_t * evaluate(char *str, int expected) {
  str_t  *text = str_copy_chars(str);
  data_t *ret;

  create_parser();
  // grammar_dump(grammar);
  ret = parser_parse(parser, (data_t *) text);
  str_free(text);
  if (ret) {
    error("parser_parse: %s", data_tostring(ret));
  }
  ck_assert_ptr_eq(ret, NULL);
  ck_assert(result);
  ck_assert_int_eq(expected, data_intval(result));
  return result;
}

static void _teardown(void) {
  data_free(result);
  parser_free(parser);
  grammar_free(grammar);
  grammar_parser_free(gp);
  file_free(file);
}

__DLL_EXPORT__ parser_t * ast_make_prefix(parser_t *parser) {
  ast_expr_t   *value;
  token_t      *sign;
  ast_prefix_t *expr;

  value = (ast_expr_t *) datastack_pop(parser -> stack);
  sign = (token_t *) datastack_pop(parser -> stack);
  expr = ast_prefix_create((data_t *) sign, value);
  debug(tast, "expr: %s", ast_prefix_tostring(expr));
  datastack_push(parser -> stack, expr);
  return parser;
}

__DLL_EXPORT__ parser_t * ast_make_infix(parser_t *parser) {
  ast_expr_t  *left;
  ast_expr_t  *right;
  token_t     *op;
  ast_infix_t *expr;

  right = (ast_expr_t *) datastack_pop(parser -> stack);
  op = (token_t *) datastack_pop(parser -> stack);
  left = (ast_expr_t *) datastack_pop(parser -> stack);
  expr = ast_infix_create(left, (data_t *) op, right);
  debug(tast, "expr: %s", ast_infix_tostring(expr));
  datastack_push(parser -> stack, expr);
  return parser;
}

__DLL_EXPORT__ parser_t * ast_make_const(parser_t *parser) {
  data_t      *number = token_todata(parser -> last_token);
  ast_const_t *expr = ast_const_create(number);

  return parser_pushval(parser, (data_t *) expr);
}

__DLL_EXPORT__ parser_t * ast_make_variable(parser_t *parser) {
  data_t         *name = token_todata(parser -> last_token);
  name_t         *n = name_create(1, data_tostring(name));
  ast_variable_t *expr = ast_variable_create(n);

  data_free(name);
  name_free(n);
  return parser_pushval(parser, (data_t *) expr);
}

__DLL_EXPORT__ parser_t * assign_result(parser_t *parser) {
  ast_expr_t   *expr = (ast_expr_t *) datastack_pop(parser -> stack);
  dictionary_t *ctx = dictionary_create(NULL);

  dictionary_set(ctx, "x", int_to_data(12));
  result = ast_execute(expr, ctx);
  return parser;
}

/* ----------------------------------------------------------------------- */

START_TEST(ast_parser_create)
  create_parser();
END_TEST

START_TEST(ast_parser_parse)
  evaluate("1+1", 2);
END_TEST

START_TEST(ast_parser_stack_order)
  evaluate("1 - 2", -1);
END_TEST

START_TEST(ast_parser_parens)
  evaluate("2 * (3 + 4)", 14);
END_TEST

START_TEST(ast_parser_signed_number)
  evaluate("1 - -2", 3);
END_TEST

START_TEST(ast_parser_two_pairs_of_parens)
  evaluate("(1+2) * (3 + 4)", 21);
END_TEST

START_TEST(ast_parser_nested_parens)
  evaluate("2 * ((3*2) + 4)", 20);
END_TEST

START_TEST(ast_parser_precedence)
  evaluate("2 * (4 + 3*2)", 20);
END_TEST

START_TEST(ast_parser_variable)
  evaluate("2 * (4 + 3*x)", 80);
END_TEST

/* ----------------------------------------------------------------------- */

void ast_expr(void) {
  TCase *tc = tcase_create("ASTExpr");
  tcase_add_test(tc, test_ast_const_create);
  tcase_add_test(tc, test_ast_infix_create);
  tcase_add_test(tc, test_ast_prefix_create);
  tcase_add_test(tc, test_ast_variable_create);
  tcase_add_test(tc, test_ast_ternary_create);
  tcase_add_test(tc, test_ast_const_call);
  tcase_add_test(tc, test_ast_infix_call);
  tcase_add_test(tc, test_ast_prefix_call);
  tcase_add_test(tc, test_ast_variable_call);
  tcase_add_test(tc, test_ast_variable_doesnt_exist_call);
  tcase_add_test(tc, test_ast_tree_call);
  tcase_add_test(tc, test_ast_ternary_call);
  tcase_add_test(tc, test_ast_call_call);
  tcase_add_test(tc, test_ast_assignment_call);
  tcase_add_test(tc, test_ast_block_call);
  tcase_add_test(tc, test_ast_loop_call);
  tcase_add_test(tc, test_ast_generator_loop);
  add_tcase(tc);
}

void ast_parser(void) {
  TCase *tc = tcase_create("Parser");
  tcase_add_checked_fixture(tc, NULL, _teardown);
  tcase_add_test(tc, ast_parser_create);
  tcase_add_test(tc, ast_parser_parse);
  tcase_add_test(tc, ast_parser_stack_order);
  tcase_add_test(tc, ast_parser_parens);
  tcase_add_test(tc, ast_parser_signed_number);
  tcase_add_test(tc, ast_parser_two_pairs_of_parens);
  tcase_add_test(tc, ast_parser_nested_parens);
  tcase_add_test(tc, ast_parser_precedence);
  tcase_add_test(tc, ast_parser_variable);
  add_tcase(tc);
}

extern void init_suite(int argc, char **argv) {
  logging_register_module(tast);
  logging_set_level("DEBUG");
  logging_enable("ast");
  logging_enable("tast");
  ast_expr();
  ast_parser();
}
