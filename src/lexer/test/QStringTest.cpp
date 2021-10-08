//
// Created by Jan de Visser on 2021-09-22.
//

#include <gtest/gtest.h>
#include <lexer/Lexer.h>
#include <lexer/test/Lexa.h>

static void check_qstring(std::string const& in, std::string const& out)
{
    std::string s = "Hello " + in;
    Obelix::Lexa lexa(s.c_str());
    lexa.add_scanner<Obelix::QStringScanner>();
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    lexa.tokenize();
    lexa.check_codes(4,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::SingleQuotedString,
        Obelix::TokenCode::EndOfFile);
    EXPECT_EQ(lexa.tokens[2].value(), out);
}

static void check_qstring_error(std::string const& in)
{
    Obelix::Lexa lexa(("Hello " + in).c_str());
    lexa.add_scanner<Obelix::QStringScanner>();
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    lexa.tokenize();
    lexa.check_codes(4,
        Obelix::TokenCode::Identifier,
        Obelix::TokenCode::Whitespace,
        Obelix::TokenCode::Error,
        Obelix::TokenCode::EndOfFile);
}

TEST(QStringTest, qstring)
{
    Obelix::Lexa lexa("Hello 'single quotes' `backticks` \"double quotes\" World");
    lexa.add_scanner<Obelix::QStringScanner>();
    lexa.add_scanner<Obelix::IdentifierScanner>();
    lexa.add_scanner<Obelix::WhitespaceScanner>(Obelix::WhitespaceScanner::Config { false, false });
    lexa.tokenize();
    lexa.check_codes(10,
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

    EXPECT_EQ(lexa.count_tokens_with_code(Obelix::TokenCode::Identifier), 2);
    EXPECT_EQ(lexa.count_tokens_with_code(Obelix::TokenCode::SingleQuotedString), 1);
    EXPECT_EQ(lexa.count_tokens_with_code(Obelix::TokenCode::DoubleQuotedString), 1);
    EXPECT_EQ(lexa.count_tokens_with_code(Obelix::TokenCode::BackQuotedString), 1);
    EXPECT_EQ(lexa.tokens[2].value(), "single quotes");
    EXPECT_EQ(lexa.tokens[4].value(), "backticks");
    EXPECT_EQ(lexa.tokens[6].value(), "double quotes");
}

TEST(QStringTest, qstring_unclosed_string)
{
    check_qstring_error("'no close quote");
}

TEST(QStringTest, qstring_escapes)
{
    check_qstring("'escaped backslash \\\\'", "escaped backslash \\");
    check_qstring("'escaped quote\\''", "escaped quote'");
    check_qstring("'escaped\\nnewline'", "escaped\nnewline");
    check_qstring("'escaped \\$ dollarsign'", "escaped $ dollarsign");
    check_qstring_error("'escape \\");
}
