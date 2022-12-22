/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <cstring>
#include <optional>
#include <string>

#include <core/Error.h>
#include <core/Format.h>
#include <core/Logging.h>

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
        count
};

constexpr TokenCode TokenCode_by_char(int ch)
{
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str)                                    \
    {                                                                           \
        if (c != nullptr) {                                                     \
            char const* c_str = c;                                              \
            if ((c_str != nullptr) && (ch == c_str[0]) && (c_str[1] == '\0')) { \
                return TokenCode::code;                                         \
            }                                                                   \
        }                                                                       \
    }
    ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
    return TokenCode::Unknown;
}

inline TokenCode TokenCode_by_string(char const* str)
{
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, s)   \
    {                                        \
        if (c != nullptr) {                  \
            char const* c_str = c;           \
            if (str != nullptr) {            \
                if (strcmp(str, c_str) == 0) \
                    return TokenCode::code;  \
            }                                \
        }                                    \
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

template<>
struct Converter<TokenCode> {
    static std::string to_string(TokenCode val)
    {
        return TokenCode_name(val);
    }

    static double to_double(TokenCode val)
    {
        return static_cast<double>(val);
    }

    static long to_long(TokenCode val)
    {
        return static_cast<long>(val);
    }
};

struct Span {
    std::string file_name;
    size_t start_line;
    size_t start_column;
    size_t end_line;
    size_t end_column;

    [[nodiscard]] std::string to_string() const
    {
        if (!empty()) {
            return (!file_name.empty())
                ? format("{}:{}:{}-{}:{}:", file_name, start_line, start_column, end_line, end_column)
                : format("{}:{}-{}:{}", start_line, start_column, end_line, end_column);
        } else {
            return format("{}:", file_name);
        }
    }

    [[nodiscard]] Span merge(Span const&) const;
    [[nodiscard]] bool empty() const
    {
        return (start_line == end_line) && (start_column == end_column);
    }

    bool operator==(Span const& other) const
    {
        return file_name == other.file_name && start_line == other.start_line && end_line == other.end_line && start_column == other.start_column && end_column == other.end_column;
    }
};

class Token {
public:
    Token() = default;
    explicit Token(TokenCode code, std::string value = "")
        : m_code(code)
        , m_value(std::move(value))
    {
    }

    explicit Token(int code, std::string value = "")
        : Token((TokenCode)code, std::move(value))
    {
    }

    Token(Span location, TokenCode code, std::string value = "")
        : m_location(std::move(location))
        , m_code(code)
        , m_value(std::move(value))
    {
    }

    Token(Span location, int code, std::string value = "")
        : Token(std::move(location), (TokenCode)code, std::move(value))
    {
    }

    [[nodiscard]] Span const& location() const { return m_location; }
    void location(Span location) { m_location = std::move(location); }
    [[nodiscard]] TokenCode code() const { return m_code; }
    [[nodiscard]] std::string code_name() const { return TokenCode_name(code()); }
    [[nodiscard]] std::string const& value() const { return m_value; }
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] std::optional<long> to_long() const;
    [[nodiscard]] std::optional<double> to_double() const;
    [[nodiscard]] std::optional<bool> to_bool() const;
    [[nodiscard]] int compare(Token const& other) const;
    [[nodiscard]] bool is_whitespace() const;

private:
    Span m_location { {}, 0, 0, 0, 0 };
    TokenCode m_code { TokenCode::Unknown };
    std::string m_value {};
};

class SyntaxError {
public:
    SyntaxError(Span location, std::string msg)
        : m_location(std::move(location))
        , m_message(std::move(msg))
    {
    }

    template<typename... Args>
    SyntaxError(Span location, std::string message, Args&&... args)
        : m_location(std::move(location))
    {
        m_message = format(message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    SyntaxError(Span location, ErrorCode code, Args&&... args)
        : m_location(std::move(location))
    {
        m_message = format(ErrorCode_message(code), std::forward<Args>(args)...);
    }

    SyntaxError(std::string msg)
        : m_message(std::move(msg))
    {
    }

    template<typename... Args>
    SyntaxError(std::string message, Args&&... args)
    {
        m_message = format(message, std::forward<Args>(args)...);
    }

    template<typename... Args>
    SyntaxError(ErrorCode code, Args&&... args)
    {
        m_message = format(ErrorCode_message(code), std::forward<Args>(args)...);
    }

    [[nodiscard]] std::string const& message() const { return m_message; }
    [[nodiscard]] Span const& location() const { return m_location; }

    [[nodiscard]] std::string to_string() const
    {
        return format("{} {}", m_location, m_message);
    }

    bool operator==(SyntaxError const& other) const
    {
        return location() == other.m_location && m_message == other.m_message;
    }

private:
    Span m_location {};
    std::string m_message;
};

template<typename T>
inline ErrorOr<T, SyntaxError> token_value(Token const& token)
{
    fatal("Specialize token_value<T>(Token const&) for T = {}", typeid(T).name());
}

template<std::convertible_to<long> T>
inline ErrorOr<T, SyntaxError> token_value(Token const& token)
{
    if (token.code() != TokenCode::Float && token.code() != TokenCode::Integer && token.code() != TokenCode::HexNumber)
        return SyntaxError { token.location(), "Cannot get {} value as {}", token.code(), typeid(T).name() };
    auto v = to_long_unconditional(token.value());
    if (v < std::numeric_limits<T>::min() || v > std::numeric_limits<T>::max())
        return SyntaxError { token.location(), "Long value {} overflows {}", v, typeid(T).name() };
    return static_cast<T>(v);
}

template<>
inline ErrorOr<std::string, SyntaxError> token_value(Token const& token)
{
    return token.value();
}

template<>
inline ErrorOr<bool, SyntaxError> token_value(Token const& token)
{
    auto number_maybe = token.to_long();
    if (number_maybe.has_value())
        return number_maybe.value() != 0;
    auto bool_maybe = Obelix::to_bool(token.value());
    if (bool_maybe.has_value())
        return bool_maybe.value();
    return SyntaxError { token.location(), "Cannot convert get {} with value {} as bool", token.code(), token.value() };
}

template<>
struct Converter<Token> {
    static std::string to_string(Token const& t)
    {
        return format("{}:{}", t.location().to_string(), t.to_string());
    }

    static double to_double(Token const&)
    {
        fatal("Can't convert Token to double");
    }

    static long to_long(Token const&)
    {
        fatal("Can't convert Token to long");
    }
};

}
