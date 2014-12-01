/*
 * set.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include <set.h>
#include "collections.h"

#define MANY 500

typedef struct _test_set {
  set_t *set;
  test_t **tests;
  int size;
} test_set_t;

test_set_t * fill_many() {
  test_set_t *ret;
  set_t *set;
  int ix;
  char key[11];
  test_t **tests;

  mark_point();
  set = set_create((cmp_t) test_cmp);
  ck_assert_ptr_ne(set, NULL);
  ck_assert_int_eq(set_size(set), 0);
  set_set_hash(set, (hash_t) test_hash);
  set_set_free(set, (free_t) test_free);
  tests = (test_t **) resize_ptrarray(NULL, MANY, 0);
  ck_assert_ptr_ne(tests, NULL);
  mark_point();
  for (ix = 0; ix < MANY; ix++) {
    strrand(key, 10);
    tests[ix] = test_create(key);
    ck_assert_int_ne(set_add(set, tests[ix]), FALSE);
    ck_assert_int_eq(set_size(set), ix + 1);
  }
  ret = NEW(test_set_t);
  ck_assert_ptr_ne(ret, NULL);
  ret -> set = set;
  ret -> tests = tests;
  ret -> size = MANY;
  mark_point();
  return ret;
}


START_TEST(test_set_create)
  set_t *set;
  str_t *str;
  
  set = set_create((cmp_t) strcmp);
  ck_assert_ptr_ne(set, NULL);
  ck_assert_int_eq(set_size(set), 0);
  str = set_tostr(set);
  ck_assert_str_eq(str_chars(str), "{}");
  set_free(set);
END_TEST


START_TEST(test_set_add_one)
  set_t *set;
  str_t *str;
  
  set = set_create((cmp_t) strcmp);
  ck_assert_ptr_ne(set, NULL);
  ck_assert_int_eq(set_size(set), 0);
  set_set_hash(set, (hash_t) strhash);
  set_set_free(set, (free_t) free);
  set_set_tostring(set, (tostring_t) chars);
  set_add(set, strdup("key1"));
  ck_assert_int_eq(set_size(set), 1);
  str = set_tostr(set);
  ck_assert_str_eq(str_chars(str), "{key1}");
  set_free(set);
END_TEST

START_TEST(test_set_add_many)
  test_set_t *td;
  set_t *set;
  int ix;
  test_t **tests;

  td = fill_many();
  set = td -> set;
  tests = td -> tests;
  
  for (ix = 0; ix < td -> size; ix++) {
    mark_point();
    ck_assert_int_eq(set_has(set, tests[ix]), TRUE);    
  }
  
  mark_point();
  set_free(set);
  free(tests);
  free(td);
END_TEST

START_TEST(test_set_clear)
  test_set_t *td;
  set_t *set;

  td = fill_many();
  set = td -> set;

  set_clear(set);
  ck_assert_int_eq(set_size(set), 0);
  
  mark_point();
  set_free(set);
  free(td -> tests);
  free(td);
END_TEST

START_TEST(test_set_remove)
  test_set_t *td;
  set_t *set;
  int ix;
  test_t *copy;

  td = fill_many();
  set = td -> set;

  for (ix = 0; ix < td -> size; ix++) {
    mark_point();
    copy = test_copy(td -> tests[ix]);
    set_remove(set, td -> tests[ix]);
    ck_assert_int_eq(set_size(set), td -> size - ix - 1);
    ck_assert_int_eq(set_has(set, copy), FALSE);
    test_free(copy);
  }
  
  mark_point();
  set_free(set);
  free(td -> tests);
  free(td);
END_TEST

static void test_visitor(test_t *test) {
  mark_point();
  test -> flag = 1;
}

START_TEST(test_set_visit)
  test_set_t *td;
  set_t *set;
  int ix;

  td = fill_many();
  set = td -> set;

  mark_point();
  set_visit(set, (visit_t) test_visitor);

  for (ix = 0; ix < td -> size; ix++) {
    mark_point();
    ck_assert_int_eq(td -> tests[ix] -> flag, 1);
  }
  
  mark_point();
  set_free(set);
  free(td -> tests);
  free(td);
END_TEST

START_TEST(test_int_set)
  set_t *set;
  int ix;
  int values[MANY];

  initialize_random();
  set = set_create((cmp_t) NULL);
  ck_assert_ptr_ne(set, NULL);
  for (ix = 0; ix < MANY; ix++) {
    values[ix] = rand();
    set_add(set, (void *) ((long) values[ix]));
  }
  ck_assert_int_eq(set_size(set), MANY);

  for (ix = 0; ix < MANY; ix++) {
    ck_assert(set_has(set, (void *) ((long) values[ix])));
  }
  set_free(set);
END_TEST

START_TEST(test_set_union)
  set_t *s1, *s2;
  int ix;

  s1 = intset_create();
  ck_assert_ptr_ne(s1, NULL);
  s2 = intset_create();
  ck_assert_ptr_ne(s2, NULL);
  for (ix = 0; ix < MANY; ix++) {
    set_add_int(((ix % 2) == 0) ? s1 : s2, ix);
  }
  ck_assert_int_eq(set_size(s1), MANY/2);
  ck_assert_int_eq(set_size(s2), MANY/2);
  ck_assert(set_disjoint(s1, s2));
  ck_assert(set_cmp(s1, s2));
  set_union(s1, s2);
  ck_assert_int_eq(set_size(s1), MANY);
  for (ix = 0; ix < MANY; ix++) {
    ck_assert(set_has_int(s1, ix));
  }
  set_free(s1);
  set_free(s2);
END_TEST

START_TEST(test_set_intersect)
  set_t *s1, *s2;
  int ix;

  s1 = intset_create();
  ck_assert_ptr_ne(s1, NULL);
  s2 = intset_create();
  ck_assert_ptr_ne(s2, NULL);
  /*
   * s1: All evens (750) + the odd multiples of 3 (500/2) = 1000
   * s2: All odds (750) + the even multiples of 3 (500/2) = 1000
   *
   * intersect: all multiples of 3 (1000)
   */
  for (ix = 0; ix < 1500; ix++) {
    if ((ix % 3) == 0) {
      set_add_int(s1, ix);
      set_add_int(s2, ix);
    } else {
      set_add_int(((ix % 2) == 0) ? s1 : s2, ix);
    }
  }
  ck_assert_int_eq(set_size(s1), 1000);
  ck_assert_int_eq(set_size(s2), 1000);
  set_intersect(s1, s2);
  ck_assert_int_eq(set_size(s1), 500);
  for (ix = 0; ix < MANY; ix++) {
    if ((ix % 3) == 0) {
      ck_assert(set_has_int(s1, ix));
    } else {
      ck_assert(!set_has_int(s1, ix));
    }
  }
  set_free(s1);
  set_free(s2);
