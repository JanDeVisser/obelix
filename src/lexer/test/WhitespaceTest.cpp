/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-10-07.
//

//
// Created by Jan de Visser on 2021-09-22.
//

#include <gtest/gtest.h>
#include <lexer/Tokenizer.h>
#include <lexer/test/LexerTest.h>

TEST_F(LexerTest, tokenizer_lex_with_whitespace)
{
    add_scanner<Obelix::NumberScanner>();
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    tokenize("1 + 2 + a");
    check_codes(10,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Plus,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Plus,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
}

TEST_F(LexerTest, tokenizer_whitespace_newline)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    tokenize("Hello  World\nSecond Line");
    check_codes(8,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(tokens[3].value(), "\n");
}

TEST_F(LexerTest, Symbols)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(true);
    tokenize("Hello !@ /\\ * && World");
    check_codes(10,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::ExclamationPoint,
        Obelix::TokenCode::AtSign,
        Obelix::TokenCode::Slash,
        Obelix::TokenCode::Backslash,
        Obelix::TokenCode::Asterisk,
        Obelix::TokenCode::Ampersand,
        Obelix::TokenCode::Ampersand,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(tokens[8].value(), "World");
}

TEST_F(LexerTest, TrailingWhitespace)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    tokenize("Hello  World  \nSecond Line");
    check_codes(9,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(tokens[3].value(), "  ");
}

TEST_F(LexerTest, IgnoreWS)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, true, false });
    tokenize(" Hello  World\nSecond Line \n Third Line ");
    check_codes(9,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
}

TEST_F(LexerTest, IgnoreNL)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, false, false });
    tokenize(" Hello  World\nSecond Line \n Third Line ");
    check_codes(14,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::EndOfFile);
}

TEST_F(LexerTest, IgnoreAllWS_newlines_are_not_spaces)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
    tokenize(" Hello  World\nSecond Line \n Third Line ");
    check_codes(7,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
}

TEST_F(LexerTest, IgnoreAllWS_newlines_are_spaces)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, true });
    tokenize(" Hello  World\nSecond Line \n Third Line ");
    check_codes(7,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
}

TEST_F(LexerTest, IgnoreNoWhitespace)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    tokenize(" Hello  World\nSecond Line \n Third Line ");
    check_codes(16,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::EndOfFile);
}

TEST_F(LexerTest, IgnoreNoWhitespace_newlines_are_spaces)
{
    add_scanner<Obelix::IdentifierScanner>();
    add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, true });
    tokenize(" Hello  World\nSecond Line \n Third Line ");
    check_codes(14,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(tokens[8].value(), " \n ");

}
