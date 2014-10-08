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
#include <string.h>
#include <stdlib.h>

#include "../src/list.h"

static void     test_raw_print_list(list_t *, char *);
static void     test_print_list(list_t *, char *);

list_t *test_list;

typedef struct _test {
  char *data;
  int flag;
} test_t;

test_t * test_create(char *data) {
  test_t *ret = NEW(test_t);
  if (ret) {
    ret -> data = (data) ? strdup(data) : NULL;
  }
  return ret;
}

void test_free(test_t *test) {
  if (test -> data) {
    free(test -> data);
  }
  free(test);
}

void setup(void) {
  test_list = list_create();
}

void teardown(void) {
  list_free(test_list, (visit_t) test_free);
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
  printf("zz");
  list_t *list = list_create();
  test_raw_print_list(test_list, "============================");
  ck_assert_ptr_ne(list, NULL);
  ck_assert_int_eq(list_size(list), 0);
END_TEST

START_TEST(test_list_append)
  list_append(test_list, test_create("test1"));
  ck_assert_int_eq(list_size(test_list), 1);
  list_append(test_list, test_create("test2"));
  ck_assert_int_eq(list_size(test_list), 2);
END_TEST

START_TEST(test_list_prepend)
  listiterator_t *iter = li_create(test_list);
  li_insert(iter, test_create("test0"));
  ck_assert_int_eq(list_size(test_list), 3);
END_TEST

START_TEST(test_list_insert)
  listiterator_t *iter = li_create(test_list);
  li_next(iter);
  li_insert(iter, test_create("test0.1"));
  ck_assert_int_eq(list_size(test_list), 3);
END_TEST

START_TEST(test_list_tail_insert)
  listiterator_t *iter = li_create(test_list);
  li_tail(iter);
  li_insert(iter, test_create("test2.xx"));
  ck_assert_int_eq(list_size(test_list), 3);
END_TEST

START_TEST(test_list_last_insert)
  listiterator_t *iter = li_create(test_list);
  li_tail(iter);
  li_prev(iter);
  li_insert(iter, test_create("test2.1"));
  ck_assert_int_eq(list_size(test_list), 4);
test_print_list(test_list, "==-=-=-=-=-=-=-=-=-==");
END_TEST

void test_list_del_second(list_t *list) {
  listiterator_t *iter = li_create(list);
  li_next(iter);
  li_next(iter);
  li_remove(iter);
  test_print_list(list, "After list_del_second");
}

void test_list_del_first(list_t *list) {
  listiterator_t *iter = li_create(list);
  li_next(iter);
  li_remove(iter);
  test_print_list(list, "After list_del_first");
}

void test_list_del_last(list_t *list) {
  listiterator_t *iter = li_create(list);
  li_tail(iter);
  li_prev(iter);
  li_remove(iter);
  test_print_list(list, "After list_del_last");
}

void test_list_del_tail(list_t *list) {
  listiterator_t *iter = li_create(list);
  li_tail(iter);
  li_remove(iter);
  test_print_list(list, "After list_del_tail");
}

void test_list_del_head(list_t *list) {
  listiterator_t *iter = li_create(list);
  li_head(iter);
  li_remove(iter);
  test_print_list(list, "After list_del_head");
}

void test_list_clear(list_t *list) {
  list_clear(list, NULL);
  test_print_list(list, "After list_clear");
}

void test_list_clear_free(list_t *list) {
  list_append(list, strdup("strdupped"));
  test_print_list(list, "After append(strdupped)");
  list_clear(list, free);
  test_print_list(list, "After list_clear(free)");
}

void test_list_visitor(void *data) {
  printf("%s = %d\n", (char *) data, strlen((char *) data));
}

test_list_visit(list_t *list) {
  list_visit(list, test_list_visitor);
}

void * test_list_reducer(void *data, void *curr) {
  long count = (long) curr;
  count += strlen((char *) data);
  return (void *) count;
}

test_list_reduce(list_t *list) {
  long count = (long) list_reduce(list, test_list_reducer, (void *) 0L);
  printf("list_reduce counted %d characters\n", count);
}

Suite * list_suite(void) {
  Suite *s;
  TCase *tc_create;
  TCase *tc_manip;
  
  s = suite_create("List");
  
  /* Core test case */
  tc_create = tcase_create("Create");
  
  tcase_add_test(tc_create, test_list_create);
  suite_add_tcase(s, tc_create);
  
  /* Manipulate test case */
  tc_manip = tcase_create("Manipulate");
 
  tcase_add_checked_fixture(tc_manip, setup, teardown);
  tcase_add_test(tc_manip, test_list_append);
  tcase_add_test(tc_manip, test_list_prepend);
  tcase_add_test(tc_manip, test_list_insert);
  tcase_add_test(tc_manip, test_list_tail_insert);
  tcase_add_test(tc_manip, test_list_last_insert);
  /* test_list_visit(list); */
  /* test_list_reduce(list); */
  /* test_list_del_second(list); */
  /* test_list_del_first(list); */
  /* test_list_del_last(list); */
  /* test_list_del_tail(list); */
  /* test_list_del_head(list); */
  /* test_list_clear(list); */
  suite_add_tcase(s, tc_manip);
  return s;
}

int main(void) {
  int number_failed;
  Suite *s;
  SRunner *sr;
 
  s = list_suite();
  sr = srunner_create(s);
 
  printf("xx\n");
  srunner_run_all(sr, CK_NORMAL);
  printf("yy\n");
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
