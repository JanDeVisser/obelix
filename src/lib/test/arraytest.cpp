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

#include <cstdio>
#include <cstdlib>

#include <list.h>
#include <str.h>
#include <testsuite.h>

class ArrayTest : public ::testing::Test {
public:
  array_t * arr { NULL };

protected:
  void SetUp() override {
    logging_set_level("DEBUG");
  }

  void build_test_array() {
    test_t * test;
    int ix;
    char buf[10];

    arr = array_create(4);
    array_set_free(arr, (visit_t) test_free);
    for (ix = 0; ix < 100; ix++) {
      sprintf(buf, "test%d", ix);
      test = test_create(buf);
      test->flag = ix;
      ASSERT_TRUE(array_set(arr, -1, test));
    }
    ASSERT_EQ(array_size(arr), 100);
  }

  void TearDown() override {
    array_free(arr);
    arr = NULL;
  }
};


TEST_F(ArrayTest, Create) {
  arr = array_create(4);
  ASSERT_TRUE(arr);
  ASSERT_EQ(array_size(arr), 0);
  ASSERT_EQ(array_capacity(arr), 4);
}

TEST_F(ArrayTest, CreateZero) {
  arr = array_create(0);
  ASSERT_TRUE(arr);
  ASSERT_EQ(array_size(arr), 0);
  ASSERT_GT(array_capacity(arr), 0);
}

TEST_F(ArrayTest, CreateNegative) {
  arr = array_create(-10);
  ASSERT_TRUE(arr);
  ASSERT_EQ(array_size(arr), 0);
  ASSERT_GT(array_capacity(arr), 0);
}

TEST_F(ArrayTest, Set) {
  arr = array_create(4);
  ASSERT_TRUE(array_set(arr, 0, (void *) "test1"));
  ASSERT_EQ(array_size(arr), 1);
}

TEST_F(ArrayTest, SetAppend) {
  arr = array_create(4);
  ASSERT_TRUE(array_set(arr, -1, (void *) "test2"));
  ASSERT_EQ(array_size(arr), 1);
}

TEST_F(ArrayTest, SetAppendLargeNegative) {
  arr = array_create(4);
  ASSERT_TRUE(array_set(arr, -100, (void *) "test2"));
  ASSERT_EQ(array_size(arr), 1);
}

TEST_F(ArrayTest, SetReplace) {
  arr = array_create(4);
  ASSERT_TRUE(array_set(arr, -1, (void *) "test0"));
  ASSERT_TRUE(array_set(arr, -1, (void *) "test1"));
  ASSERT_TRUE(array_set(arr, -1, (void *) "test2"));
  ASSERT_TRUE(array_set(arr, 1, (void *) "new_test1"));
  char *s = (char *) array_get(arr, 1);
  ASSERT_TRUE(s);
  ASSERT_STREQ(s, "new_test1");
}

TEST_F(ArrayTest, SetWithHole) {
  arr = array_create(4);
  ASSERT_TRUE(array_set(arr, 0, (void *) "test0"));
  ASSERT_TRUE(array_set(arr, 2, (void *) "test2"));
  ASSERT_EQ(array_size(arr), 3);
  char *s = (char *) array_get(arr, 1);
  ASSERT_FALSE(s);
  s = (char *) array_get(arr, 2);
  ASSERT_TRUE(s);
  ASSERT_STREQ(s, "test2");
}

TEST_F(ArrayTest, SetMany) {
  int   ix;
  char *s;

  arr = array_create(0);
  array_set_free(arr, free);
  for (ix = 0; ix < 200; ix++) {
    asprintf(&s, "test%d", ix);
    array_set(arr, ix, s);
  }
  ASSERT_EQ(array_size(arr), 200);
  ASSERT_GE(array_capacity(arr), 200);
  s = (char *) array_get(arr, 100);
  ASSERT_TRUE(s);
  ASSERT_STREQ(s, "test100");
  s = (char *) array_get(arr, 199);
  ASSERT_TRUE(s);
  ASSERT_STREQ(s, "test199");
}

TEST_F(ArrayTest, SetLargeHole) {
  arr = array_create(4);
  ASSERT_TRUE(array_set(arr, 0, (void *) "test0"));
  ASSERT_TRUE(array_set(arr, 200, (void *) "test200"));
  ASSERT_EQ(array_size(arr), 201);
  ASSERT_GE(array_capacity(arr), 201);
  char *s = (char *) array_get(arr, 100);
  ASSERT_FALSE(s);
  s = (char *) array_get(arr, 200);
  ASSERT_TRUE(s);
  ASSERT_STREQ(s, "test200");
}

TEST_F(ArrayTest, Get) {
  arr = array_create(4);
  ASSERT_TRUE(array_set(arr, 0, (void *) "test1"));
  char *s = (char *) array_get(arr, 0);
  ASSERT_TRUE(s);
  ASSERT_STREQ(s, "test1");
}

TEST_F(ArrayTest, GetOutOfRange) {
  array_t *arr;

  arr = array_create(4);
  array_set_free(arr, (visit_t) test_free);
  ASSERT_TRUE(array_set(arr, 0, (void *) "test1"));
  char *s = (char *) array_get(arr, 1);
  ASSERT_FALSE(s);
  ASSERT_EQ(errno, EFAULT);
}

