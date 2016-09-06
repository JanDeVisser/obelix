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


#include <check.h>
#include <stdio.h>
#include <stdlib.h>

#include <file.h>
#include <lexa.h>
#include <testsuite.h>

static lexa_t * setup(void);
static lexa_t * setup_with_scanners(void);
static void     teardown(lexa_t *);

lexa_t * setup(void) {
  lexa_t *ret;
  
  ret = lexa_create();
  ck_assert_ptr_ne(ret, NULL);
  return ret;
}

lexa_t * setup_with_scanners(void) {
  lexa_t *lexa;

  lexa = setup();
  lexa_add_scanner(lexa, "identifier");
  lexa_add_scanner(lexa, "whitespace");
  return lexa;
}

void teardown(lexa_t *lexa) {
  lexa_free(lexa);
}

START_TEST(test_lexa_create)
  lexa_t *lexa;

  lexa = setup();
  teardown(lexa);
END_TEST

START_TEST(test_lexa_add_scanner)
  lexa_t *lexa;

  lexa = setup_with_scanners();
  ck_assert_int_eq(dict_size(lexa -> scanners), 2);
  teardown(lexa);
END_TEST

START_TEST(test_lexa_build_lexer)
  lexa_t *lexa;

  lexa = setup_with_scanners();
  ck_assert_int_eq(dict_size(lexa -> scanners), 2);
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
  teardown(lexa);
END_TEST

START_TEST(test_lexa_tokenize)
  lexa_t *lexa;

  lexa = setup_with_scanners();
  ck_assert_int_eq(dict_size(lexa -> scanners), 2);
  lexa_build_lexer(lexa);
  ck_assert_ptr_ne(lexa -> config, NULL);
  lexa -> stream = (data_t *) str_copy_chars("Hello World");
  ck_assert_ptr_ne(lexa -> stream, NULL);
  lexa -> fname = strdup("<<string>>");
  lexa_tokenize(lexa);
  teardown(lexa);
END_TEST

extern void init_suite(void) {
  TCase *tc = tcase_create("Lexa");

  /* tcase_add_checked_fixture(tc_core, setup, teardown); */
  tcase_add_test(tc, test_lexa_create);
  tcase_add_test(tc, test_lexa_add_scanner);
  tcase_add_test(tc, test_lexa_build_lexer);
  tcase_add_test(tc, test_lexa_tokenize);
  add_tcase(tc);
}
 
