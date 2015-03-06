/*
 * src/bin/test/scripts.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <data.h>
#include <exception.h>
#include <testsuite.h>

static void     _init_scripts(void) __attribute__((constructor(101)));
static void     _init_scripts_cases(void) __attribute__((constructor(300)));

START_TEST(t1)
  data_t *d;

  d = run_script("t1");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 0);
  data_free(d);
END_TEST

START_TEST(t2)
  data_t  *d;
  error_t *e;

  d = run_script("t2");
  ck_assert_int_eq(data_type(d), Error);
  e = data_errorval(d);
  ck_assert_int_eq(e -> code, ErrorName);
  data_free(d);
END_TEST

START_TEST(t3)
  data_t *d;

  d = run_script("t3");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 2);
  data_free(d);
END_TEST

START_TEST(t4)
  data_t *d;

  d = run_script("t4");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 1);
  data_free(d);
END_TEST

START_TEST(t5)
  data_t *d;

  d = run_script("t5");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), -1);
  data_free(d);
END_TEST

START_TEST(t6)
  data_t *d;

  d = run_script("t6");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 10);
  data_free(d);
END_TEST

START_TEST(t7)
  data_t *d;

  d = run_script("t7");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 0);
  data_free(d);
END_TEST

START_TEST(t8)
  data_t *d;

  d = run_script("t8");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 4);
  data_free(d);
END_TEST

START_TEST(t9)
  data_t *d;

  d = run_script("t9");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 3);
  data_free(d);
END_TEST

START_TEST(t10)
  data_t *d;

  d = run_script("t10");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 6);
  data_free(d);
END_TEST

START_TEST(t11)
  data_t *d;

  d = run_script("t11");
  ck_assert_int_eq(data_type(d), Int);
  ck_assert_int_eq(data_intval(d), 1);
  data_free(d);
END_TEST

void _init_scripts(void) {
  set_suite_name("Scripts");
}

void _init_scripts_cases(void) {
  TCase *tc = tcase_create("Scripts");

  tcase_add_test(tc, t1);
  tcase_add_test(tc, t2);
  tcase_add_test(tc, t3);
  tcase_add_test(tc, t4);
  tcase_add_test(tc, t5);
  tcase_add_test(tc, t6);
  tcase_add_test(tc, t7);
  tcase_add_test(tc, t8);
  tcase_add_test(tc, t9);
  tcase_add_test(tc, t10);
  tcase_add_test(tc, t11);
  add_tcase(tc);
}
