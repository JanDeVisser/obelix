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


char * get_suite_name() {
  return "Dict";
}


TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Dict");

  tcase_add_test(tc, test_dict_create);
  tcase_add_test(tc, test_dict_put_one);
  return tc;
}
