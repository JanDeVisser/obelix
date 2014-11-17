/*
 * /obelix/test/resolve.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include <check.h>
#include "collections.h"
#include <resolve.h>

START_TEST(test_resolve_get)
  resolve_t *resolve;

  resolve = resolve_get();
  ck_assert_ptr_ne(resolve, NULL);
END_TEST

START_TEST(test_resolve_open)
  resolve_t *resolve;

  resolve = resolve_get();
  ck_assert_ptr_ne(resolve, NULL);
  ck_assert_ptr_ne(resolve_open(resolve, "libtestlib.so"), NULL);
END_TEST

START_TEST(test_resolve_resolve)
  resolve_t *resolve;
  voidptr_t tc;
  test_t    *test;

  resolve = resolve_get();
  ck_assert_ptr_ne(resolve, NULL);
  tc = resolve_resolve(resolve, "test_create");
  ck_assert_ptr_ne(tc, NULL);
  test = tc("test");
  ck_assert_str_eq(test -> data, "test");
END_TEST

START_TEST(test_resolve_library)
  ck_assert_int_ne(resolve_library("libtestlib.so"), 0);
END_TEST

START_TEST(test_resolve_function)
  voidptr_t tc;
  test_t    *test;

  tc = resolve_function("test_create");
  ck_assert_ptr_ne(tc, NULL);
  test = tc("test");
  ck_assert_str_eq(test -> data, "test");
END_TEST

START_TEST(test_resolve_foreign_function)
  voidptr_t hw;
  void     *test;

  ck_assert_int_ne(resolve_library("libtestlib.so"), 0);
  hw = resolve_function("testlib_helloworld");
  ck_assert_ptr_ne(hw, NULL);
  test = hw("test");
  ck_assert_ptr_ne(test, NULL);
END_TEST

char * get_suite_name() {
  return "Resolve";
}

TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Resolve");

  tcase_add_test(tc, test_resolve_get);
  tcase_add_test(tc, test_resolve_open);
  tcase_add_test(tc, test_resolve_resolve);
  tcase_add_test(tc, test_resolve_library);
  tcase_add_test(tc, test_resolve_function);
  tcase_add_test(tc, test_resolve_foreign_function);
  return tc;
}


