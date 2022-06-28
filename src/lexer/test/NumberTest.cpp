/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "lexer/Token.h"
#include <core/Format.h>
#include <core/Object.h>
#include <gtest/gtest.h>
#include <iostream>
#include <lexer/Tokenizer.h>
#include <lexer/test/LexerTest.h>

class NumberTest : public LexerTest {
public:
    template <typename T>
    void check_number(std::string const& in, T const& out, Obelix::TokenCode code)
    {
        add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, true, true });
        add_scanner<Obelix::IdentifierScanner>();
        add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
        tokenize(Obelix::format("Foo = {}", in));
        check_codes(6,
            Obelix::TokenCode::Identifier,
            Obelix::TokenCode::Whitespace,
            Obelix::TokenCode::Equals,
            Obelix::TokenCode::Whitespace,
            code,
            Obelix::TokenCode::EndOfFile);
        EXPECT_EQ(token_value<T>(tokens[4]), out);
    }

    bool debugOn() override
    {
        return false;
    }
};

TEST_F(NumberTest, number_integer)
{
    check_number<int>("1", 1, Obelix::TokenCode::Integer);
}

TEST_F(NumberTest, number_float)
{
    check_number<double>("3.14", 3.14, Obelix::TokenCode::Float);
}

TEST_F(NumberTest, number_hex)
{
    check_number<long>("0xDEADC0DE", 3735929054, Obelix::TokenCode::HexNumber);
}

TEST_F(NumberTest, number_dollar_hex)
{
    check_number<long>("$DEADC0DE", 3735929054, Obelix::TokenCode::HexNumber);
}

TEST_F(NumberTest, double_period)
{
    add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, true, true });
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::KeywordScanner>(Obelix::Token(Obelix::TokenCode::Keyword18, ".."));
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    tokenize("0..10");

    check_codes(4,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::Keyword18,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(token_value<int>(tokens[0]), 0);
    EXPECT_EQ(token_value<int>(tokens[2]), 10);
}