/*
 * list.c - Copyright (c) 2014 Jan de Visser <jan@finiandarcy.com>  
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
#include <stdio.h>
#include <stdlib.h>

#include "../src/list.h"
#include "collections.h"

static void     test_raw_print_list(list_t *, char *);
static void     test_print_list(list_t *, char *);
static list_t * setup();
static list_t * setup2();
static void     teardown(list_t *);

list_t * setup(void) {
  list_t *ret;
  
  ret = list_create();
  ck_assert_ptr_ne(ret, NULL);
  ck_assert_int_eq(list_size(ret), 0);
  return ret;
}

list_t * setup2(void) {
  list_t *ret;
  
  ret = setup();
  list_append(ret, test_create("test1"));
  ck_assert_int_eq(list_size(ret), 1);
  list_append(ret, test_create("test2"));
  ck_assert_int_eq(list_size(ret), 2);
  return ret;
}

list_t * setup3(void) {
  list_t *ret;
  listiterator_t *iter;
  
  ret = setup2();
  iter = li_create(ret);
  li_insert(iter, test_create("test0"));
  ck_assert_int_eq(list_size(ret), 3);
  return ret;
}

list_t * setup4(void) {
  list_t *ret;
  listiterator_t *iter;
  
  ret = setup3();
  iter = li_create(ret);
  li_next(iter);
  li_insert(iter, test_create("test0.1"));
  ck_assert_int_eq(list_size(ret), 4);
  return ret;
}

list_t * setup5(void) {
  list_t *ret;
  listiterator_t *iter;
  
  ret = setup4();
  iter = li_create(ret);
  li_tail(iter);
  li_prev(iter);
  li_insert(iter, test_create("test2.1"));
  ck_assert_int_eq(list_size(ret), 5);
  return ret;
}

void teardown(list_t *list) {
  list_free(list, (visit_t) test_free);
}

void test_print_list(list_t *list, char *header) {
  listiterator_t *iter = li_create(list);

  printf("%s:\n{ ", header);
  while (li_has_next(iter)) {
    printf("[%s] ", (char *) li_next(iter));
  }
  printf(" } (%d)\n", list_size(list));
} 

void test_raw_print_list(list_t *list, char *header) {
  listnode_t *node = list -> head;

  printf("%s:\n{ ", header);
  while (node) {
    if (!node -> prev) {
      printf("[H%s] -> ", (char *) node -> data);
    } else if (!node -> next) {
      printf("[T%s]", (char *) node -> data);
    } else {
      printf("\"%s\" -> ", (char *) node -> data);
    }
    node = node -> next;
  }
  printf(" } (%d)\n", list_size(list));
} 

START_TEST(test_list_create)
  list_t *l = setup();
  teardown(l);
END_TEST

START_TEST(test_list_append)
  list_t *l;
  
  l = setup2();
  teardown(l);
END_TEST

START_TEST(test_list_prepend)
  list_t *l;
  
  l = setup3();
  teardown(l);
END_TEST

START_TEST(test_list_insert)
  list_t *l;
  
  l = setup4();
  teardown(l);
END_TEST

START_TEST(test_list_tail_insert)
  list_t *l;
  listiterator_t *iter;
  
  l = setup4();
  iter = li_create(l);
  li_tail(iter);
  li_insert(iter, test_create("test2.xx"));
  ck_assert_int_eq(list_size(l), 4);
  teardown(l);
END_TEST

START_TEST(test_list_last_insert)
  list_t *l;
  
  l = setup5();
  teardown(l);
END_TEST

START_TEST(test_list_del_second)
  list_t *l;
  listiterator_t *iter;
  
  l = setup5();
  iter = li_create(l);
  li_next(iter);
  li_next(iter);
  li_remove(iter);
  ck_assert_int_eq(list_size(l), 4);
  teardown(l);
END_TEST

START_TEST(test_list_del_first)
  list_t *l;
  listiterator_t *iter;
  
  l = setup5();
  iter = li_create(l);
  li_next(iter);
  li_remove(iter);
  ck_assert_int_eq(list_size(l), 4);
  teardown(l);
END_TEST

START_TEST(test_list_del_last)
  list_t *l;
  listiterator_t *iter;
  
  l = setup5();
  iter = li_create(l);
  li_tail(iter);
  li_prev(iter);
  li_remove(iter);
  ck_assert_int_eq(list_size(l), 4);
  teardown(l);
END_TEST

START_TEST(test_list_del_tail)
  list_t *l;
  listiterator_t *iter;
  
  l = setup5();
  iter = li_create(l);
  li_tail(iter);
  li_remove(iter);
  ck_assert_int_eq(list_size(l), 5);
  teardown(l);
END_TEST

START_TEST(test_list_del_head)
  list_t *l;
  listiterator_t *iter;
  
  l = setup5();
  iter = li_create(l);
  li_head(iter);
  li_remove(iter);
  ck_assert_int_eq(list_size(l), 5);
  teardown(l);
END_TEST

START_TEST(test_list_clear)
  list_t *l;
  
  l = setup5();
  list_clear(l, (visit_t) test_free);
  ck_assert_int_eq(list_size(l), 0);
  teardown(l);
END_TEST

void test_list_visitor(void *data) {
  test_t *t = (test_t *) data;
  t -> flag = 1;
}

START_TEST(test_list_visit)
  list_t *l;
  listiterator_t *iter;
  
  l = setup5();
  list_visit(l, test_list_visitor);
  iter = li_create(l);
  while (li_has_next(iter)) {
    test_t *test = (test_t *) li_next(iter);
    ck_assert_int_eq(test -> flag, 1);
  }
  teardown(l);
END_TEST

void * test_list_reducer(void *data, void *curr) {
  long count = (long) curr;
  test_t *t = (test_t *) data;
  
  count += t -> flag;
  return (void *) count;
}

START_TEST(test_list_reduce)
  list_t *l;
  
  l = setup5();
  list_visit(l, test_list_visitor);
  long count = (long) list_reduce(l, test_list_reducer, (void *) 0L);
  ck_assert_int_eq(count, 5);
  teardown(l);
END_TEST

TCase * tc_list(void) {
  TCase *tc;
  tc = tcase_create("List");

  /* tcase_add_checked_fixture(tc_core, setup, teardown); */
  tcase_add_test(tc, test_list_create);
  tcase_add_test(tc, test_list_append);
  tcase_add_test(tc, test_list_prepend);
  tcase_add_test(tc, test_list_insert);
  tcase_add_test(tc, test_list_tail_insert);
  tcase_add_test(tc, test_list_last_insert);
  tcase_add_test(tc, test_list_del_second);
  tcase_add_test(tc, test_list_del_first);
  tcase_add_test(tc, test_list_del_last);
  tcase_add_test(tc, test_list_del_tail);
  tcase_add_test(tc, test_list_del_head);
  tcase_add_test(tc, test_list_clear);
  tcase_add_test(tc, test_list_visit);
  tcase_add_test(tc, test_list_reduce);

  return tc;
}
 
