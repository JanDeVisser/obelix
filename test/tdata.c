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

#include <array.h>
#include <data.h>
#include "collections.h"

#define TEST_STRING     "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define TEST_STRING_LEN 36

START_TEST(data_string)
  data_t *data = data_create_string(TEST_STRING);
  
  ck_assert_ptr_ne(data, NULL);
  ck_assert_int_eq(strcmp((char *) data -> ptrval, TEST_STRING), 0);
  ck_assert_int_eq(data_count, 1);
  data_free(data);
  ck_assert_int_eq(data_count, 0);
END_TEST

START_TEST(data_int)
  data_t  *d1 = data_create_int(1);
  data_t  *d2 = data_create_int(1);
  data_t  *sum;
  array_t *args;

  ck_assert_int_eq(d1 -> intval, 1);
  ck_assert_int_eq(d2 -> intval, 1);
  ck_assert_int_eq(data_count, 2);
  args = data_array_create(1);
  array_push(args, d2);
  ck_assert_int_eq(array_size(args), 1);
  sum = data_execute(d1, "+", args, NULL);
  ck_assert_int_eq(data_count, 3);
  ck_assert_int_eq(sum -> type, Int);
  ck_assert_int_eq(sum -> intval, 2);
  data_free(sum);
  ck_assert_int_eq(data_count, 2);

  array_clear(args);
  d2 = data_create_int(1);
  array_push(args, data_copy(d1));
  array_push(args, d2);
  array_push(args, data_copy(d2));
  sum = data_execute(NULL, "+", args, NULL);
  ck_assert_int_eq(data_count, 3);
  ck_assert_int_eq(sum -> type, Int);
  ck_assert_int_eq(sum -> intval, 3);

  array_free(args);
  data_free(d1);
  data_free(sum);
  ck_assert_int_eq(data_count, 0);
END_TEST

START_TEST(data_parsers)
  data_t *d;

  d = data_parse(String, TEST_STRING);
  ck_assert_str_eq(strcmp((char *) d -> ptrval, TEST_STRING), 0);
  data_free(d);
END_TEST

char * get_suite_name() {
  return "Data";
}

TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Data");

  tcase_add_test(tc, data_string);
  tcase_add_test(tc, data_int);
  tcase_add_test(tc, data_parsers);
  return tc;
}


