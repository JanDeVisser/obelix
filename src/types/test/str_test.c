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

#include <exception.h>
#include <testsuite.h>
#include "types.h"

#define TEST_STRING     "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
#define TEST_STRING_LEN 36

static void _init_str_test(void) __attribute__((constructor(300)));

START_TEST(data_string)
  data_t  *data = data_create(String, TEST_STRING);
  data_t  *ret;
  error_t *e;
  
  ck_assert_ptr_ne(data, NULL);
  ck_assert_int_eq(strcmp((char *) data -> ptrval, TEST_STRING), 0);
  ck_assert_int_eq(data_count(), 1);
  
  ret = execute(data, "len", 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Int);
  ck_assert_int_eq(data_intval(ret), TEST_STRING_LEN);
  data_free(ret);
  
  ret = execute(data, "len", 1, Int, 10);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorArgCount);
  data_free(ret);

  ret = execute(data, "at", 1, Int, 10);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "K");
  data_free(ret);

  ret = execute(data, "at", 1, Int, 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "A");
  data_free(ret);

  ret = execute(data, "at", 1, Int, TEST_STRING_LEN-1);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "9");
  data_free(ret);

  ret = execute(data, "at", 1, Int, -1);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorRange);
  data_free(ret);

  ret = execute(data, "at", 1, Int, TEST_STRING_LEN);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorRange);
  data_free(ret);

  ret = execute(data, "at", 2, Int, 10, Int, 20);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorArgCount);
  data_free(ret);

  ret = execute(data, "at", 1, String, "string");
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorType);
  data_free(ret);

  ret = execute(data, "slice", 2, Int, 0, Int, 1);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "A");
  data_free(ret);

  ret = execute(data, "slice", 2, Int, -2, Int, 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_str_eq(data_charval(ret), "89");
  data_free(ret);

  ret = execute(data, "+", 2, String, "0123456789", String, "0123456789");
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), String);
  ck_assert_int_eq(strlen(data_charval(ret)), 56);
  data_free(ret);

  ret = execute(data, "+", 2, String, "0123456789", Int, 10);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(data_type(ret), Error);
  e = (error_t *) ret -> ptrval;
  ck_assert_int_eq(e -> code, ErrorType);
  data_free(ret);

  data_free(data);
  typedescr_count();
  ck_assert_int_eq(data_count(), 0);
END_TEST

START_TEST(str_parse)
  data_t *d;

  d = data_parse(String, TEST_STRING);
  ck_assert_ptr_ne(d, NULL);
  ck_assert_int_eq(d -> type, String);
  ck_assert_str_eq(d -> ptrval, TEST_STRING);
  data_free(d);
END_TEST

void _init_str_test(void) {
  TCase *tc = tcase_create("Str");

  tcase_add_test(tc, data_string);
  tcase_add_test(tc, str_parse);
  add_tcase(tc);
}
