/*
 * /obelix/test/str.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>
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

#include "tcore.h"
#include <stdio.h>

#include <array.h>
#include <data.h>
#include <dict.h>
#include <list.h>
#include <str.h>

static void _init_tresolve(void) __attribute__((constructor(300)));

START_TEST(test_str_copy_chars)
  char *text = "This is a test string";
  str_t *str;

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  str_free(str);

  str = str_copy_nchars("0123456789", 0);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), 0);
  str_free(str);
END_TEST

START_TEST(test_str_copy)
  char *text = "This is a test string";
  str_t *str, *wrap, *ret;

  wrap = str_wrap(text);
  ck_assert_ptr_ne(wrap, NULL);
  ck_assert_int_eq(str_len(wrap), strlen(text));
  ret = str_append_char(wrap, 'A');
  ck_assert_ptr_eq(ret, NULL);

  str = str_copy(wrap);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), str_len(wrap));
  str_free(str);
  str_free(wrap);
END_TEST

START_TEST(test_str_slice)
  char *text = "This is a test string";
  str_t *str, *slice;

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  slice = str_slice(str, 1, 4);
  ck_assert_ptr_ne(slice, NULL);
  ck_assert_int_eq(str_len(slice), 3);
  ck_assert_str_eq(str_chars(slice), "his");
  str_free(slice);

  slice = str_slice(str, 0, 4);
  ck_assert_ptr_ne(slice, NULL);
  ck_assert_int_eq(str_len(slice), 4);
  ck_assert_str_eq(str_chars(slice), "This");
  str_free(slice);

  slice = str_slice(str, -1, 4);
  ck_assert_ptr_ne(slice, NULL);
  ck_assert_int_eq(str_len(slice), 4);
  ck_assert_str_eq(str_chars(slice), "This");
  str_free(slice);

  slice = str_slice(str, 15, 21);
  ck_assert_ptr_ne(slice, NULL);
  ck_assert_int_eq(str_len(slice), 6);
  ck_assert_str_eq(str_chars(slice), "string");
  str_free(slice);

  slice = str_slice(str, 15, 22);
  ck_assert_ptr_ne(slice, NULL);
  ck_assert_int_eq(str_len(slice), 6);
  ck_assert_str_eq(str_chars(slice), "string");
  str_free(slice);

  str_free(str);
END_TEST

START_TEST(test_str_chop)
  char *text = "This is a test string";
  str_t *str, *ret;

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_chop(str, 7);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), 14);
  ck_assert_str_eq(str_chars(str), "This is a test");
  str_free(str);

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_chop(str, 21);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), 0);
  ck_assert_str_eq(str_chars(str), "");
  str_free(str);

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_chop(str, 25);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), 0);
  ck_assert_str_eq(str_chars(str), "");
  str_free(str);

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_chop(str, 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  str_free(str);
END_TEST

START_TEST(test_str_lchop)
  char *text = "This is a test string";
  str_t *str, *ret;

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_lchop(str, 5);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), 16);
  ck_assert_str_eq(str_chars(str), "is a test string");
  str_free(str);

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_lchop(str, 21);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), 0);
  ck_assert_str_eq(str_chars(str), "");
  str_free(str);

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_lchop(str, 25);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), 0);
  ck_assert_str_eq(str_chars(str), "");
  str_free(str);

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_lchop(str, 0);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  str_free(str);
END_TEST

START_TEST(test_str_erase)
  char *text = "This is a test string";
  str_t *str, *ret;

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
  ret = str_erase(str);
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(str_len(str), 0);
  ck_assert_int_eq(strlen(str_chars(str)), 0);
  str_free(str);
END_TEST

START_TEST(test_str_indexof)
  char  *text = "This is a test string";
  str_t *str, *wrap;
  int    ix;

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));

  ix = str_indexof_chars(str, "This");
  ck_assert_int_eq(ix, 0);
  ix = str_indexof_chars(str, "test");
  ck_assert_int_eq(ix, 10);
  ix = str_indexof_chars(str, "is");
  ck_assert_int_eq(ix, 2);
  ix = str_rindexof_chars(str, "string");
  ck_assert_int_eq(ix, 15);
  ix = str_rindexof_chars(str, "test");
  ck_assert_int_eq(ix, 10);
  ix = str_rindexof_chars(str, "is");
  ck_assert_int_eq(ix, 5);

  wrap = str_wrap("test");
  ix = str_indexof(str, wrap);
  ck_assert_int_eq(ix, 10);
  ix = str_rindexof(str, wrap);
  ck_assert_int_eq(ix, 10);
  str_free(wrap);

  str_free(str);
END_TEST

START_TEST(test_str_ncopy)
  str_t *str1, *str2;
  char *text = "1234567890abcdefghijklmnopqrstuvwxyz";

  str1 = str_copy_nchars(text, 10);
  ck_assert_ptr_ne(str1, NULL);
  ck_assert_int_eq(str_len(str1), 10);
  ck_assert_str_eq(str_chars(str1), "1234567890");
  str2 = str_copy_nchars(str_chars(str1), 10);
  ck_assert_ptr_ne(str2, NULL);
  ck_assert_int_eq(str_len(str2), 10);
  ck_assert_str_eq(str_chars(str2), "1234567890");
  str_free(str1);
  str_free(str2);
END_TEST

START_TEST(test_str_split)
  array_t *array;
  int      ix;
  str_t   *str;
  str_t   *c;
  char    *test1 = "this,is,a,test,string";
  char    *test2 = ",this,is,a,test,string";
  char    *test3 = ",this,is,a,test,string,";
  char    *test4 = "this,is,a,test,string,";
  char    *test5 = "this,,is,a,test,string";

  str = str_wrap(test1);
  array = str_split(str, ",");
  ck_assert_ptr_ne(array, NULL);
  ck_assert_int_eq(array_size(array), 5);
  for (ix = 0; ix < array_size(array); ix++) {
    c = (str_t *) array_get(array, ix);
    ck_assert(str_indexof_chars(c, ",") < 0);
  }
  array_free(array);
  str_free(str);

  str = str_wrap(test2);
  array = str_split(str, ",");
  ck_assert_int_eq(array_size(array), 6);
  ck_assert_int_eq(strcmp(str_chars(array_get(array, 0)), ""), 0);
  array_free(array);
  str_free(str);

  str = str_wrap(test3);
  array = str_split(str, ",");
  ck_assert_int_eq(array_size(array), 7);
  ck_assert_int_eq(strcmp(str_chars(array_get(array, 6)), ""), 0);
  array_free(array);
  str_free(str);

  str = str_wrap(test4);
  array = str_split(str, ",");
  ck_assert_int_eq(array_size(array), 6);
  ck_assert_int_eq(strcmp(str_chars(array_get(array, 5)), ""), 0);
  array_free(array);
  str_free(str);

  str = str_wrap(test5);
  array = str_split(str, ",");
  ck_assert_int_eq(array_size(array), 6);
  ck_assert_int_eq(strcmp(str_chars(array_get(array, 1)), ""), 0);
  array_free(array);
  str_free(str);

  str = str_wrap("");
  array = str_split(str, ",");
  ck_assert_int_eq(array_size(array), 0);
  array_free(array);
  str_free(str);

  str = str_wrap(" ");
  array = str_split(str, ",");
  ck_assert_int_eq(array_size(array), 1);
  array_free(array);
  str_free(str);
END_TEST


START_TEST(test_str_join)
  list_t *list;
  str_t  *str;

  list = str_list_create();
  list_push(list, strdup("This"));
  list_push(list, strdup("is"));
  list_push(list, strdup("a"));
  list_push(list, strdup("test"));
  str = str_join(".", list, _list_reduce_chars);
  ck_assert_str_eq(str_chars(str), "This.is.a.test");
  str_free(str);
END_TEST

START_TEST(test_str_data)
  str_t  *str;
  data_t *data;

  str = str_wrap("This is a test");
  ck_assert_str_eq(str_chars(str), "This is a test");
  data = (data_t *) str;
  ck_assert_int_eq(data_type(data), String);
  ck_assert_str_eq(data_typename(data), "string");
  ck_assert_str_eq(data_tostring(data), "This is a test");
END_TEST

START_TEST(test_str_replace)
  str_t   *str;
  int      count;

  str = str_copy_chars("This is the best test string in history");
  count = str_replace_all(str, "is", "was");
  ck_assert_int_eq(count, 3);
  ck_assert_str_eq(str_chars(str), "Thwas was the best test string in hwastory");
  str_free(str);
  str = str_copy_chars("This is the best test string in history");
  count = str_replace_one(str, "is", "was");
  ck_assert_int_eq(count, 1);
  ck_assert_str_eq(str_chars(str), "Thwas is the best test string in history");
  str_free(str);
END_TEST

/* ------------------------------------------------------------------------ */

