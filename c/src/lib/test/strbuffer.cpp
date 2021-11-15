/*
 * This file is part of ${PROJECT} (https://github.com/JanDeVisser/${PROJECT}).
 *
 * (c) Copyright Jan de Visser 2014-2021
 *
 * Foobar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "strtest.h"

TEST_F(StrTest, ReadAll) {
  string = str(TEST_STRING);
  char buf[TEST_STRING_LEN + 1];

  auto num = str_read(string, buf, -1);
  ASSERT_EQ(num, TEST_STRING_LEN);
  buf[num] = 0;
  ASSERT_STREQ(buf, TEST_STRING);
}

TEST_F(StrTest, ReadPart) {
  string = str(TEST_STRING);
  char buf[TEST_STRING_LEN + 1];

  auto num = str_read(string, buf, 10);
  ASSERT_EQ(num, 10);
  buf[10] = 0;
  ASSERT_STREQ(buf, "ABCDEFGHIJ");
}

TEST_F(StrTest, ReadTwoPart) {
  string = str(TEST_STRING);
  char buf[TEST_STRING_LEN + 1];

  auto num = str_read(string, buf, 10);
  ASSERT_EQ(num, 10);
  num = str_read(string, buf + 10, 10);
  ASSERT_EQ(num, 10);
  buf[20] = 0;
  ASSERT_STREQ(buf, "ABCDEFGHIJKLMNOPQRST");
}

TEST_F(StrTest, ReadBeyondEnd) {
  string = str(TEST_STRING);
  char buf[TEST_STRING_LEN + 1];

  auto num = str_read(string, buf, 10);
  ASSERT_EQ(num, 10);
  num = str_read(string, buf + 10, 40);
  ASSERT_EQ(num, TEST_STRING_LEN - 10);
  buf[TEST_STRING_LEN] = 0;
  ASSERT_STREQ(buf, TEST_STRING);
}

TEST_F(StrTest, ReadNull) {
  char buf[TEST_STRING_LEN + 1];
  auto num = str_read(NULL, buf, -1);
  ASSERT_EQ(num, -1);
}

TEST_F(StrTest, ReadNullStr) {
  string = str_wrap(NULL);
  char buf[TEST_STRING_LEN + 1];

  auto num = str_read(string, buf, -1);
  ASSERT_EQ(num, 0);
}

TEST_F(StrTest, ReadStatic) {
  string = str_wrap(TEST_STRING);
  char buf[TEST_STRING_LEN + 1];

  auto num = str_read(string, buf, -1);
  ASSERT_EQ(num, TEST_STRING_LEN);
  buf[num] = 0;
  ASSERT_STREQ(buf, TEST_STRING);
}
