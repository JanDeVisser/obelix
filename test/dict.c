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
  dict_t *dict;
  int ix;
  char valbuf[10];
  char **keys;
  char *valptr;

  mark_point();
  dict = dict_create((cmp_t) strcmp);
  ck_assert_ptr_ne(dict, NULL);
  ck_assert_int_eq(dict_size(dict), 0);
  dict_set_hash(dict, (hash_t) strhash);
  dict_set_free_key(dict, (visit_t) free);
  dict_set_free_data(dict, (visit_t) free);
  keys = (char **) resize_ptrarray(NULL, 500);
  mark_point();
  for (ix = 0; ix < 500; ix++) {
    mark_point();
    keys[ix] = malloc(11);
    mark_point();
    strrand(keys[ix], 10);
    mark_point();
    sprintf(valbuf, "%d", ix);
    mark_point();
    ck_assert_int_ne(dict_put(dict, strdup(keys[ix]), strdup(valbuf)), FALSE);
    mark_point();
    ck_assert_int_eq(dict_size(dict), ix + 1);
  }
  
  /* dict_dump(dict); */
  
  for (ix = 0; ix < 500; ix++) {
    mark_point();
    debug("ix: %d key: %s", ix, keys[ix]);
    mark_point();
    valptr = dict_get(dict, keys[ix]);
    debug("valptr: %s", valptr);
    mark_point();
    ck_assert_int_eq(atoi(valptr), ix);
    free(keys[ix]);
  }
  
  mark_point();
  dict_free(dict);
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
  return tc;
}