#if 0
TEST_F(ArrayTest, test_array_set_extend) {
  array_t *arr;
  test_t * t;

  array = array_create(4);
  array_set_free(array, (visit_t) test_free);
  ASSERT_NE(array_set(array, 0, test_create("test1")), FALSE);
  ASSERT_NE(array_set(array, 9, test_create("test2")), FALSE);
  ASSERT_EQ(array_size(array), 10);
  ck_assert_int_ge(array_capacity(array), 10);

  t = (test_t *) array_get(array, 9);
  ASSERT_NE(t, NULL);
  ASSERT_STREQ(t->data, "test2");

  t = (test_t *) array_get(array, 5);
  ASSERT_EQ(t, NULL);
  array_free(array);
}

static int visit_count = 0;

void test_array_visitor(void * data) {
  test_t * t = (test_t *) data;
  t->flag = 1;
  visit_count++;
}


TEST_F(ArrayTest, test_array_visit) {
  array_t *arr;
  test_t * test;
  int ix;

  array = build_test_array();
  visit_count = 0;
  array_visit(array, test_array_visitor);
  ASSERT_EQ(visit_count, 100);
  for (ix = 0; ix < array_size(array); ix++) {
    test = (test_t *) array_get(array, ix);
    ASSERT_EQ(test->flag, 1);
  }
  array_free(array);
}


void * test_array_reducer(void * data, void * curr) {
  long count = (long) curr;
  test_t * t = (test_t *) data;

  count += t->flag;
  return (void *) count;
}


TEST_F(ArrayTest, test_array_reduce) {
  array_t *arr;
  long count = 0;

  array = build_test_array();
  array_visit(array, test_array_visitor);
  count = (long) array_reduce(array, test_array_reducer, (void *) 0L);
  ASSERT_EQ(count, 100);
  array_free(array);
}


TEST_F(ArrayTest, test_array_clear) {
  array_t *arr;
  test_t * test;
  int cap;
  int ix;
  char buf[10];

  array = build_test_array();
  cap = array_capacity(array);
  array_clear(array);
  ASSERT_EQ(array_size(array), 0);
  ASSERT_EQ(array_capacity(array), cap);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "--test%d", ix);
    ASSERT_FALSE(array_set(array, ix, test_create(buf)));
  }
  ASSERT_EQ(array_capacity(array), cap);
  for (ix = 0; ix < 100; ix++) {
    sprintf(buf, "--test%d", ix);
    test = (test_t *) array_get(array, ix);
    ASSERT_STREQ(test->data, buf);
  }
  array_free(array);
}

TEST_F(ArrayTest, test_array_split) {
  array_t *arr;

  array = array_split("This,is,a,test", ",");
  ASSERT_EQ(array_size(array), 4);
  ASSERT_STREQ((char *) array_get(array, 2), "a");
  array_free(array);
}

TEST_F(ArrayTest, test_array_split_starts_with_sep) {
  array_t *arr;

  array = array_split(",This,is,a,test", ",");
  ASSERT_EQ(array_size(array), 5);
  ASSERT_STREQ((char *) array_get(array, 0), "");
  ASSERT_STREQ((char *) array_get(array, 3), "a");
  array_free(array);
}

TEST_F(ArrayTest, test_array_split_ends_with_sep) {
  array_t *arr;

  array = array_split("This,is,a,test,", ",");
  ASSERT_EQ(array_size(array), 5);
  ASSERT_STREQ((char *) array_get(array, 4), "");
  ASSERT_STREQ((char *) array_get(array, 2), "a");
  array_free(array);
}

TEST_F(ArrayTest, test_array_slice) {
  array_t *arr;
  array_t * slice;
  test_t * test;

  array = build_test_array();

  slice = array_slice(array, 10, 10);
  ASSERT_EQ(array_size(slice), 10);
  test = (test_t *) array_get(slice, 2);
  ASSERT_EQ(test->flag, 12);
  test = (test_t *) array_get(slice, 0);
  ASSERT_EQ(test->flag, 10);
  test = (test_t *) array_get(slice, 9);
  ASSERT_EQ(test->flag, 19);
  test = (test_t *) array_get(slice, 10);
  ASSERT_EQ(test, NULL);

  array_free(slice);

/* Make sure that the original array is still intact: */
  test = (test_t *) array_get(array, 0);
  ASSERT_STREQ(test->data, "test0");

  array_free(array);
}

TEST_F(ArrayTest, test_array_slice_neg_num) {
  array_t *arr;
  array_t * slice;
  test_t * test;

  array = build_test_array();

  slice = array_slice(array, 81, -10);
  ASSERT_EQ(array_size(slice), 10);
  test = (test_t *) array_get(slice, 2);
  ASSERT_EQ(test->flag, 83);
  test = (test_t *) array_get(slice, 0);
  ASSERT_EQ(test->flag, 81);
  test = (test_t *) array_get(slice, 9);
  ASSERT_EQ(test->flag, 90);
  test = (test_t *) array_get(slice, 10);
  ASSERT_EQ(test, NULL);

  array_free(slice);
  array_free(array);
}

TEST_F(ArrayTest, test_array_tostr) {
  array_t *arr;
  array_t * split;
  str_t * str;

  array = build_test_array();
  array_set_tostring(array, (tostring_t) test_tostring);
  mark_point();
  str = array_to_str(array);
  ASSERT_EQ(str_len(str), 1282);
  split = array_split(str_chars(str), ", ");
  ASSERT_EQ(array_size(split), 100);
  ASSERT_STREQ((char *) array_get(split, 0), "[ test0 [0]");
  ASSERT_STREQ((char *) array_get(split, 10), "test10 [10]");
  ASSERT_STREQ((char *) array_get(split, 99), "test99 [99] ]");
  str_free(str);
  array_free(split);
  array_free(array);
}

#endif