/*
 * obelix/src/lexer/test/number.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

#include "lexertest.h"

/* ----------------------------------------------------------------------- */

class NumberTest : public LexerTest { protected:
  void SetUp() override {
    LexerTest::SetUp();
    withScanners();
    lexa_add_scanner(lexa, "number");
    EXPECT_EQ(dictionary_size(lexa->scanners), 4);
    lexa_build_lexer(lexa);
    EXPECT_TRUE(lexa);
  }
};

/* ----------------------------------------------------------------------- */

TEST_F(NumberTest, Integer) {
  lexa_set_stream(lexa, (data_t *) str("Hello 1234 World"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeInteger), 1);
}

TEST_F(NumberTest, NegativeInteger) {
  lexa_set_stream(lexa, (data_t *) str("Hello -1234 World"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeInteger), 1);
}

TEST_F(NumberTest, IntegerNoSpace) {
  lexa_set_stream(lexa, (data_t *) str("Hello -1234World"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 5);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeInteger), 1);
}

TEST_F(NumberTest, Hex) {
  lexa_set_stream(lexa, (data_t *) str("Hello 0x1234abcd World"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeHexNumber), 1);
}

TEST_F(NumberTest, HexNohexDigit) {
  lexa_set_stream(lexa, (data_t *) str("Hello 0x1234abcj World"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 7);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 3);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeHexNumber), 1);
}

TEST_F(NumberTest, FloatUnconfigured) {
  lexa_set_config_value(lexa, "number", "float=0");
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.12 World"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 8);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeInteger), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeDot), 1);
}

TEST_F(NumberTest, Float) {
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56 World"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
}

TEST_F(NumberTest, NegativeFloat) {
  lexa_set_stream(lexa, (data_t *) str("Hello -1234.56 World"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
}

TEST_F(NumberTest, ScientificFloat) {
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56e+02 World"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
}

TEST_F(NumberTest, SciFloatNoSignInExponent) {
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56e02 World"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeFloat), 1);
}

TEST_F(NumberTest, SciFloatNoExponent) {
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56e World"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeError), 1);
}

TEST_F(NumberTest, SciFloatExponentSignButNoExponent) {
  lexa_set_stream(lexa, (data_t *) str("Hello 1234.56e+ World"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 6);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeError), 1);
}

/* ------------------------------------------------------------------------ */
