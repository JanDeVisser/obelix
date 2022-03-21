/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include "core/StringUtil.h"
#include <core/Object.h>
#include <optional>
#include <string>

namespace Obelix {

#define ENUMERATE_TOKEN_CODES(S)         \
    S(Unknown, nullptr, nullptr)         \
    S(EndOfFile, nullptr, nullptr)       \
    S(Error, nullptr, nullptr)           \
    S(Comment, nullptr, nullptr)         \
    S(Whitespace, " ", nullptr)          \
    S(NewLine, nullptr, nullptr)         \
    S(Plus, "+", nullptr)                \
    S(Minus, "-", nullptr)               \
    S(Slash, "/", nullptr)               \
    S(Backslash, "\\", nullptr)          \
    S(Asterisk, "*", nullptr)            \
    S(OpenParen, "(", nullptr)           \
    S(CloseParen, ")", nullptr)          \
    S(OpenBrace, "{", nullptr)           \
    S(CloseBrace, "}", nullptr)          \
    S(OpenBracket, "[", nullptr)         \
    S(CloseBracket, "]", nullptr)        \
    S(ExclamationPoint, "!", nullptr)    \
    S(QuestionMark, "?", nullptr)        \
    S(AtSign, "@", nullptr)              \
    S(Pound, "#", nullptr)               \
    S(Dollar, "$", nullptr)              \
    S(Percent, "%", nullptr)             \
    S(Ampersand, "&", nullptr)           \
    S(Hat, "^", nullptr)                 \
    S(UnderScore, "_", nullptr)          \
    S(Equals, "=", nullptr)              \
    S(Pipe, "|", nullptr)                \
    S(Colon, ":", nullptr)               \
    S(LessThan, "<", nullptr)            \
    S(GreaterThan, ">", nullptr)         \
    S(Comma, ",", nullptr)               \
    S(Period, ".", nullptr)              \
    S(SemiColon, ";", nullptr)           \
    S(Tilde, "~", nullptr)               \
                                         \
    S(LessEqualThan, nullptr, "<=")      \
    S(GreaterEqualThan, nullptr, ">=")   \
    S(EqualsTo, nullptr, "==")           \
    S(NotEqualTo, nullptr, "!=")         \
    S(LogicalAnd, nullptr, "&&")         \
    S(LogicalOr, nullptr, "||")          \
    S(ShiftLeft, nullptr, "<<")          \
    S(ShiftRight, nullptr, ">>")         \
    S(BinaryIncrement, nullptr, "+=")    \
    S(BinaryDecrement, nullptr, "+=")    \
    S(UnaryIncrement, nullptr, "++")     \
    S(UnaryDecrement, nullptr, "--")     \
                                         \
    S(Integer, nullptr, nullptr)         \
    S(HexNumber, nullptr, nullptr)       \
    S(BinaryNumber, nullptr, nullptr)    \
    S(Float, nullptr, nullptr)           \
    S(Identifier, nullptr, nullptr)      \
    S(DoubleQuotedString, "\"", nullptr) \
    S(SingleQuotedString, "'", nullptr)  \
    S(BackQuotedString, "`", nullptr)    \
                                         \
    S(Keyword0, nullptr, nullptr)        \
    S(Keyword1, nullptr, nullptr)        \
    S(Keyword2, nullptr, nullptr)        \
    S(Keyword3, nullptr, nullptr)        \
    S(Keyword4, nullptr, nullptr)        \
    S(Keyword5, nullptr, nullptr)        \
    S(Keyword6, nullptr, nullptr)        \
    S(Keyword7, nullptr, nullptr)        \
    S(Keyword8, nullptr, nullptr)        \
    S(Keyword9, nullptr, nullptr)        \
    S(Keyword10, nullptr, nullptr)       \
    S(Keyword11, nullptr, nullptr)       \
    S(Keyword12, nullptr, nullptr)       \
    S(Keyword13, nullptr, nullptr)       \
    S(Keyword14, nullptr, nullptr)       \
    S(Keyword15, nullptr, nullptr)       \
    S(Keyword16, nullptr, nullptr)       \
    S(Keyword17, nullptr, nullptr)       \
    S(Keyword18, nullptr, nullptr)       \
    S(Keyword19, nullptr, nullptr)       \
    S(Keyword20, nullptr, nullptr)       \
    S(Keyword21, nullptr, nullptr)       \
    S(Keyword22, nullptr, nullptr)       \
    S(Keyword23, nullptr, nullptr)       \
    S(Keyword24, nullptr, nullptr)       \
    S(Keyword25, nullptr, nullptr)       \
    S(Keyword26, nullptr, nullptr)       \
    S(Keyword27, nullptr, nullptr)       \
    S(Keyword28, nullptr, nullptr)       \
    S(Keyword29, nullptr, nullptr)       \
    S(Keyword30, nullptr, nullptr)       \
    S(Keyword31, nullptr, nullptr)       \
    S(Keyword32, nullptr, nullptr)       \
    S(Keyword33, nullptr, nullptr)       \
    S(Keyword34, nullptr, nullptr)       \
    S(Keyword35, nullptr, nullptr)       \
    S(Keyword36, nullptr, nullptr)       \
    S(Keyword37, nullptr, nullptr)       \
    S(Keyword38, nullptr, nullptr)       \
    S(Keyword39, nullptr, nullptr)       \
    S(Keyword40, nullptr, nullptr)       \
    S(Keyword41, nullptr, nullptr)       \
    S(Keyword42, nullptr, nullptr)       \
    S(Keyword43, nullptr, nullptr)       \
    S(Keyword44, nullptr, nullptr)       \
    S(Keyword45, nullptr, nullptr)       \
    S(Keyword46, nullptr, nullptr)       \
    S(Keyword47, nullptr, nullptr)       \
    S(Keyword48, nullptr, nullptr)       \
    S(Keyword49, nullptr, nullptr)       \
    S(Keyword50, nullptr, nullptr)       \
    S(Keyword51, nullptr, nullptr)       \
    S(Keyword52, nullptr, nullptr)       \
    S(Keyword53, nullptr, nullptr)       \
    S(Keyword54, nullptr, nullptr)       \
    S(Keyword55, nullptr, nullptr)       \
    S(Keyword56, nullptr, nullptr)       \
    S(Keyword57, nullptr, nullptr)       \
    S(Keyword58, nullptr, nullptr)       \
    S(Keyword59, nullptr, nullptr)

enum class TokenCode {
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str) code,
    ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
};

