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

TEST(TokenizerTest, tokenizer_create)
{
    Obelix::Tokenizer tokenizer("1 + 2 + a");
    tokenizer.add_scanner<Obelix::NumberScanner>();
    tokenizer.add_scanner<Obelix::IdentifierScanner>();
    tokenizer.add_scanner<Obelix::WhitespaceScanner>();
    EXPECT_EQ(tokenizer.state(), Obelix::TokenizerState::Fresh);
}

TEST_F(LexerTest, lexer_lex)
{
    add_scanner<Obelix::NumberScanner>();
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>();
    tokenize("1 + 2 + a");
    check_codes(6,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::Plus,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::Plus,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(tokens[4].value(), "a");
}
