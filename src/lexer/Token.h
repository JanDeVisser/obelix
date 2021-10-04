//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include "core/Object.h"
#include <optional>
#include <string>

#define ENUMERATE_TOKEN_TYPES(S)       \
    S(EndOfFile, -1, nullptr)          \
    S(Unknown, -1, nullptr)            \
    S(Whitespace, ' ', nullptr)        \
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
    S(Number, -1, nullptr)             \
    S(Identifier, -1, nullptr)         \
    S(DoubleQuotedString, -1, nullptr) \
    S(SingleQuotedString, -1, nullptr) \
    S(BackQuotedString, -1, nullptr)

enum class TokenType {
#undef __ENUMERATE_TOKEN_TYPE
#define __ENUMERATE_TOKEN_TYPE(type, c, str) type,
    ENUMERATE_TOKEN_TYPES(__ENUMERATE_TOKEN_TYPE)
#undef __ENUMERATE_TOKEN_TYPE
};

std::string TokenType_name(TokenType);

class Token {
public:
    explicit Token(TokenType type, std::string value = "")
        : m_type(type)
        , m_value(move(value))
    {
    }

    [[nodiscard]] TokenType type() const { return m_type; }
    [[nodiscard]] std::string const& value() const { return m_value; }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::optional<int> to_number() const;
    [[nodiscard]] std::shared_ptr<Object> to_object() const;

private:
    TokenType m_type { TokenType::Unknown };
    std::string m_value { };
};
