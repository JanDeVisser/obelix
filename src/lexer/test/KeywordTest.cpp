//
// Created by Jan de Visser on 2021-10-07.
//

#include <gtest/gtest.h>
#include <lexer/Lexer.h>
#include <lexer/test/Lexa.h>

namespace Obelix {

class KeywordTest : public ::LexerTestF {
protected:
    unsigned int prepareWithBig()
    {
        initialize();
        lexa.add_scanner<Obelix::KeywordScanner>(1, "Big");
        return 100;
    }

    void prepareWithBigBad(unsigned int* big, unsigned int* bad)
    {
        initialize();
        lexa.add_scanner<Obelix::KeywordScanner>(2, "Big", "Bad");
        *big = 100;
        *bad = 101;
    }

    unsigned int prepareWithAbc()
    {
        initialize();
        lexa.add_scanner<Obelix::KeywordScanner>(9,
            "abb",
            "aca",
            "aba",
            "aaa",
            "aab",
            "abc",
            "aac",
            "acc",
            "acb");
        return 105;
    }

    void tokenize(const char* s, int total_count, int big_count)
    {
        auto code = prepareWithBig();
        lexa.tokenize(s);
        EXPECT_EQ(lexa.tokens.size(), total_count);
        for (auto& token : lexa.tokens) {
            debug(lexer, "%s\n", token.to_string().c_str());
        }
        EXPECT_EQ(lexa.tokens_by_code[(Obelix::TokenCode)code].size(), big_count);
    }

    void tokenizeBigBad(const char* s, int total_count, int big_count, int bad_count)
    {
        initialize();
        unsigned int big, bad;
        prepareWithBigBad(&big, &bad);
        lexa.tokenize(s);
        EXPECT_EQ(lexa.tokens.size(), total_count);
        EXPECT_EQ(lexa.tokens_by_code[(Obelix::TokenCode)big].size(), big_count);
        EXPECT_EQ(lexa.tokens_by_code[(Obelix::TokenCode)bad].size(), bad_count);
    }
};

void addScanners(Obelix::Lexa& lexa)
{
    lexa.add_scanner<Obelix::QStringScanner>();
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
}

unsigned int prepareWithBig(Obelix::Lexa& lexa)
{
    lexa.add_scanner<Obelix::KeywordScanner>(1, "Big");
    return 100;
}

void prepareWithBigBad(Obelix::Lexa& lexa, unsigned int* big, unsigned int* bad)
{
    lexa.add_scanner<Obelix::KeywordScanner>(2, "Big", "Bad");
    *big = 100;
    *bad = 101;
}

unsigned int prepareWithAbc(Obelix::Lexa& lexa)
{
    lexa.add_scanner<Obelix::KeywordScanner>(9,
        "abb",
        "aca",
        "aba",
        "aaa",
        "aab",
        "abc",
        "aac",
        "acc",
        "acb");
    return 105;
}

void tokenize(Obelix::Lexa& lexa, const char* s, int total_count, int big_count)
{
    unsigned int code;

    code = prepareWithBig(lexa);
    lexa.tokenize(s);
    EXPECT_EQ(lexa.tokens.size(), total_count);
    for (auto& token : lexa.tokens) {
        fprintf(stderr, "%s\n", token.to_string().c_str());
    }
    EXPECT_EQ(lexa.tokens_by_code[(Obelix::TokenCode)code].size(), big_count);
}

void tokenizeBigBad(Obelix::Lexa& lexa, const char* s, int total_count, int big_count, int bad_count)
{
    unsigned int big;
    unsigned int bad;

    prepareWithBigBad(lexa, &big, &bad);
    lexa.tokenize(s);
    EXPECT_EQ(lexa.tokens.size(), total_count);
    EXPECT_EQ(lexa.tokens_by_code[(Obelix::TokenCode)big].size(), big_count);
    EXPECT_EQ(lexa.tokens_by_code[(Obelix::TokenCode)bad].size(), bad_count);
}

class KeywordTestDebug : public KeywordTest {
    bool debugOn() override
    {
        return true;
    }
};

TEST_F(KeywordTest, Keyword)
{
    tokenize("Big", 2, 1);
}

TEST_F(KeywordTest, KeywordSpace)
{
    tokenize("Big ", 3, 1);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 1);
}

TEST_F(KeywordTest, KeywordIsPrefix)
{
    tokenize("Bigger", 3, 1);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 1);
}

TEST_F(KeywordTest, KeywordAndIdentifiers)
{
    tokenize("Hello Big World", 6, 1);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

TEST_F(KeywordTest, TwoKeywords)
{
    tokenize("Hello Big Big Beautiful World", 10, 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 3);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 4);
}

TEST_F(KeywordTest, keyword_two_keywords_separated)
{
    tokenize("Hello Big Beautiful Big World", 10, 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 3);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 4);
}

TEST_F(KeywordTest, keyword_big_bad_big)
{
    tokenizeBigBad("Hello Big World", 6, 1, 0);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

TEST_F(KeywordTest, keyword_big_bad_bad)
{
    tokenizeBigBad("Hello Bad World", 6, 0, 1);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

TEST_F(KeywordTest, keyword_big_bad_big_bad)
{
    tokenizeBigBad("Hello Big Bad World", 8, 1, 1);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 3);
}

TEST_F(KeywordTest, keyword_big_bad_bad_big)
{
    tokenizeBigBad("Hello Bad Big World", 8, 1, 1);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 3);
}

TEST_F(KeywordTestDebug, keyword_abc)
{
    unsigned int abc = prepareWithAbc();

    lexa.tokenize("yyz abc ams");
    EXPECT_EQ(lexa.tokens.size(), 6);
    EXPECT_EQ(lexa.tokens_by_code[(Obelix::TokenCode)abc].size(), 1);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Identifier].size(), 2);
    EXPECT_EQ(lexa.tokens_by_code[Obelix::TokenCode::Whitespace].size(), 2);
}

}