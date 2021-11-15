/*
 * obelix/src/lexer/test/qstring.c - Copyright (c) 2016 Jan de Visser <jan@finiandarcy.com>
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

class QStringTest : public LexerTest {
};

TEST_F(QStringTest, QString) {
  lexa_set_stream(lexa, (data_t *) str("Hello 'single quotes' `backticks` \"double quotes\" World"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 10);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 4);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeSQuotedStr), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeBQuotedStr), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeDQuotedStr), 1);
}

TEST_F(QStringTest, NoClose) {
  lexa_set_stream(lexa, (data_t *) str("Hello 'no close quote"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeError), 1);
}

int ok = 0;
const char *filter;

void _test_filter(token_t *token) {
  if (token_code(token) == TokenCodeSQuotedStr) {
    ok = (strcmp(token_token(token), filter) == 0) ? 1 : -1;
  }
}

TEST_F(QStringTest, EscapedBackslash) {
  lexa_set_stream(lexa, (data_t *) str("Hello 'escaped backslash \\\\'"));
  filter = "escaped backslash \\";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  EXPECT_EQ(ok, 1);
}

TEST_F(QStringTest, EscapedQuote) {
  lexa_set_stream(lexa, (data_t *) str("Hello 'escaped quote \\''"));
  filter = "escaped quote '";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  EXPECT_EQ(ok, 1);
}

TEST_F(QStringTest, NoEscape) {
  lexa_set_stream(lexa, (data_t *) str("Hello 'escape \\"));
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeError), 1);
}

TEST_F(QStringTest, Newline) {
  lexa_set_stream(lexa, (data_t *) str("Hello 'escaped\\nnewline''"));
  filter = "escaped\nnewline";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  EXPECT_EQ(ok, 1);
}

TEST_F(QStringTest, GratuitousEscape) {
  lexa_set_stream(lexa, (data_t *) str("Hello 'escaped \\$ dollarsign''"));
  filter = "escaped $ dollarsign";
  lexa_set_tokenfilter(lexa, _test_filter);
  lexa_tokenize(lexa);
  EXPECT_EQ(ok, 1);
}

/* ------------------------------------------------------------------------ */
