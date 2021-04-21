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
#include <data.h>
#include <file.h>
#include <heap.h>

class HeapTest : public ::testing::Test {
protected:
  void SetUp() override {
    logging_set_level("DEBUG");
    logging_enable("heap");
    logging_enable("file");
  }

  void *allocate(size_t size) {
    if (!size) size = sizeof(data_t);
    return heap_allocate(size);
  }

  void *allocate_random() {
    return allocate(sizeof(data_t) + (rand() % (256-sizeof(data_t))));
  }

  void TearDown() override {
  }
};

TEST_F(HeapTest, allocate) {
  void *buf = allocate(0);
  ASSERT_TRUE(buf);
}

TEST_F(HeapTest, allocate_random) {
  size_t size = sizeof(data_t) + (rand() % (256-sizeof(data_t)));
  void *buf = heap_allocate(size);
  ASSERT_TRUE(buf);
}

//TEST_F(HeapTest, allocate_many) {
//  for (int ix = 0; ix < 100000; ix++) {
//    size_t size = sizeof(data_t) + (rand() % (256 - sizeof(data_t)));
//    logging_enable("heap");
//    void *buf = heap_allocate(size);
//    ASSERT_TRUE(buf);
//  }
//}

TEST_F(HeapTest, deallocate) {
  void *buf1 = allocate(0);
  ASSERT_TRUE(buf1);
  void *buf2 = allocate(0);
  ASSERT_TRUE(buf2);
  void *buf3 = allocate(0);
  ASSERT_TRUE(buf3);
  void *buf4 = allocate(0);
  ASSERT_TRUE(buf4);
  void *buf5 = allocate(0);
  ASSERT_TRUE(buf5);

  heap_deallocate(buf3);
  ASSERT_FALSE(((data_t *) buf3)->is_live);
  ASSERT_EQ(((data_t *) buf3)->cookie, FREEBLOCK_COOKIE);
}

TEST_F(HeapTest, gc) {
  void *buf = allocate(0);
  ASSERT_TRUE(buf);

  heap_gc();
  ASSERT_FALSE(((data_t *) buf)->is_live);
  ASSERT_EQ(((data_t *) buf)->cookie, FREEBLOCK_COOKIE);
}

TEST_F(HeapTest, gc_with_root) {
  void *buf = allocate(0);
  ASSERT_TRUE(buf);
  heap_register_root(buf);

  heap_gc();
  ASSERT_TRUE(((data_t *) buf)->is_live);
  ASSERT_TRUE(((data_t *) buf)->marked);
}

TEST_F(HeapTest, create_data) {
  data_t *i = int_to_data(12);
  ASSERT_TRUE(i);
  ASSERT_EQ(data_intval(i), 12);
}

TEST_F(HeapTest, create_file) {
  file_t *f = file_open("/etc/passwd");
  ASSERT_TRUE(f);

  heap_gc();
  ASSERT_FALSE(((data_t *) f)->is_live);
  ASSERT_EQ(((data_t *) f)->cookie, FREEBLOCK_COOKIE);
}

TEST_F(HeapTest, list_with_strings) {
  datalist_t *list = datalist_create(NULL);
  ASSERT_TRUE(list);
  str_t *s = str_copy_chars("elem 1");
  datalist_push(list, s);
  datalist_push(list, str_copy_chars("elem 2"));
  datalist_push(list, str_copy_chars("elem 3"));
  datalist_push(list, str_copy_chars("elem 4"));
  datalist_push(list, str_copy_chars("elem 5"));

  heap_gc();
  ASSERT_FALSE(((data_t *) list)->is_live);
  ASSERT_FALSE(((data_t *) s)->is_live);
}

TEST_F(HeapTest, list_with_strings_as_root) {
  datalist_t *list = datalist_create(NULL);
  ASSERT_TRUE(list);
  heap_register_root(list);
  str_t *s = str_copy_chars("elem 1");
  datalist_push(list, s);
  datalist_push(list, str_copy_chars("elem 2"));
  datalist_push(list, str_copy_chars("elem 3"));
  datalist_push(list, str_copy_chars("elem 4"));
  datalist_push(list, str_copy_chars("elem 5"));

  heap_gc();
  ASSERT_TRUE(((data_t *) list)->is_live);
  ASSERT_TRUE(((data_t *) s)->is_live);
}

