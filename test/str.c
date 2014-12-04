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

#include <list.h>
#include <str.h>
#include "collections.h"

START_TEST(test_str_copy_chars)
  char *text = "This is a test string";
  str_t *str;

  str = str_copy_chars(text);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_int_eq(str_len(str), strlen(text));
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
  
  str1 = str_copy_nchars(10, text);
  ck_assert_ptr_ne(str1, NULL);
  ck_assert_int_eq(str_len(str1), 10);
  ck_assert_str_eq(str_chars(str1), "1234567890");
  str2 = str_copy_nchars(10, str_chars(str1));
  ck_assert_ptr_ne(str2, NULL);
  ck_assert_int_eq(str_len(str2), 10);
  ck_assert_str_eq(str_chars(str2), "1234567890");
  str_free(str1);
  str_free(str2);
END_TEST

START_TEST(test_str_split)
  list_t         *list;
  listiterator_t *iter;
  str_t          *str;
  str_t          *c;
  char           *test = "this,is,a,test,string";

  str = str_wrap(test);
  list = str_split(str, ",");
  ck_assert_ptr_ne(list, NULL);
  ck_assert_int_eq(list_size(list), 5);
  for (iter = li_create(list); li_has_next(iter); ) {
    c = (str_t *) li_next(iter);
    ck_assert(str_indexof_chars(c, ",") < 0);
  }
  list_free(list);
  str_free(str);
END_TEST


char * get_suite_name() {
  return "Str";
}

TCase * get_testcase(int ix) {
  TCase *tc;
  if (ix > 0) return NULL;
  tc = tcase_create("Str");

  tcase_add_test(tc, test_str_copy_chars);
  tcase_add_test(tc, test_str_copy);
  tcase_add_test(tc, test_str_ncopy);
  tcase_add_test(tc, test_str_slice);
  tcase_add_test(tc, test_str_chop);
  tcase_add_test(tc, test_str_lchop);
  tcase_add_test(tc, test_str_erase);
  tcase_add_test(tc, test_str_indexof);
  tcase_add_test(tc, test_str_split);
  return tc;
}


