/*
 * NumberTest.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/Format.h>
#include <core/Object.h>
#include <gtest/gtest.h>
#include <lexer/Tokenizer.h>
#include <lexer/test/LexerTest.h>

class NumberTest : public LexerTest {
public:
    void check_number(std::string const& in, Obelix::Obj out, Obelix::TokenCode code)
    {
        add_scanner<Obelix::NumberScanner>(Obelix::NumberScanner::Config { true, false, true, true });
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
        EXPECT_EQ(tokens[4].to_object(), out);
    }

    bool debugOn() override
    {
        return false;
    }
};

TEST_F(NumberTest, number_integer)
{
    check_number("1", Obelix::make_obj<Obelix::Integer>(1), Obelix::TokenCode::Integer);
}

TEST_F(NumberTest, number_float)
{
    check_number("3.14", Obelix::make_obj<Obelix::Float>(3.14), Obelix::TokenCode::Float);
}

TEST_F(NumberTest, number_hex)
{
    check_number("0xDEADC0DE", Obelix::make_obj<Obelix::Integer>(3735929054), Obelix::TokenCode::HexNumber);
}
