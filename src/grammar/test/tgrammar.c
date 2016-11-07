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

#include "tgrammar.h"
#include <file.h>

/* ----------------------------------------------------------------------- */

static file_t           *file = NULL;
static grammar_parser_t *gp = NULL;
static grammar_t        *grammar = NULL;

static void _create_grammar_parser(char *grammar) {
  char             *grammar_path;

  asprintf(&grammar_path, "../share/grammar/%s.grammar", grammar);
  file = file_open(grammar_path);
  ck_assert(file_isopen(file));
  gp = grammar_parser_create((data_t *) file);
  ck_assert(gp);
  gp -> dryrun = TRUE;
}

static void _teardown(void) {
  file_free(file);
  grammar_parser_free(gp);
  grammar_free(grammar);
}

/* ----------------------------------------------------------------------- */

START_TEST(test_grammar_parser_create)
  _create_grammar_parser("dummy");
END_TEST

START_TEST(test_grammar_parser_parse)
  _create_grammar_parser("dummy");
  grammar = grammar_parser_parse(gp);
  ck_assert(grammar);
END_TEST

START_TEST(test_grammar_analyze)
  grammar_t *ret;

  _create_grammar_parser("dummy");
  grammar = grammar_parser_parse(gp);
  ck_assert(grammar);
  ret = grammar_analyze(grammar);
  ck_assert(ret);
END_TEST

START_TEST(test_grammar_dump)
  grammar_t *ret;

  _create_grammar_parser("dummy");
  grammar = grammar_parser_parse(gp);
  ck_assert(grammar);
  ret = grammar_analyze(grammar);
  ck_assert(ret);
  ret = grammar_dump(grammar);
  ck_assert(ret);
END_TEST

START_TEST(test_grammar_modifiers)
  grammar_t *ret;

  _create_grammar_parser("modifiers");
  grammar = grammar_parser_parse(gp);
  ck_assert(grammar);
  ret = grammar_analyze(grammar);
  ck_assert(ret);
  ret = grammar_dump(grammar);
  ck_assert(ret);
END_TEST

/* ----------------------------------------------------------------------- */

/* ----------------------------------------------------------------------- */

void create_grammar_parser(void) {
  TCase *tc = tcase_create("Grammar");
  tcase_add_checked_fixture(tc, NULL, _teardown);
//  tcase_add_test(tc, test_grammar_parser_create);
//  tcase_add_test(tc, test_grammar_parser_parse);
//  tcase_add_test(tc, test_grammar_analyze);
//  tcase_add_test(tc, test_grammar_dump);
  tcase_add_test(tc, test_grammar_modifiers);
  add_tcase(tc);
}

extern void init_suite(int argc, char **argv) {
  create_grammar_parser();
}
