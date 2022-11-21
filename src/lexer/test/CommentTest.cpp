/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <gtest/gtest.h>
#include <lexer/Tokenizer.h>
#include <lexer/test/LexerTest.h>

namespace Obelix {

class CommentTest : public LexerTest {
protected:

    void SetUp() override {
        LexerTest::SetUp();
        initialize();
        add_scanner<CommentScanner>(
            CommentScanner::CommentMarker { false, false, "/*", "*/" },
            CommentScanner::CommentMarker { false, true, "//", "" },
            CommentScanner::CommentMarker { true, true, "#", "" });
    }

    bool debugOn() override {
        return false;
    }
};


TEST_F(CommentTest, Comment) {
    tokenize("BeforeComment /* comment */ AfterComment");
    EXPECT_EQ(tokens.size(), 6);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Identifier), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Whitespace), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Comment), 1);
    EXPECT_EQ(tokens[2].value(), " comment ");
}

TEST_F(CommentTest, SlashInComment) {
    tokenize("BeforeComment /* com/ment */ AfterComment");
    EXPECT_EQ(tokens.size(), 6);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Identifier), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Whitespace), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Comment), 1);
    EXPECT_EQ(tokens[2].value(), " com/ment ");
}

TEST_F(CommentTest, SlashStartsComment) {
    tokenize("BeforeComment /*/ comment */ AfterComment");
    EXPECT_EQ(tokens.size(), 6);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Identifier), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Whitespace), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Comment), 1);
    EXPECT_EQ(tokens[2].value(), "/ comment ");
}

TEST_F(CommentTest, SlashEndsComment) {
    tokenize("BeforeComment /* comment /*/ AfterComment");
    EXPECT_EQ(tokens.size(), 6);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Identifier), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Whitespace), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Comment), 1);
    EXPECT_EQ(tokens[2].value(), " comment /");
}

TEST_F(CommentTest, SlashOutsideComment) {
    tokenize("Before/Comment /* comment /*/ AfterComment");
    EXPECT_EQ(tokens.size(), 8);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Identifier), 3);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Whitespace), 2);
    EXPECT_EQ(count_tokens_with_code(TokenCode::Comment), 1);
}

#if 0

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

#endif

}
