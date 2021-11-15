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

TEST(NameTest, name_create) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  ASSERT_TRUE(name);
}

TEST(NameTest, name_size) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  ASSERT_EQ(name_size(name), 3);
}

TEST(NameTest, name_get) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  ASSERT_STREQ(name_get(name, 0), "Jan");
  ASSERT_STREQ(name_get(name, 1), "de");
  ASSERT_STREQ(name_get(name, 2), "Visser");
}

TEST(NameTest, name_get_out_of_bounds) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  ASSERT_FALSE(name_get(name, 4));
}

TEST(NameTest, name_first) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  ASSERT_STREQ(name_first(name), "Jan");
}

TEST(NameTest, name_last) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  ASSERT_STREQ(name_last(name), "Visser");
}

TEST(NameTest, name_tostring_sep) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  ASSERT_STREQ(name_tostring_sep(name, " "), "Jan de Visser");
}

TEST(NameTest, name_split) {
  name_t *name = name_split("Jan de Visser", " ");
  ASSERT_EQ(name_size(name), 3);
  ASSERT_STREQ(name_get(name, 0), "Jan");
  ASSERT_STREQ(name_get(name, 1), "de");
  ASSERT_STREQ(name_get(name, 2), "Visser");
}

TEST(NameTest, name_parse) {
  name_t *name = name_parse("Jan.de.Visser");
  ASSERT_EQ(name_size(name), 3);
  ASSERT_STREQ(name_get(name, 0), "Jan");
  ASSERT_STREQ(name_get(name, 1), "de");
  ASSERT_STREQ(name_get(name, 2), "Visser");
}

TEST(NameTest, name_deepcopy) {
  name_t *name = name_parse("Jan.de.Visser");
  name_t *copy = name_deepcopy(name);
  ASSERT_EQ(name_size(copy), 3);
  ASSERT_STREQ(name_get(copy, 0), "Jan");
  ASSERT_STREQ(name_get(copy, 1), "de");
  ASSERT_STREQ(name_get(copy, 2), "Visser");
}

TEST(NameTest, name_as_array) {
  name_t *name = name_parse("Jan.de.Visser");
  array_t *array = name_as_array(name);
  ASSERT_EQ(array_size(array), 3);
  ASSERT_STREQ(data_tostring(data_as_data(array_get(array, 0))), "Jan");
  ASSERT_STREQ(data_tostring(data_as_data(array_get(array, 1))), "de");
  ASSERT_STREQ(data_tostring(data_as_data(array_get(array, 2))), "Visser");
}

TEST(NameTest, name_as_list) {
  name_t *name = name_parse("Jan.de.Visser");
  datalist_t *list = name_as_list(name);
  ASSERT_EQ(datalist_size(list), 3);
  ASSERT_STREQ(data_tostring(datalist_get(list, 0)), "Jan");
  ASSERT_STREQ(data_tostring(datalist_get(list, 1)), "de");
  ASSERT_STREQ(data_tostring(datalist_get(list, 2)), "Visser");
}

TEST(NameTest, name_head) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  name_t *head = name_head(name);
  ASSERT_EQ(name_size(head), 2);
  ASSERT_STREQ(name_get(head, 0), "Jan");
  ASSERT_STREQ(name_get(head, 1), "de");
}

TEST(NameTest, name_tail) {
  name_t *name = name_create(3, "Jan", "de", "Visser");
  name_t *tail = name_tail(name);
  ASSERT_EQ(name_size(tail), 2);
  ASSERT_STREQ(name_get(tail, 0), "de");
  ASSERT_STREQ(name_get(tail, 1), "Visser");
}

