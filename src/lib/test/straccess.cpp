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
  string = str_wrap(NULL);
  ASSERT_TRUE(string);
  ASSERT_TRUE(str_is_null(string));
}

TEST_F(StrTest, NotNullStr) {
  string = str(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_is_null(string));
}

TEST_F(StrTest, StaticIsNotNullStr) {
  string = str_wrap(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_is_null(string));
}

TEST_F(StrTest, NullIsNotNullStr) {
  ASSERT_FALSE(str_is_null(NULL));
}

/* -- str_is_static ------------------------------------------------------ */

TEST_F(StrTest, IsStatic) {
  string = str_wrap(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_TRUE(str_is_static(string));
}

TEST_F(StrTest, NotStatic) {
  string = str(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_is_static(string));
}

TEST_F(StrTest, NullStrIsNotStatic) {
  string = str_wrap(NULL);
  ASSERT_TRUE(string);
  ASSERT_FALSE(str_is_static(string));
}

TEST_F(StrTest, NullIsNotStatic) {
  ASSERT_FALSE(str_is_static(NULL));
}

/* -- str_len ------------------------------------------------------------ */

TEST_F(StrTest, StrLen) {
  string = str(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_EQ(str_len(string), TEST_STRING_LEN);
}

TEST_F(StrTest, StrLenStatic) {
  string = str_wrap(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_EQ(str_len(string), TEST_STRING_LEN);
}

TEST_F(StrTest, StrLenModifiedStatic) {
  char s[80];
  strcpy(s, "ABCD");
  string = str_wrap(s);
  ASSERT_TRUE(string);
  ASSERT_EQ(str_len(string), 4);
  strcat(s, "EFGH");
  ASSERT_EQ(str_len(string), 8);
}

TEST_F(StrTest, StrLenEmpty) {
  string = str_wrap("");
  ASSERT_TRUE(string);
  ASSERT_EQ(str_len(string), 0);
}

TEST_F(StrTest, StrLenNull) {
  ASSERT_EQ(str_len(NULL), -1);
}

TEST_F(StrTest, StrLenNullStr) {
  string = str_wrap(NULL);
  ASSERT_EQ(str_len(string), -1);
}

/* -- str_chars ---------------------------------------------------------- */

TEST_F(StrTest, Chars) {
  string = str(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

TEST_F(StrTest, CharsNull) {
  ASSERT_FALSE(str_chars(NULL));
}

TEST_F(StrTest, CharsNullStr) {
  string = str_wrap(NULL);
  ASSERT_TRUE(str_is_null(string));
  ASSERT_FALSE(str_chars(NULL));
}

TEST_F(StrTest, CharsStatic) {
  string = str_wrap(TEST_STRING);
  ASSERT_TRUE(string);
  ASSERT_TRUE(str_is_static(string));
  ASSERT_STREQ(str_chars(string), TEST_STRING);
}

/* -- str_hash ----------------------------------------------------------- */

TEST_F(StrTest, Hash) {
  int hash = strhash(TEST_STRING);
  string = str(TEST_STRING);
  ASSERT_TRUE(string);
  EXPECT_EQ(str_hash(string), hash);
}

TEST_F(StrTest, HashStatic) {
  int hash = strhash(TEST_STRING);
  string = str_wrap(TEST_STRING);
  ASSERT_TRUE(string);
  EXPECT_EQ(str_hash(string), hash);
}

TEST_F(StrTest, HashNullStr) {
  string = str_wrap(NULL);
  ASSERT_TRUE(string);
  EXPECT_EQ(str_hash(string), 0);
}

TEST_F(StrTest, HashNull) {
  EXPECT_EQ(str_hash(NULL), 0);
}

/* -- str_at ------------------------------------------------------------- */

TEST_F(StrTest, At) {
  string = str(DIGITS);
  ASSERT_EQ(str_at(string, 5), '5');
}

TEST_F(StrTest, AtZero) {
  string = str(DIGITS);
  ASSERT_EQ(str_at(string, 0), '0');
}

TEST_F(StrTest, AtStrlenMinusOne) {
  string = str(DIGITS);
  ASSERT_EQ(str_at(string, 9), '9');
}

TEST_F(StrTest, AtStrlen) {
  string = str(DIGITS);
  ASSERT_EQ(str_at(string, 10), -1);
}

TEST_F(StrTest, AtLarge) {
  string = str(DIGITS);
  ASSERT_EQ(str_at(string, 100), -1);
}

TEST_F(StrTest, AtNegative) {
  string = str(DIGITS);
  ASSERT_EQ(str_at(string, -2), '8');
}

TEST_F(StrTest, AtNull) {
  ASSERT_EQ(str_at(NULL, 5), -1);
}

TEST_F(StrTest, AtNullStr) {
  string = str_wrap(NULL);
  ASSERT_EQ(str_at(NULL, 5), -1);
}

TEST_F(StrTest, AtStatic) {
  string = str_wrap(DIGITS);
  ASSERT_EQ(str_at(string, 5), '5');
}

/* -- str_cmp ------------------------------------------------------------- */

TEST_F(StrTest, CmpS1LtS2) {
  string = str("ABCD");
  str_t *string2 = str("EFGH");

  ASSERT_LT(str_cmp(string, string2), 0);
  ASSERT_GT(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpS1GtS2) {
  string = str("EFGH");
  str_t *string2 = str("ABCD");

  ASSERT_GT(str_cmp(string, string2), 0);
  ASSERT_LT(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpS1EqS2) {
  string = str("ABCD");
  str_t *string2 = str("ABCD");

  ASSERT_EQ(str_cmp(string, string2), 0);
  ASSERT_EQ(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpS1PrefixOfS2) {
  string = str("ABCD");
  str_t *string2 = str("ABCDE");

  ASSERT_LT(str_cmp(string, string2), 0);
  ASSERT_GT(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpS2PrefixOfS1) {
  string = str("ABCDE");
  str_t *string2 = str("ABCD");

  ASSERT_GT(str_cmp(string, string2), 0);
  ASSERT_LT(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpS2Null) {
  string = str("ABCD");
  str_t *string2 = NULL;

  ASSERT_GT(str_cmp(string, string2), 0);
  ASSERT_LT(str_cmp(string2, string), 0);
}

TEST_F(StrTest, CmpS2StrNull) {
  string = str("ABCD");
  str_t *string2 = str_wrap(NULL);

  ASSERT_GT(str_cmp(string, string2), 0);
  ASSERT_LT(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpS1NullS2Null) {
  string = NULL;
  str_t *string2 = NULL;

  ASSERT_EQ(str_cmp(string, string2), 0);
  ASSERT_EQ(str_cmp(string2, string), 0);
}

TEST_F(StrTest, CmpS1NullS2StrNull) {
  string = NULL;
  str_t *string2 = str_wrap(NULL);

  ASSERT_LT(str_cmp(string, string2), 0);
  ASSERT_GT(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpS1StrNullS2StrNull) {
  string = str_wrap(NULL);
  str_t *string2 = str_wrap(NULL);

  ASSERT_EQ(str_cmp(string, string2), 0);
  ASSERT_EQ(str_cmp(string2, string), 0);
  str_free(string2);
}

TEST_F(StrTest, CmpStatic) {
  string = str_wrap("ABCD");
  str_t *string2 = str_wrap("EFGH");

  ASSERT_LT(str_cmp(string, string2), 0);
  ASSERT_GT(str_cmp(string2, string), 0);
  str_free(string2);
}

/* -- str_cmp_chars ------------------------------------------------------ */

TEST_F(StrTest, CmpCharsS1LtS2) {
  string = str("ABCD");
  const char *string2 = "EFGH";

  ASSERT_LT(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsS1GtS2) {
  string = str("EFGH");
  const char *string2 = "ABCD";

  ASSERT_GT(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsS1EqS2) {
  string = str("ABCD");
  const char *string2 = "ABCD";

  ASSERT_EQ(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsS1PrefixOfS2) {
  string = str("ABCD");
  const char *string2 = "ABCDE";

  ASSERT_LT(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsS2PrefixOfS1) {
  string = str("ABCDE");
  const char *string2 = "ABCD";

  ASSERT_GT(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsS2Null) {
  string = str("ABCD");
  const char *string2 = NULL;

  ASSERT_GT(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsS1Null) {
  string = NULL;
  const char *string2 = "ABCD";

  ASSERT_LT(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsS1NullS2Null) {
  string = NULL;
  const char *string2 = NULL;

  ASSERT_EQ(str_cmp_chars(string, string2), 0);
}

TEST_F(StrTest, CmpCharsStatic) {
  string = str_wrap("ABCD");
  const char *string2 = "EFGH";

  ASSERT_LT(str_cmp_chars(string, string2), 0);
}

