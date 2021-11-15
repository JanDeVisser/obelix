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
 * along withthis program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "lexertest.h"


/* ----------------------------------------------------------------------- */

class CommentTest : public LexerTest {
protected:

  void SetUp() override {
    LexerTest::SetUp();
    withScanners();
    lexa_add_scanner(lexa, "comment: marker=/* */;marker=//;marker=^#");
    EXPECT_EQ(dictionary_size(lexa->scanners), 4);
    lexa_build_lexer(lexa);
    EXPECT_TRUE(lexa);
  }

  bool debugOn() override {
    return false;
  }
};


TEST_F(CommentTest, Comment) {
  lexa_set_stream(lexa, (data_t *) str("BeforeComment /* comment */ AfterComment"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 5);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
}

TEST_F(CommentTest, UnterminatedComment) {
  lexa_set_stream(lexa, (data_t *) str("UnterminatedComment /* comment"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeError), 1);
}

TEST_F(CommentTest, AsteriskComment) {
  lexa_set_stream(lexa, (data_t *) str("BeforeCommentWithAsterisk /* comment * comment */ AfterComment"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 5);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 2);
}

TEST_F(CommentTest, EOLComment) {
  lexa_set_stream(lexa, (data_t *) str(
    "BeforeLineEndComment // comment * comment */ World\n"
    "LineAfterLineEndComment"));
  EXPECT_TRUE(lexa -> stream);
  lexa_tokenize(lexa);
  EXPECT_EQ(lexa -> tokens, 5);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeIdentifier), 2);
  EXPECT_EQ(lexa_tokens_with_code(lexa, TokenCodeWhitespace), 1);
}

/* ----------------------------------------------------------------------- */
