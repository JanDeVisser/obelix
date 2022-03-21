/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Format.h>
#include <core/Object.h>
#include <gtest/gtest.h>
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
        EXPECT_EQ(tokens[4].token_value<T>(), out);
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
