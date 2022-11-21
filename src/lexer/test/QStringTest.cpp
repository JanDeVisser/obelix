/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-09-22.
//

#include <gtest/gtest.h>
#include <lexer/Tokenizer.h>
#include <lexer/test/LexerTest.h>

class QStringTest : public LexerTest {
public:
    void check_qstring(std::string const& in, std::string const& out)
    {
        add_scanner<Obelix::QStringScanner>();
        add_scanner<Obelix::IdentifierScanner>();
        add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
        tokenize("Hello " + in);
        check_codes(4,
            Obelix::TokenCode::Identifier,
            Obelix::TokenCode::Whitespace,
            Obelix::TokenCode::SingleQuotedString,
            Obelix::TokenCode::EndOfFile);
        EXPECT_EQ(tokens[2].value(), out);
    }

    void check_qstring_error(std::string const& in)
    {
        add_scanner<Obelix::QStringScanner>();
        add_scanner<Obelix::IdentifierScanner>();
        add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
        tokenize("Hello " + in);
        check_codes(4,
            Obelix::TokenCode::Identifier,
            Obelix::TokenCode::Whitespace,
            Obelix::TokenCode::Error,
            Obelix::TokenCode::EndOfFile);
    }

    bool debugOn() override
    {
        return false;
    }
};

TEST_F(QStringTest, qstring)
{
    add_scanner<Obelix::QStringScanner>();
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    tokenize("Hello 'single quotes' `backticks` \"double quotes\" World");
    check_codes(10,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::SingleQuotedString,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::BackQuotedString,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::DoubleQuotedString,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);

    EXPECT_EQ(count_tokens_with_code(Obelix::TokenCode::Identifier), 2);
    EXPECT_EQ(count_tokens_with_code(Obelix::TokenCode::SingleQuotedString), 1);
    EXPECT_EQ(count_tokens_with_code(Obelix::TokenCode::DoubleQuotedString), 1);
    EXPECT_EQ(count_tokens_with_code(Obelix::TokenCode::BackQuotedString), 1);
    EXPECT_EQ(tokens[2].value(), "single quotes");
    EXPECT_EQ(tokens[4].value(), "backticks");
    EXPECT_EQ(tokens[6].value(), "double quotes");
}

TEST_F(QStringTest, qstring_unclosed_string)
{
    check_qstring_error("'no close quote");
}

TEST_F(QStringTest, qstring_escape_backslash)
{
    check_qstring("'escaped backslash \\\\'", "escaped backslash \\");
}

TEST_F(QStringTest, qstring_escape_quote)
{
    check_qstring("'escaped quote\\''", "escaped quote'");
}

TEST_F(QStringTest, qstring_escape_newline)
{
    check_qstring("'escaped\\nnewline'", "escaped\nnewline");
}

TEST_F(QStringTest, qstring_escape_plain_char)
{
    check_qstring("'escaped \\$ dollarsign'", "escaped $ dollarsign");
}

TEST_F(QStringTest, qstring_escape_as_last_char)
{
    check_qstring_error("'escape \\");
}
