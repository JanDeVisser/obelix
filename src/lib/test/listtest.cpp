/*
 * This file is part of the obelix distribution (https://github.com/JanDeVisser/obelix).
 * Copyright (c) 2021 Jan de Visser.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include <stdio.h>
#include <stdlib.h>

#include <list.h>
#include <str.h>
#include <testsuite.h>

class ListTest : public ::testing::Test {
public:
  list_t         *list { NULL };
  listiterator_t *iter { NULL };

protected:
  void SetUp() override {
    logging_set_level("DEBUG");
  }

  void build0() {
    list = list_create();
    ASSERT_TRUE(list);
    list_set_type(list, type_test);
    ASSERT_EQ(list_size(list), 0);
  }

  void build1() {
    build0();
    list_append(list, test_create("test1"));
    ASSERT_EQ(list_size(list), 1);
    list_append(list, test_create("test2"));
    ASSERT_EQ(list_size(list), 2);
  }

  void build2() {
    build1();
    iter = li_create(list);
    li_insert(iter, test_create("test0"));
    ASSERT_EQ(list_size(list), 3);
  }

  void build3() {
    build2();
    li_next(iter);
    li_insert(iter, test_create("test0.1"));
    ASSERT_EQ(list_size(list), 4);
  }

  void build4() {
    build3();
    li_tail(iter);
    li_prev(iter);
    li_insert(iter, test_create("test2.1"));
    ASSERT_EQ(list_size(list), 5);
    li_free(iter);
    iter = NULL;
  }

  void TearDown() override {
    li_free(iter);
    list_free(list);
    iter = NULL;
    list = NULL;
  }
};

void test_print_list(list_t *list, char *header) {
  listiterator_t *iter = li_create(list);

  printf("%s:\n{ ", header);
  while (li_has_next(iter)) {
    printf("[%s] ", (char *) li_next(iter));
  }
  printf(" } (%d)\n", list_size(list));
}

void test_raw_print_list(list_t *list, char *header) {
  listnode_t *node = list_head_pointer(list);

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

TEST_F(ListTest, list_create) {
  str_t *str;

  build0();
  str = list_tostr(list);
  ASSERT_STREQ(str_chars(str), "<>");
  str_free(str);
}

TEST_F(ListTest, list_append) {
  str_t *str;

  build1();
  str = list_tostr(list);
  ASSERT_STREQ(str_chars(str), "<test1 [0], test2 [0]>");
  str_free(str);
}

TEST_F(ListTest, list_prepend) {
  build2();
}

TEST_F(ListTest, list_insert) {
  build3();
}

TEST_F(ListTest, list_tail_insert) {
  build3();
  li_free(iter);
  iter = li_create(list);
  li_tail(iter);
  li_insert(iter, test_create("test2.xx"));
  ASSERT_EQ(list_size(list), 4);
}

TEST_F(ListTest, list_last_insert) {
  build4();
}

TEST_F(ListTest, list_del_second) {
  build4();
  li_free(iter);
  iter = li_create(list);
  li_next(iter);
  li_next(iter);
  li_remove(iter);
  ASSERT_EQ(list_size(list), 4);
}

TEST_F(ListTest, list_del_first) {
  build4();
  li_free(iter);
  iter = li_create(list);
  li_next(iter);
  li_remove(iter);
  ASSERT_EQ(list_size(list), 4);
}

TEST_F(ListTest, list_del_last) {
  build4();
  li_free(iter);
  iter = li_create(list);
  li_tail(iter);
  li_prev(iter);
  li_remove(iter);
  ASSERT_EQ(list_size(list), 4);
}

TEST_F(ListTest, list_del_tail) {
  build4();
  li_free(iter);
  iter = li_create(list);
  li_tail(iter);
  li_remove(iter);
  ASSERT_EQ(list_size(list), 5);
}

TEST_F(ListTest, list_del_head) {
  build4();
  iter = li_create(list);
  li_head(iter);
  li_remove(iter);
  ASSERT_EQ(list_size(list), 5);
}

TEST_F(ListTest, list_clear) {
  build4();
  list_clear(list);
  ASSERT_EQ(list_size(list), 0);
}

void test_list_visitor(void *data) {
  test_t *t = (test_t *) data;
  t -> flag = 1;
}

TEST_F(ListTest, list_visit) {
  build4();
  list_visit(list, test_list_visitor);
  iter = li_create(list);
  while (li_has_next(iter)) {
    test_t *test = (test_t *) li_next(iter);
    ASSERT_EQ(test -> flag, 1);
  }
}

void * test_list_reducer(void *data, void *curr) {
  intptr_t  count = (intptr_t) curr;
  test_t   *t = (test_t *) data;

  count += t -> flag;
  return (void *) count;
}

TEST_F(ListTest, list_reduce) {
  intptr_t count;

  build4();
  list_visit(list, test_list_visitor);
  count = (intptr_t) list_reduce(list, test_list_reducer, (void *) 0);
  ASSERT_EQ(count, 5);
}

TEST_F(ListTest, list_replace) {
  intptr_t count;

  build4();
  li_free(iter);
  for (iter = li_create(list); li_has_next(iter); ) {
    li_next(iter);
    test_t *test = test_create("test--");
    test -> flag = 2;
    li_replace(iter, test);
  }
  count = (intptr_t) list_reduce(list, test_list_reducer, (void *) 0L);
  ASSERT_EQ(count, 10);
}

TEST_F(ListTest, list_add_all) {
  list_t *src;
  list_t *dest;

  build1();
  src = list;
  build0();
  dest = list;
  list_add_all(dest, src);
  ASSERT_EQ(list_size(dest), 2);
  list_add_all(dest, src);
  ASSERT_EQ(list_size(dest), 4);
  list_free(src);
}