constexpr TokenCode TokenCode_by_char(int ch)
{
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str)                                \
    {                                                                       \
        auto c_str = static_cast<char const*>(c);                           \
        if ((c_str != nullptr) && (ch == c_str[0]) && (c_str[1] == '\0')) { \
            return TokenCode::code;                                         \
        }                                                                   \
    }
    ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
    return TokenCode::Unknown;
}

inline TokenCode TokenCode_by_string(char const* str)
{
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str)                   \
    {                                                          \
        auto c_str = static_cast<char const*>(c);              \
        if ((c_str != nullptr) && (strcmp(str, c_str) == 0)) { \
            return TokenCode::code;                            \
        }                                                      \
    }
    ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
    return TokenCode::Unknown;
}

constexpr char const* TokenCode_to_string(TokenCode code)
{
    switch (code) {
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str) \
    case TokenCode::code:                    \
        return (c != nullptr) ? c : str;
        ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
    default:
        return "Custom";
    }
}

std::string TokenCode_name(TokenCode);

struct Span {
    size_t start_line;
    size_t start_column;
    size_t end_line;
    size_t end_column;

    [[nodiscard]] std::string to_string() const
    {
        return format("{}:{} - {}:{}", start_line, start_column, end_line, end_column);
    }
};

class Token {
public:
    Token() = default;
    explicit Token(TokenCode code, std::string value = "")
        : m_code(code)
        , m_value(move(value))
    {
    }

    explicit Token(int code, std::string value = "")
        : Token((TokenCode)code, move(value))
    {
    }

    [[nodiscard]] TokenCode code() const { return m_code; }
    [[nodiscard]] std::string code_name() const { return TokenCode_name(code()); }
    [[nodiscard]] std::string const& value() const { return m_value; }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::optional<long> to_long() const;
    [[nodiscard]] std::optional<double> to_double() const;
    [[nodiscard]] std::optional<bool> to_bool() const;
    [[nodiscard]] int compare(Token const& other) const;
    [[nodiscard]] bool is_whitespace() const;

    template <typename T>
    T token_value() const
    {
        fatal("Specialize Token::token_value for type '{}'", typeid(T).name());
    }

    template <>
    std::string const& token_value() const
    {
        return m_value;
    }

    template <>
    int token_value() const
    {
        assert(m_code == TokenCode::Float || m_code == TokenCode::Integer || m_code == TokenCode::HexNumber);
        return to_long_unconditional(m_value);
    }

    template <>
    long token_value() const
    {
        assert(m_code == TokenCode::Float || m_code == TokenCode::Integer || m_code == TokenCode::HexNumber);
        return to_long_unconditional(m_value);
    }

    template <>
    double token_value() const
    {
        assert(m_code == TokenCode::Float || m_code == TokenCode::Integer || m_code == TokenCode::HexNumber);
        return to_long_unconditional(m_value);
    }

    template <>
    bool token_value() const
    {
        auto number_maybe = to_long();
        if (number_maybe.has_value())
            return number_maybe.value() != 0;
        return Obelix::to_bool_unconditional(m_value);
    }

    Span location { 0, 0, 0, 0 };

private:
    TokenCode m_code { TokenCode::Unknown };
    std::string m_value {};
};

template<>
struct Converter<Token> {
    static std::string to_string(Token t)
    {
        return format("{}:{}", t.location.to_string(), t.to_string());
    }

    static double to_double(Token t)
    {
        fatal("Can't convert Token to double");
    }

    static long to_long(Token t)
    {
        fatal("Can't convert Token to long");
    }
};

}
