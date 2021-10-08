//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <core/Object.h>
#include <optional>
#include <string>

namespace Obelix {

#define ENUMERATE_TOKEN_CODES(S)       \
    S(EndOfFile, -1, nullptr)          \
    S(Error, -1, nullptr)              \
    S(Unknown, -1, nullptr)            \
    S(Whitespace, ' ', nullptr)        \
    S(NewLine, -1, nullptr)            \
    S(Plus, '+', nullptr)              \
    S(Minus, '-', nullptr)             \
    S(Slash, '/', nullptr)             \
    S(Backslash, '\\', nullptr)        \
    S(Asterisk, '*', nullptr)          \
    S(OpenParen, '(', nullptr)         \
    S(CloseParen, ')', nullptr)        \
    S(OpenBrace, '{', nullptr)         \
    S(CloseBrace, '}', nullptr)        \
    S(OpenBracket, '[', nullptr)       \
    S(CloseBracket, ']', nullptr)      \
    S(ExclamationPoint, '!', nullptr)  \
    S(QuestionMark, '?', nullptr)      \
    S(AtSign, '@', nullptr)            \
    S(Pound, '#', nullptr)             \
    S(Dollar, '$', nullptr)            \
    S(Percent, '%', nullptr)           \
    S(Ampersand, '&', nullptr)         \
    S(UnderScore, '_', nullptr)        \
    S(Equals, '=', nullptr)            \
    S(Pipe, '|', nullptr)              \
    S(Colon, ':', nullptr)             \
    S(LessEqualThan, -1, "<=")         \
    S(GreaterEqualThan, -1, ">=")      \
    S(LessThan, '<', nullptr)          \
    S(GreaterThan, '>', nullptr)       \
    S(Comma, ',', nullptr)             \
    S(Period, '.', nullptr)            \
    S(SemiColon, ';', nullptr)         \
    S(Tilde, '~', nullptr)             \
                                       \
    S(Integer, -1, nullptr)            \
    S(HexNumber, -1, nullptr)          \
    S(Float, -1, nullptr)              \
    S(Identifier, -1, nullptr)         \
    S(DoubleQuotedString, '"', nullptr) \
    S(SingleQuotedString, '\'', nullptr) \
    S(BackQuotedString, '`', nullptr)

enum class TokenCode {
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str) code,
    ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
};

constexpr char const* TokenCode_name(TokenCode t)
{
    switch (t) {
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str) \
    case TokenCode::code:                    \
        return #code;
        ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
    default:
        return "Custom";
    }
}

constexpr TokenCode TokenCode_by_char(int ch)
{
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str) \
    if (ch == c) {                           \
        return TokenCode::code;              \
    }
    ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
    return TokenCode::Unknown;
}


class Token {
public:
    Token() = default;
    explicit Token(TokenCode code, std::string value = "")
        : m_code(code)
        , m_value(move(value))
    {
    }

    explicit Token(int code, std::string value = "")
        : Token((TokenCode) code, move(value))
    {
    }

    [[nodiscard]] TokenCode code() const { return m_code; }
    [[nodiscard]] std::string code_name() const { return TokenCode_name(code()); }
    [[nodiscard]] std::string const& value() const { return m_value; }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::optional<long> to_long() const;
    [[nodiscard]] std::optional<double> to_double() const;
    [[nodiscard]] std::optional<bool> to_bool() const;
    [[nodiscard]] Ptr<Object> to_object() const;
    [[nodiscard]] int compare(Token const& other) const;
    [[nodiscard]] bool is_whitespace() const;

    size_t line { 0 };
    size_t column { 0 };
private:
    TokenCode m_code { TokenCode::Unknown };
    std::string m_value {};
    std::string m_token {};
};

}
