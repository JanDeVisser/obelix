/*
 * /obelix/src/types/format.c - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>
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

#include <testsuite.h>

#include <array.h>
#include <data.h>
#include <dict.h>
#include <str.h>

static void _init_format(void) __attribute__((constructor(300)));

START_TEST(format_copy)
  str_t *s = format("Test string", NULL, NULL);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST

START_TEST(format_one)
  array_t *args = data_array_create(1);
  str_t *s;
  
  array_push(args, data_create(String, "str"));
  s = format("Test ${0}ing", args, NULL);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST

START_TEST(format_two)
  array_t *args = data_array_create(1);
  str_t *s;
  
  array_push(args, data_create(String, "Test"));
  s = format("${0} string", args, NULL);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST

START_TEST(format_three)
  array_t *args = data_array_create(1);
  str_t *s;
  
  array_push(args, data_create(String, "ing"));
  s = format("Test str${0}", args, NULL);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST

START_TEST(format_four)
  array_t *args = data_array_create(1);
  str_t *s;
  
  array_push(args, data_create(String, "Test"));
  array_push(args, data_create(String, "string"));
  s = format("${0} ${1}", args, NULL);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST

START_TEST(format_five)
  array_t *args = data_array_create(1);
  str_t *s;
  
  array_push(args, data_create(String, "Test"));
  array_push(args, data_create(String, "string"));
  s = format("${0} ${x}", args, NULL);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test ${x}");
  str_free(s);
END_TEST
    
START_TEST(format_six)
  array_t *args = data_array_create(1);
  str_t *s;
  
  array_push(args, data_create(String, "Test"));
  s = format("${0} ${1}", args, NULL);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test ${1}");
  str_free(s);
END_TEST
    
START_TEST(format_seven)
  array_t *args = data_array_create(1);
  dict_t *kw = strdata_dict_create();
  str_t *s;
  
  array_push(args, data_create(String, "Test"));
  dict_put(kw, "one", data_create(String, "string"));
  s = format("${0} ${one}", args, kw);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST
    
START_TEST(format_eight)
  array_t *args = data_array_create(1);
  dict_t *kw = strdata_dict_create();
  str_t *s;
  
  array_push(args, data_create(String, "Test"));
  dict_put(kw, "oneoneon", data_create(String, "string"));
  s = format("${0} ${oneoneon}", args, kw);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST
    
START_TEST(format_nine)
  array_t *args = data_array_create(1);
  dict_t *kw = strdata_dict_create();
  str_t *s;
  
  array_push(args, data_create(String, "Test"));
  dict_put(kw, "oneoneone", data_create(String, "string"));
  s = format("${0} ${oneoneone}", args, kw);
  array_free(args);
  ck_assert_ptr_ne(s, NULL);
  ck_assert_str_eq(str_chars(s), "Test string");
  str_free(s);
END_TEST
    
void _init_format(void) {
  TCase *tc = tcase_create("Format");

  tcase_add_test(tc, format_copy);
  tcase_add_test(tc, format_one);
  tcase_add_test(tc, format_two);
  tcase_add_test(tc, format_three);
  tcase_add_test(tc, format_four);
  tcase_add_test(tc, format_five);
  tcase_add_test(tc, format_six);
  tcase_add_test(tc, format_seven);
  tcase_add_test(tc, format_eight);
  add_tcase(tc);
}
