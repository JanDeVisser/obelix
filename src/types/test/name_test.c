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

#include <name.h>
#include <testsuite.h>

void _init_name_test(void) __attribute__((constructor(300)));

START_TEST(test_name_create)
  name_t *n = name_create(0);
  ck_assert_ptr_ne(n, NULL);
  name_free(n);
  
  n = name_create(1, "foo");
  ck_assert_ptr_ne(n, NULL);
  name_free(n);
END_TEST

START_TEST(test_name_tostring)
  name_t *n = name_create(0);
  char *str;
  ck_assert_ptr_ne(n, NULL);
  str = name_tostring(n);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_str_eq(str, "");
  name_free(n);
  
  n = name_create(1, "foo");
  ck_assert_ptr_ne(n, NULL);
  str = name_tostring(n);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_str_eq(str, "foo");
  name_free(n);
  
  n = name_create(2, "foo", "bar");
  ck_assert_ptr_ne(n, NULL);
  str = name_tostring(n);
  ck_assert_ptr_ne(str, NULL);
  ck_assert_str_eq(str, "foo.bar");
  ck_assert_str_eq(name_tostring_sep(n, "--"), "foo--bar");
  ck_assert_str_eq(name_tostring(n), "foo.bar");
  ck_assert_str_eq(name_tostring_sep(n, "+"), "foo+bar");
  ck_assert_str_eq(name_tostring(n), "foo.bar");
  name_free(n);
END_TEST

START_TEST(test_name_size)
  name_t *n = name_create(0);
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 0);
  name_free(n);
  
  n = name_create(1, "foo");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 1);
  name_free(n);
  
  n = name_create(2, "foo", "bar");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 2);
  name_free(n);
END_TEST

START_TEST(test_name_split)
  name_t *n = name_split("", ".");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 0);
  name_free(n);
  
  n = name_split("foo", ".");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 1);
  name_free(n);
  
  n = name_split("foo.bar", ".");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 2);
  ck_assert_str_eq(name_tostring(n), "foo.bar");
  name_free(n);
END_TEST

START_TEST(test_name_parse)
  name_t *n = name_parse("");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 0);
  name_free(n);
  
  n = name_parse("foo");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 1);
  name_free(n);
  
  n = name_parse("foo.bar");
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 2);
  ck_assert_str_eq(name_tostring(n), "foo.bar");
  name_free(n);
END_TEST

START_TEST(test_name_tail)
  name_t *n = name_parse("foo.bar.baz");
  name_t *tail;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 3);
  
  tail = name_tail(n);
  ck_assert_int_eq(name_size(tail), 2);
  ck_assert_str_eq(name_tostring(tail), "bar.baz");
  
  name_free(tail);
  name_free(n);
END_TEST

START_TEST(test_name_tail_one)
  name_t *n = name_parse("foo");
  name_t *tail;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 1);
  
  tail = name_tail(n);
  ck_assert_int_eq(name_size(tail), 0);
  
  name_free(tail);
  name_free(n);
END_TEST

START_TEST(test_name_tail_empty)
  name_t *n = name_create(0);
  name_t *tail;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 0);
  
  tail = name_tail(n);
  ck_assert_int_eq(name_size(tail), 0);
  
  name_free(tail);
  name_free(n);
END_TEST

START_TEST(test_name_head)
  name_t *n = name_parse("foo.bar.baz");
  name_t *head;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 3);
  
  head = name_head(n);
  ck_assert_int_eq(name_size(head), 2);
  ck_assert_str_eq(name_tostring(head), "foo.bar");
  
  name_free(head);
  name_free(n);
END_TEST

START_TEST(test_name_head_one)
  name_t *n = name_parse("foo");
  name_t *head;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 1);
  
  head = name_head(n);
  ck_assert_int_eq(name_size(head), 0);
  
  name_free(head);
  name_free(n);
END_TEST

START_TEST(test_name_head_empty)
  name_t *n = name_create(0);
  name_t *head;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 0);
  
  head = name_head(n);
  ck_assert_int_eq(name_size(head), 0);
  
  name_free(head);
  name_free(n);
END_TEST

START_TEST(test_name_last)
  name_t *n = name_parse("foo.bar.baz");
  char   *last;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 3);
  
  last = name_last(n);
  ck_assert_ptr_ne(last, NULL);
  ck_assert_str_eq(last, "baz");
  
  name_free(n);
END_TEST

START_TEST(test_name_last_one)
  name_t *n = name_parse("foo");
  char   *last;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 1);
  
  last = name_last(n);
  ck_assert_ptr_ne(last, NULL);
  ck_assert_str_eq(last, "foo");
  
  name_free(n);
END_TEST

START_TEST(test_name_last_empty)
  name_t *n = name_create(0);
  char   *last;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 0);
  
  last = name_last(n);
  ck_assert_ptr_eq(last, NULL);
  
  name_free(n);
END_TEST

START_TEST(test_name_first)
  name_t *n = name_parse("foo.bar.baz");
  char   *first;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 3);
  
  first = name_first(n);
  ck_assert_ptr_ne(first, NULL);
  ck_assert_str_eq(first, "foo");
  
  name_free(n);
END_TEST

START_TEST(test_name_first_one)
  name_t *n = name_parse("foo");
  char   *first;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 1);
  
  first = name_first(n);
  ck_assert_ptr_ne(first, NULL);
  ck_assert_str_eq(first, "foo");
  
  name_free(n);
END_TEST

START_TEST(test_name_first_empty)
  name_t *n = name_create(0);
  char   *first;
  ck_assert_ptr_ne(n, NULL);
  ck_assert_int_eq(name_size(n), 0);
  
  first = name_first(n);
  ck_assert_ptr_eq(first, NULL);
  
  name_free(n);
END_TEST

void _init_name_test(void) {
  TCase *tc = tcase_create("Name");

  tcase_add_test(tc, test_name_create);
  tcase_add_test(tc, test_name_tostring);
  tcase_add_test(tc, test_name_size);
  tcase_add_test(tc, test_name_split);
  tcase_add_test(tc, test_name_parse);
  tcase_add_test(tc, test_name_tail);
  tcase_add_test(tc, test_name_tail_one);
  tcase_add_test(tc, test_name_tail_empty);
  tcase_add_test(tc, test_name_head);
  tcase_add_test(tc, test_name_head_one);
  tcase_add_test(tc, test_name_head_empty);
  tcase_add_test(tc, test_name_last);
  tcase_add_test(tc, test_name_last_one);
  tcase_add_test(tc, test_name_last_empty);
  tcase_add_test(tc, test_name_first);
  tcase_add_test(tc, test_name_first_one);
  tcase_add_test(tc, test_name_first_empty);
  add_tcase(tc);
}

