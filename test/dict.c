/*
 * dict.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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

#include "../src/dict.h"
#include "collections.h"

#define MANY 500

typedef struct _test_dict {
  dict_t *dict;
  char **keys;
  int size;
} test_dict_t;

test_dict_t * fill_many() {
  test_t *test;
  test_dict_t *ret;
  dict_t *dict;
  int ix;
  char valbuf[10];
  char **keys;

  mark_point();
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_hash(dict, (hash_t) strhash);
  dict_set_free_key(dict, (visit_t) free);
  dict_set_free_data(dict, (visit_t) test_free);
  keys = (char **) resize_ptrarray(NULL, MANY);
  ck_assert_ptr_ne(keys, NULL);
  mark_point();
  for (ix = 0; ix < MANY; ix++) {
    keys[ix] = malloc(11);
    strrand(keys[ix], 10);
    sprintf(valbuf, "%d", ix);
    test = test_create(valbuf);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_ne(dict_put(dict, strdup(keys[ix]), test), FALSE);
    ck_assert_int_eq(dict_size(dict), ix + 1);
  }
  ret = NEW(test_dict_t);
  ck_assert_ptr_ne(ret, NULL);
  ret -> dict = dict;
  ret -> keys = keys;
  ret -> size = MANY;
  return ret;
}


START_TEST(test_dict_create)
  dict_t *dict;
  
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_free(dict);
END_TEST


START_TEST(test_dict_put_one)
  dict_t *dict;
  
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_hash(dict, (hash_t) strhash);
  dict_set_free_key(dict, (visit_t) free);
  dict_set_free_data(dict, (visit_t) free);
  dict_put(dict, strdup("key1"), strdup("data1"));
  ck_assert_int_eq(dict_size(dict), 1);
  dict_free(dict);
END_TEST


START_TEST(test_dict_put_one_get_one)
  dict_t *dict;
  void *d;
  
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_hash(dict, (hash_t) strhash);
  dict_set_free_key(dict, (visit_t) free);
  dict_set_free_data(dict, (visit_t) free);
  dict_put(dict, strdup("key1"), strdup("data1"));
  ck_assert_int_eq(dict_size(dict), 1);
  d = dict_get(dict, "key1");
  ck_assert_ptr_ne(d, NULL);
  ck_assert_str_eq(d, "data1");
  dict_free(dict);
END_TEST

START_TEST(test_dict_put_many)
  test_dict_t *td;
  test_t *test;
  dict_t *dict;
  int ix;
  char **keys;

  td = fill_many();
  dict = td -> dict;
  keys = td -> keys;
  /* dict_dump(dict); */
  
  for (ix = 0; ix < td -> size; ix++) {
    mark_point();
    test = (test_t *) dict_get(dict, keys[ix]);
    ck_assert_ptr_ne(test, NULL);
    ck_assert_int_eq(atoi((char *) test -> data), ix);
    free(keys[ix]);
  }
  
  mark_point();
  dict_free(dict);
  free(keys);
  free(td);
END_TEST

START_TEST(test_dict_clear)
  test_dict_t *td;
  test_t *test;
  dict_t *dict;
  int ix;
  char **keys;

  td = fill_many();
  dict = td -> dict;
  keys = td -> keys;

  dict_clear(dict);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_dump(dict);
  
  mark_point();
  dict_free(dict);
  free(keys);
  free(td);
END_TEST

START_TEST(test_dict_has_key)
  test_dict_t *td;
  dict_t *dict;
  int ix;
  char **keys;
  char keybuf[21];
  int success;

  td = fill_many();
  dict = td -> dict;
  keys = td -> keys;

  for (ix = 0; ix < td -> size; ix++) {
    mark_point();
    success = dict_has_key(dict, keys[ix]);
    ck_assert_int_eq(success, TRUE);
    sprintf(keybuf, "%s%s", keys[ix], keys[ix]);
    success = dict_has_key(dict, keybuf);
    ck_assert_int_eq(success, FALSE);
    free(keys[ix]);
  }
  
  mark_point();
  dict_free(dict);
  free(keys);
  free(td);
END_TEST

START_TEST(test_dict_remove)
  test_dict_t *td;
  dict_t *dict;
  int ix;
  char **keys;
  int success;

  td = fill_many();
  dict = td -> dict;
  keys = td -> keys;

  for (ix = 0; ix < td -> size; ix++) {
    mark_point();
    success = dict_remove(dict, keys[ix]);
    ck_assert_int_eq(success, TRUE);
    ck_assert_int_eq(dict_size(dict), td -> size - ix - 1);
    success = dict_remove(dict, keys[ix]);
    ck_assert_int_eq(success, FALSE);
    free(keys[ix]);
  }
  
  mark_point();
  dict_free(dict);
  free(keys);
  free(td);
END_TEST

char * get_suite_name() {
  return "Dict";
}


TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Dict");

  tcase_add_test(tc, test_dict_create);
  tcase_add_test(tc, test_dict_put_one);
  tcase_add_test(tc, test_dict_put_one_get_one);
  tcase_add_test(tc, test_dict_put_many);
  tcase_add_test(tc, test_dict_clear);
  tcase_add_test(tc, test_dict_has_key);
  tcase_add_test(tc, test_dict_remove);
  return tc;
}
