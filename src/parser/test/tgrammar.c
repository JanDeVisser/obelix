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

START_TEST(test_grammar_create)
  grammar_t *g;

  g = grammar_create();
  grammar_free(g);
END_TEST

int main(void){
  int number_failed;
  Suite *s;
  SRunner *sr;
  TCase *tc;
  int ix;

  s = suite_create("Grammar");
  tc = tcase_create("Data");

  tcase_add_test(tc, test_grammar_create);
  //tcase_add_test(tc, test_nonterminal_create);
  //tcase_add_test(tc, test_rule_create);
  //tcase_add_test(tc, test_terminal_create);

  sr = srunner_create(s);

  srunner_run_all(sr, CK_VERBOSE);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