END_TEST

START_TEST(test_set_subsetof)
  set_t *s1, *s2;
  int ix;

  s1 = intset_create();
  ck_assert_ptr_ne(s1, NULL);
  s2 = intset_create();
  ck_assert_ptr_ne(s2, NULL);
  for (ix = 0; ix < MANY; ix++) {
    set_add_int(s2, ix);
    if ((ix % 2) == 0) {
      set_add_int(s1, ix);
    }
  }
  ck_assert(set_subsetof(s1, s2));
  ck_assert(set_cmp(s1, s2));
  set_free(s1);
  set_free(s2);
END_TEST

START_TEST(test_set_cmp)
  set_t *s1, *s2;
  int ix;

  s1 = intset_create();
  ck_assert_ptr_ne(s1, NULL);
  s2 = intset_create();
  ck_assert_ptr_ne(s2, NULL);
  for (ix = 0; ix < MANY; ix++) {
    set_add_int(s2, ix);
    set_add_int(s1, ix);
  }
  ck_assert(!set_cmp(s1, s2));
  set_free(s1);
  set_free(s2);
END_TEST

char * get_suite_name() {
  return "Set";
}


TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Set");

  tcase_add_test(tc, test_set_create);
  tcase_add_test(tc, test_set_add_one);
  tcase_add_test(tc, test_set_add_many);
  tcase_add_test(tc, test_set_clear);
  tcase_add_test(tc, test_set_remove);
  tcase_add_test(tc, test_set_visit);
  tcase_add_test(tc, test_int_set);
  tcase_add_test(tc, test_set_union);
  tcase_add_test(tc, test_set_intersect);
  tcase_add_test(tc, test_set_subsetof);
  tcase_add_test(tc, test_set_cmp);
  return tc;
}
