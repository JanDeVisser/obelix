//
// Created by Jan de Visser on 2021-10-07.
//

#include <gtest/gtest.h>
#include <lexer/Tokenizer.h>
#include <lexer/test/LexerTest.h>

namespace Obelix {

class KeywordTest : public ::LexerTest {
protected:
    unsigned int prepareWithBig()
    {
        initialize();
        add_scanner<Obelix::KeywordScanner>(Token(200, "Big"));
        return 200;
    }

    void prepareWithBigBad(unsigned int* big, unsigned int* bad)
    {
        initialize();
        add_scanner<Obelix::KeywordScanner>(Token(200, "Big"), Token(201, "Bad"));
        *big = 200;
        *bad = 201;
    }

    unsigned int prepareWithAbc()
    {
        initialize();
        add_scanner<Obelix::KeywordScanner>(
            Token(200, "abb"),
            Token(201, "aca"),
            Token(202, "aba"),
            Token(203, "aaa"),
            Token(204, "aab"),
            Token(205, "abc"),
            Token(206, "aac"),
            Token(207, "acc"),
            Token(208, "acb"));
        return 205;
    }

    void tokenizeBig(const char* s, int total_count, int big_count)
    {
        auto code = prepareWithBig();
        tokenize(s);
        EXPECT_EQ(tokens.size(), total_count);
        for (auto& token : tokens) {
            debug(lexer, "{}", token.to_string());
        }
        EXPECT_EQ(tokens_by_code[(Obelix::TokenCode)code].size(), big_count);
    }

    void tokenizeBigBad(const char* s, int total_count, int big_count, int bad_count)
    {
        initialize();
        unsigned int big, bad;
        prepareWithBigBad(&big, &bad);
        tokenize(s);
        debug(lexer, "{}", tokens[0].to_string());
        EXPECT_EQ(tokens.size(), total_count);
        EXPECT_EQ(tokens_by_code[(Obelix::TokenCode)big].size(), big_count);
        EXPECT_EQ(tokens_by_code[(Obelix::TokenCode)bad].size(), bad_count);
    }

    bool debugOn() override {
        return false;
    }

};

TEST_F(KeywordTest, Keyword)
{
    tokenizeBig("Big", 2, 1);
}

TEST_F(KeywordTest, KeywordSpace)
{
    tokenizeBig("Big ", 3, 1);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 1);
}

TEST_F(KeywordTest, KeywordIsPrefix)
{
    tokenizeBig("Bigger", 3, 1);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 1);
}

TEST_F(KeywordTest, KeywordAndIdentifiers)
{
    tokenizeBig("Hello Big World", 6, 1);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

TEST_F(KeywordTest, TwoKeywords)
{
    tokenizeBig("Hello Big Big Beautiful World", 10, 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 3);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 4);
}

TEST_F(KeywordTest, keyword_two_keywords_separated)
{
    tokenizeBig("Hello Big Beautiful Big World", 10, 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 3);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 4);
}

TEST_F(KeywordTest, keyword_big_bad_big)
{
    tokenizeBigBad("Hello Big World", 6, 1, 0);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

TEST_F(KeywordTest, keyword_big_bad_bad)
{
    tokenizeBigBad("Hello Bad World", 6, 0, 1);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

TEST_F(KeywordTest, keyword_big_bad_big_bad)
{
    tokenizeBigBad("Hello Big Bad World", 8, 1, 1);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 3);
}

TEST_F(KeywordTest, keyword_big_bad_bad_big)
{
    tokenizeBigBad("Hello Bad Big World", 8, 1, 1);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 3);
}

TEST_F(KeywordTest, keyword_abc)
{
    unsigned int abc = prepareWithAbc();

    tokenize("yyz abc ams");
    EXPECT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens_by_code[(Obelix::TokenCode)abc].size(), 1);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

}