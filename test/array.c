/*
 * list.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/array.h"
#include "collections.h"

START_TEST(test_array_create)
  array_t *array;
  
  array = array_create(4);
  ck_assert_ptr_ne(array, NULL);
  ck_assert_int_eq(array_size(array), 0);
  ck_assert_int_eq(array_capacity(array), 4);
  array_free(array, NULL);
END_TEST


START_TEST(test_array_set)
  array_t *array;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  ck_assert_int_eq(array_size(array), 1);
  array_free(array, (visit_t) test_free);
END_TEST


START_TEST(test_array_set_append)
  array_t *array;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, -1, test_create("test2")), FALSE);
  ck_assert_int_eq(array_size(array), 1);
  array_free(array, (visit_t) test_free);
END_TEST


START_TEST(test_array_get)
  array_t *array;
  test_t *t;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  t = (test_t *) array_get(array, 0);
  ck_assert_ptr_ne(t, NULL);
  ck_assert_str_eq(t -> data, "test1");
  array_free(array, (visit_t) test_free);
END_TEST


START_TEST(test_array_get_error)
  array_t *array;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  test_t *t = (test_t *) array_get(array, 1);
  ck_assert_ptr_eq(t, NULL);
  ck_assert_int_eq(errno, EFAULT);
  array_free(array, (visit_t) test_free);
END_TEST


START_TEST(test_array_set_extend)
  array_t *array;
  test_t *t;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  ck_assert_int_ne(array_set(array, 9, test_create("test2")), FALSE);
  ck_assert_int_eq(array_size(array), 10);
  ck_assert_int_ge(array_capacity(array), 10);

  t = (test_t *) array_get(array, 9);
  ck_assert_ptr_ne(t, NULL);
  ck_assert_str_eq(t -> data, "test2");

  t = (test_t *) array_get(array, 5);
  ck_assert_ptr_eq(t, NULL);
  array_free(array, (visit_t) test_free);
END_TEST


void test_array_visitor(void *data) {
  test_t *t = (test_t *) data;
  t -> flag = 1;
}


START_TEST(test_array_visit)
  array_t *array;
  test_t *test;
  int ix;
  char buf[10];

  array = array_create(4);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "test%d", ix);
    ck_assert_int_ne(array_set(array, -1, test_create(buf)), FALSE);
  }
  array_visit(array, test_array_visitor);
  for (ix = 0; ix < array_size(array); ix++) {
    test = (test_t *) array_get(array, ix);
    ck_assert_int_eq(test -> flag, 1);
  }
  array_free(array, (visit_t) test_free);
END_TEST


void * test_array_reducer(void *data, void *curr) {
  long count = (long) curr;
  test_t *t = (test_t *) data;

  count += t -> flag;
  return (void *) count;
}


START_TEST(test_array_reduce)
  array_t *array;
  test_t *test;
  int ix;
  char buf[10];

  array = array_create(4);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "test%d", ix);
    ck_assert_int_ne(array_set(array, -1, test_create(buf)), FALSE);
  }
  array_visit(array, test_array_visitor);
  long count = (long) array_reduce(array, test_array_reducer, (void *) 0L);
  ck_assert_int_eq(count, 100);
  array_free(array, (visit_t) test_free);
END_TEST


START_TEST(test_array_clear)
  array_t *array;
  test_t *test;
  int ix;
  int cap;
  char buf[10];

  array = array_create(4);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "test%d", ix);
    ck_assert_int_ne(array_set(array, -1, test_create(buf)), FALSE);
  }
  cap = array_capacity(array);
  array_clear(array, (visit_t) test_free);
  ck_assert_int_eq(array_size(array), 0);
  ck_assert_int_eq(array_capacity(array), cap);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "--test%d", ix);
    ck_assert_int_ne(array_set(array, ix, test_create(buf)), FALSE);
  }
  ck_assert_int_eq(array_capacity(array), cap);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "--test%d", ix);
    test = (test_t *) array_get(array, ix);
    ck_assert_str_eq(test -> data, buf);
  }
  array_free(array, (visit_t) test_free);
END_TEST


char * get_suite_name() {
  return "Array";
}


TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Array");

  tcase_add_test(tc, test_array_create);
  tcase_add_test(tc, test_array_set);
  tcase_add_test(tc, test_array_set_append);
  tcase_add_test(tc, test_array_get);
  tcase_add_test(tc, test_array_get_error);
  tcase_add_test(tc, test_array_set_extend);
  tcase_add_test(tc, test_array_visit);
  tcase_add_test(tc, test_array_reduce);
  tcase_add_test(tc, test_array_clear);
  return tc;
}
 


