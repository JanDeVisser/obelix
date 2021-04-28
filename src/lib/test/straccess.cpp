/*
 * This file is part of obelix (https://github.com/JanDeVisser/obelix).
 *
 * (c) Copyright Jan de Visser 2014-2021
 *
 * Obelix is free software: you can redistribute it and/or modify
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

/* -- str_is_null -------------------------------------------------------- */

TEST_F(StrTest, IsNullStr) {
  str = str_wrap(NULL);
  ASSERT_TRUE(str);
  ASSERT_TRUE(str_is_null(str));
}

TEST_F(StrTest, NotNullStr) {
  str = str_copy_chars(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_FALSE(str_is_null(str));
}

TEST_F(StrTest, StaticIsNotNullStr) {
  str = str_wrap(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_FALSE(str_is_null(str));
}

TEST_F(StrTest, NullIsNotNullStr) {
  ASSERT_FALSE(str_is_null(NULL));
}

/* -- str_is_static ------------------------------------------------------ */

TEST_F(StrTest, IsStatic) {
  str = str_wrap(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_TRUE(str_is_static(str));
}

TEST_F(StrTest, NotStatic) {
  str = str_copy_chars(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_FALSE(str_is_static(str));
}

TEST_F(StrTest, NullStrIsNotStatic) {
  str = str_wrap(NULL);
  ASSERT_TRUE(str);
  ASSERT_FALSE(str_is_static(str));
}

TEST_F(StrTest, NullIsNotStatic) {
  ASSERT_FALSE(str_is_static(NULL));
}

/* -- str_len ------------------------------------------------------------ */

TEST_F(StrTest, StrLen) {
  str = str_copy_chars(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_EQ(str_len(str), TEST_STRING_LEN);
}

TEST_F(StrTest, StrLenStatic) {
  str = str_wrap(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_EQ(str_len(str), TEST_STRING_LEN);
}

TEST_F(StrTest, StrLenEmpty) {
  str = str_wrap("");
  ASSERT_TRUE(str);
  ASSERT_EQ(str_len(str), 0);
}

TEST_F(StrTest, StrLenNull) {
  ASSERT_EQ(str_len(NULL), -1);
}

TEST_F(StrTest, StrLenNullStr) {
  str = str_wrap(NULL);
  ASSERT_EQ(str_len(str), -1);
}

/* -- str_chars ---------------------------------------------------------- */

TEST_F(StrTest, Chars) {
  str = str_copy_chars(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

TEST_F(StrTest, CharsNull) {
  ASSERT_FALSE(str_chars(NULL));
}

TEST_F(StrTest, CharsNullStr) {
  str = str_wrap(NULL);
  ASSERT_TRUE(str_is_null(str));
  ASSERT_FALSE(str_chars(NULL));
}

TEST_F(StrTest, CharsStatic) {
  str = str_wrap(TEST_STRING);
  ASSERT_TRUE(str);
  ASSERT_TRUE(str_is_static(str));
  ASSERT_STREQ(str_chars(str), TEST_STRING);
}

/* -- str_hash ----------------------------------------------------------- */

TEST_F(StrTest, Hash) {
  int hash = strhash(TEST_STRING);
  str = str_copy_chars(TEST_STRING);
  ASSERT_TRUE(str);
  EXPECT_EQ(str_hash(str), hash);
}

TEST_F(StrTest, HashStatic) {
  int hash = strhash(TEST_STRING);
  str = str_wrap(TEST_STRING);
  ASSERT_TRUE(str);
  EXPECT_EQ(str_hash(str), hash);
}

TEST_F(StrTest, HashNullStr) {
  str = str_wrap(NULL);
  ASSERT_TRUE(str);
  EXPECT_EQ(str_hash(str), 0);
}

TEST_F(StrTest, HashNull) {
  EXPECT_EQ(str_hash(NULL), 0);
}

/* -- str_at ------------------------------------------------------------- */

TEST_F(StrTest, At) {
  str = str_copy_chars(DIGITS);
  ASSERT_EQ(str_at(str, 5), '5');
}

TEST_F(StrTest, AtZero) {
  str = str_copy_chars(DIGITS);
  ASSERT_EQ(str_at(str, 0), '0');
}

TEST_F(StrTest, AtStrlenMinusOne) {
  str = str_copy_chars(DIGITS);
  ASSERT_EQ(str_at(str, 9), '9');
}

TEST_F(StrTest, AtStrlen) {
  str = str_copy_chars(DIGITS);
  ASSERT_EQ(str_at(str, 10), -1);
}

TEST_F(StrTest, AtLarge) {
  str = str_copy_chars(DIGITS);
  ASSERT_EQ(str_at(str, 100), -1);
}

TEST_F(StrTest, AtNegative) {
  str = str_copy_chars(DIGITS);
  ASSERT_EQ(str_at(str, -2), '8');
}

TEST_F(StrTest, AtNull) {
  ASSERT_EQ(str_at(NULL, 5), -1);
}

TEST_F(StrTest, AtNullStr) {
  str = str_wrap(NULL);
  ASSERT_EQ(str_at(NULL, 5), -1);
}

TEST_F(StrTest, AtStatic) {
  str = str_wrap(DIGITS);
  ASSERT_EQ(str_at(str, 5), '5');
}

