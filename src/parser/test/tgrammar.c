/*
 * /obelix/test/tdata.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <check.h>
#include <math.h>
#include <stdarg.h>

#include <grammar.h>
#include <testsuite.h>

static void     _init_grammar(void) __attribute__((constructor(300)));

START_TEST(test_grammar_create)
  grammar_t *g;

  g = grammar_create();
  grammar_free(g);
END_TEST

void _init_lexer(void) {
  TCase *tc = tcase_create("Grammar");

  tcase_add_test(tc, test_grammar_create);
  add_tcase(tc);
}
