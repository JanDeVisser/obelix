//
// Created by Jan de Visser on 2021-09-22.
//

#include <gtest/gtest.h>
#include <lexer/Lexer.h>
#include <lexer/test/Lexa.h>

TEST(LexerTest, lexer_create)
{
    Obelix::Lexer lexer("1 + 2 + a");
    lexer.add_scanner<Obelix::NumberScanner>();
    lexer.add_scanner<Obelix::IdentifierScanner>();
    lexer.add_scanner<Obelix::WhitespaceScanner>();
    EXPECT_EQ(lexer.state(), Obelix::LexerState::Fresh);
}

TEST(LexerTest, lexer_lex)
{
    Obelix::Lexa lexa("1 + 2 + a");
    lexa.add_scanner<Obelix::NumberScanner>();
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>();
    lexa.tokenize();
    lexa.check_codes(6,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::Plus,
        Obelix::TokenCode::Integer,
        Obelix::TokenCode::Plus,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(lexa.tokens[4].value(), "a");
}
