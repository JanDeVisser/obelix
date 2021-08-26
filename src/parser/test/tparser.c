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

#include "tparser.h"
#include <file.h>
#include <grammar.h>
#include <grammarparser.h>

/* ----------------------------------------------------------------------- */

static file_t           *file = NULL;
static grammar_parser_t *gp = NULL;
static grammar_t        *grammar = NULL;
static parser_t         *parser = NULL;

static data_t           *result = NULL;

static void _create_parser(char *grammarfile) {
  char             *grammar_path;

  asprintf(&grammar_path, "../share/grammar/%s.grammar", grammarfile);
  file = file_open(grammar_path);
  ck_assert(file_isopen(file));
  gp = grammar_parser_create((data_t *) file);
  ck_assert(gp);
  grammar = grammar_parser_parse(gp);
  ck_assert(grammar);
  parser = parser_create(gp -> grammar);
  ck_assert(parser);
}

static void _teardown(void) {
  data_free(result);
  parser_free(parser);
//  grammar_free(grammar);
  grammar_parser_free(gp);
  file_free(file);
}

/* ----------------------------------------------------------------------- */

extern parser_t * expr_assign_result(parser_t *parser) {
  data_t  *value = datastack_pop(parser -> stack);
  token_t *sign = (token_t *) datastack_pop(parser -> stack);

  result = data_execute(value, token_token(sign), NULL);
  return parser;
}

extern parser_t * expr_call_op(parser_t *parser) {
  data_t      *v1, *v2, *res, *signed1;
  token_t     *s1, *s2, *op;
  arguments_t *args;

  v2 = datastack_pop(parser -> stack);
  s2 = (token_t *) datastack_pop(parser -> stack);
  args = arguments_create_args(1, data_execute(v2, token_token(s2), NULL));
  op = (token_t *) datastack_pop(parser -> stack);
  v1 = datastack_pop(parser -> stack);
  s1 = (token_t *) datastack_pop(parser -> stack);
  signed1 = data_execute(v1, token_token(s1), NULL);
  res = data_execute(signed1, token_token(op), args);
  datastack_push(parser -> stack, data_uncopy((data_t *) token_create(TokenCodePlus, "+")));
  datastack_push(parser -> stack, res);
  arguments_free(args);
  token_free(op);
  data_free(v2);
  data_free(v1);
  data_free(signed1);
  return parser;
}

data_t * evaluate(char *str) {
  str_t  *text = str(str);
  data_t *ret;

  _create_parser("expr");
  // grammar_dump(grammar);
  ret = parser_parse(parser, (data_t *) text);
  str_free(text);
  if (ret) {
    error("parser_parse: %s", data_tostring(ret));
  }
  ck_assert_ptr_eq(ret, NULL);
  ck_assert(result);
  return result;
}

/* ----------------------------------------------------------------------- */

START_TEST(test_parser_create)
  _create_parser("expr");
END_TEST

START_TEST(test_parser_parse)
  evaluate("1+1");
  ck_assert_int_eq(data_intval(result), 2);
END_TEST

START_TEST(test_parser_stack_order)
  evaluate("1 - 2");
  ck_assert_int_eq(data_intval(result), -1);
END_TEST

START_TEST(test_parser_parens)
  evaluate("2 * (3 + 4)");
  ck_assert_int_eq(data_intval(result), 14);
END_TEST

START_TEST(test_parser_signed_number)
  evaluate("1 - -2");
  ck_assert_int_eq(data_intval(result), 3);
END_TEST

START_TEST(test_parser_two_pairs_of_parens)
  evaluate("(1+2) * (3 + 4)");
  ck_assert_int_eq(data_intval(result), 21);
END_TEST

START_TEST(test_parser_nested_parens)
  evaluate("2 * ((3*2) + 4)");
  ck_assert_int_eq(data_intval(result), 20);
END_TEST

START_TEST(test_parser_precedence)
  evaluate("2 * (4 + 3*2)");
  ck_assert_int_eq(data_intval(result), 20);
END_TEST

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

void create_parser(void) {
  TCase *tc = tcase_create("Parser");
  tcase_add_checked_fixture(tc, NULL, _teardown);
  tcase_add_test(tc, test_parser_create);
  tcase_add_test(tc, test_parser_parse);
  tcase_add_test(tc, test_parser_stack_order);
  tcase_add_test(tc, test_parser_parens);
  tcase_add_test(tc, test_parser_signed_number);
  tcase_add_test(tc, test_parser_two_pairs_of_parens);
  tcase_add_test(tc, test_parser_nested_parens);
  tcase_add_test(tc, test_parser_precedence);
  add_tcase(tc);
}

extern void init_suite(int argc, char **argv) {
  create_parser();
}