START_TEST(test_str_format)
  str_t       *str;
  arguments_t *args;
  dict_t      *kwargs;

  str = str_wrap("test");
  ck_assert_str_eq(str_chars(str), "test");
  ck_assert_str_eq(data_tostring((data_t *) str), "test");
  args = arguments_create_args(2, str), data_uncopy(str_wrap("arg2"));
  ck_assert_str_eq(arguments_arg_tostring(args, 0), "test");

  str = str_format("test ${0} test", args);
  ck_assert_str_eq(str_chars(str), "test test test");
  str_free(str);

  str = str_format("test ${9} test", args);
  ck_assert_str_eq(str_chars(str), "test ${9} test");
  str_free(str);

  str = str_format("${0} test", args);
  ck_assert_str_eq(str_chars(str), "test test");
  str_free(str);

  str = str_format("test ${0}", args);
  ck_assert_str_eq(str_chars(str), "test test");
  str_free(str);

  str = str_format("test ${ test", args);
  ck_assert_str_eq("test ${ test", str_chars(str));
  str_free(str);

  str = str_format("test ${0} test ${1} test", args);
  ck_assert_str_eq(str_chars(str), "test test test arg2 test");
  str_free(str);

  str = str_format("test ${1} test ${0} test", args);
  ck_assert_str_eq(str_chars(str), "test arg2 test test test");
  str_free(str);

  kwargs = strdata_dict_create();
  arguments_set_kwarg(args, "test", str_wrap("test"));

  str = str_format("test ${test} test", args);
  ck_assert_str_eq(str_chars(str), "test test test");
  str_free(str);

  str = str_format("test ${unknown} test", args);
  ck_assert_str_eq(str_chars(str), "test ${unknown} test");
  str_free(str);

  str = str_formatf("test \\${2} ${0;d} test ${1---} ${2;f} test ${1;s}", 42, "foo", 3.14);
  ck_assert_str_eq(str_chars(str), "test \\${2} 42 test ${1---} 3.140000 test foo");
  str_free(str);

  arguments_free(args);
END_TEST

/* ------------------------------------------------------------------------ */

extern void str_test_init(void) {
  TCase *tc = tcase_create("Str");

  tcase_add_test(tc, test_str_copy_chars);
  tcase_add_test(tc, test_str_copy);
  tcase_add_test(tc, test_str_ncopy);
  tcase_add_test(tc, test_str_slice);
  tcase_add_test(tc, test_str_chop);
  tcase_add_test(tc, test_str_lchop);
  tcase_add_test(tc, test_str_erase);
  tcase_add_test(tc, test_str_indexof);
  tcase_add_test(tc, test_str_split);
  tcase_add_test(tc, test_str_join);
  tcase_add_test(tc, test_str_data);
  tcase_add_test(tc, test_str_replace);
  add_tcase(tc);
}

extern void str_format_init(void) {
  TCase *tc = tcase_create("Format");

  tcase_add_test(tc, test_str_format);
  add_tcase(tc);
}
