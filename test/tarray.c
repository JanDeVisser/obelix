/*
 * array.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <array.h>

#include "collections.h"

array_t * build_test_array() {
  array_t *array;
  test_t  *test;
  int      ix;
  char     buf[10];
  
  array = array_create(4);
  array_set_free(array, (visit_t) test_free);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "test%d", ix);
    test = test_create(buf);
    test -> flag = ix;
    ck_assert_int_ne(array_set(array, -1, test), FALSE);
  }
  ck_assert_int_eq(array_size(array), 100);
  
  return array;
}

START_TEST(test_array_create)
  array_t *array;
  
  array = array_create(4);
  ck_assert_ptr_ne(array, NULL);
  ck_assert_int_eq(array_size(array), 0);
  ck_assert_int_eq(array_capacity(array), 4);
  array_free(array);
END_TEST


START_TEST(test_array_set)
  array_t *array;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  ck_assert_int_eq(array_size(array), 1);
  array_set_free(array, (visit_t) test_free);
  array_free(array);
END_TEST


START_TEST(test_array_set_append)
  array_t *array;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, -1, test_create("test2")), FALSE);
  ck_assert_int_eq(array_size(array), 1);
  array_set_free(array, (visit_t) test_free);
  array_free(array);
END_TEST


START_TEST(test_array_get)
  array_t *array;
  test_t *t;

  array = array_create(4);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  t = (test_t *) array_get(array, 0);
  ck_assert_ptr_ne(t, NULL);
  ck_assert_str_eq(t -> data, "test1");
  array_set_free(array, (visit_t) test_free);
  array_free(array);
END_TEST


START_TEST(test_array_get_error)
  array_t *array;

  array = array_create(4);
  array_set_free(array, (visit_t) test_free);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  test_t *t = (test_t *) array_get(array, 1);
  ck_assert_ptr_eq(t, NULL);
  ck_assert_int_eq(errno, EFAULT);
  array_free(array);
END_TEST


START_TEST(test_array_set_extend)
  array_t *array;
  test_t *t;

  array = array_create(4);
  array_set_free(array, (visit_t) test_free);
  ck_assert_int_ne(array_set(array, 0, test_create("test1")), FALSE);
  ck_assert_int_ne(array_set(array, 9, test_create("test2")), FALSE);
  ck_assert_int_eq(array_size(array), 10);
  ck_assert_int_ge(array_capacity(array), 10);

  t = (test_t *) array_get(array, 9);
  ck_assert_ptr_ne(t, NULL);
  ck_assert_str_eq(t -> data, "test2");

  t = (test_t *) array_get(array, 5);
  ck_assert_ptr_eq(t, NULL);
  array_free(array);
END_TEST

static int visit_count = 0;

void test_array_visitor(void *data) {
  test_t *t = (test_t *) data;
  t -> flag = 1;
  visit_count++;
}


START_TEST(test_array_visit)
  array_t *array;
  test_t *test;
  int ix;

  array = build_test_array();
  visit_count = 0;
  array_visit(array, test_array_visitor);
  ck_assert_int_eq(visit_count, 100);
  for (ix = 0; ix < array_size(array); ix++) {
    test = (test_t *) array_get(array, ix);
    ck_assert_int_eq(test -> flag, 1);
  }
  array_free(array);
END_TEST


void * test_array_reducer(void *data, void *curr) {
  long    count = (long) curr;
  test_t *t = (test_t *) data;

  count += t -> flag;
  return (void *) count;
}


START_TEST(test_array_reduce)
  array_t *array;
  long     count = 0;

  array = build_test_array();
  array_visit(array, test_array_visitor);
  count = (long) array_reduce(array, test_array_reducer, (void *) 0L);
  ck_assert_int_eq(count, 100);
  array_free(array);
END_TEST


START_TEST(test_array_clear)
  array_t *array;
  test_t  *test;
  int      cap;
  int      ix;
  char     buf[10];

  array = build_test_array();
  cap = array_capacity(array);
  array_clear(array);
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
  array_free(array);
END_TEST

START_TEST(test_array_split)
  array_t *array;
     
  array = array_split("This,is,a,test", ",");
  ck_assert_int_eq(array_size(array), 4);
  ck_assert_str_eq((char *) array_get(array, 2), "a");
  array_free(array);
END_TEST

START_TEST(test_array_split_starts_with_sep)
  array_t *array;
     
  array = array_split(",This,is,a,test", ",");
  ck_assert_int_eq(array_size(array), 5);
  ck_assert_str_eq((char *) array_get(array, 0), "");
  ck_assert_str_eq((char *) array_get(array, 3), "a");
  array_free(array);
END_TEST

START_TEST(test_array_split_ends_with_sep)
  array_t *array;
     
  array = array_split("This,is,a,test,", ",");
  ck_assert_int_eq(array_size(array), 5);
  ck_assert_str_eq((char *) array_get(array, 4), "");
  ck_assert_str_eq((char *) array_get(array, 2), "a");
  array_free(array);
END_TEST

START_TEST(test_array_slice)
  array_t *array;
  array_t *slice;
  test_t  *test;

  array = build_test_array();
  
  slice = array_slice(array, 10, 10);
  ck_assert_int_eq(array_size(slice), 10);
  test = (test_t *) array_get(slice, 2);
  ck_assert_int_eq(test -> flag, 12);
  test = (test_t *) array_get(slice, 0);
  ck_assert_int_eq(test -> flag, 10);
  test = (test_t *) array_get(slice, 9);
  ck_assert_int_eq(test -> flag, 19);
  test = (test_t *) array_get(slice, 10);
  ck_assert_ptr_eq(test, NULL);
  
  array_free(slice);
  
  /* Make sure that the original array is still intact: */
  test = (test_t *) array_get(array, 0);
  ck_assert_str_eq(test -> data, "test0");
  
  array_free(array);
END_TEST

START_TEST(test_array_slice_neg_num)
  array_t *array;
  array_t *slice;
  test_t  *test;

  array = build_test_array();
  
  slice = array_slice(array, 81, -10);
  ck_assert_int_eq(array_size(slice), 10);
  test = (test_t *) array_get(slice, 2);
  ck_assert_int_eq(test -> flag, 83);
  test = (test_t *) array_get(slice, 0);
  ck_assert_int_eq(test -> flag, 81);
  test = (test_t *) array_get(slice, 9);
  ck_assert_int_eq(test -> flag, 90);
  test = (test_t *) array_get(slice, 10);
  ck_assert_ptr_eq(test, NULL);
  
  array_free(slice);
  array_free(array);
END_TEST

START_TEST(test_array_tostr)
  array_t *array;
  array_t *split;
  str_t   *str;
  
  array = build_test_array();
  array_set_tostring(array, (tostring_t) test_tostring);
  mark_point();
  str = array_tostr(array);
  ck_assert_int_eq(str_len(str), 1280);
  split = array_split(str_chars(str), ", ");
  ck_assert_int_eq(array_size(split), 100);
  ck_assert_str_eq((char *) array_get(split, 0), "[test0 [0]");
  ck_assert_str_eq((char *) array_get(split, 10), "test10 [10]");
  ck_assert_str_eq((char *) array_get(split, 99), "test99 [99]]");
  str_free(str);
  array_free(split);
  array_free(array);
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
  tcase_add_test(tc, test_array_split);
  tcase_add_test(tc, test_array_split_starts_with_sep);
  tcase_add_test(tc, test_array_split_ends_with_sep);
  tcase_add_test(tc, test_array_slice);
  tcase_add_test(tc, test_array_slice_neg_num);
  tcase_add_test(tc, test_array_tostr);
  
  return tc;
}
