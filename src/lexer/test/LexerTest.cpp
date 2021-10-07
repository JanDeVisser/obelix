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

TEST(LexerTest, lexer_lex_with_whitespace)
{
    Obelix::Lexa lexa("1 + 2 + a");
    lexa.add_scanner<Obelix::NumberScanner>();
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    lexa.tokenize();
    lexa.check_codes(10,
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

TEST(LexerTest, lexer_whitespace_newline)
{
    Obelix::Lexa lexa("Hello  World\nSecond Line");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    lexa.tokenize();
    lexa.check_codes(8,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(lexa.tokens[3].value(), "\n");
}

TEST(LexerTest, Symbols)
{
    Obelix::Lexa lexa("Hello !@ /\\ * && World");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(true);
    lexa.tokenize();
    lexa.check_codes(10,
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
    EXPECT_EQ(lexa.tokens[8].value(), "World");
}

TEST(LexerTest, TrailingWhitespace)
{
    Obelix::Lexa lexa("Hello  World  \nSecond Line");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    lexa.tokenize();
    lexa.check_codes(9,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::NewLine,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(lexa.tokens[3].value(), "  ");
}

TEST(LexerTest, IgnoreWS)
{
    Obelix::Lexa lexa(" Hello  World\nSecond Line \n Third Line ");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, true, false });
    lexa.tokenize();
    lexa.check_codes(9,
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

TEST(LexerTest, IgnoreNL)
{
    Obelix::Lexa lexa(" Hello  World\nSecond Line \n Third Line ");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, false, false });
    lexa.tokenize();
    lexa.check_codes(14,
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

TEST(LexerTest, IgnoreAllWS_newlines_are_not_spaces)
{
    Obelix::Lexa lexa(" Hello  World\nSecond Line \n Third Line ");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
    lexa.tokenize();
    lexa.check_codes(7,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
}

TEST(LexerTest, IgnoreAllWS_newlines_are_spaces)
{
    Obelix::Lexa lexa(" Hello  World\nSecond Line \n Third Line ");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, true });
    lexa.tokenize();
    lexa.check_codes(7,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::EndOfFile);
}

TEST(LexerTest, IgnoreNoWhitespace)
{
    Obelix::Lexa lexa(" Hello  World\nSecond Line \n Third Line ");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, false });
    lexa.tokenize();
    lexa.check_codes(16,
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

TEST(LexerTest, IgnoreNoWhitespace_newlines_are_spaces)
{
    Obelix::Lexa lexa(" Hello  World\nSecond Line \n Third Line ");
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false, true });
    lexa.tokenize();
    lexa.check_codes(14,
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
    EXPECT_EQ(lexa.tokens[8].value(), " \n ");

}
