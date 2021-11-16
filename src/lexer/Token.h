//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

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
                                         \
    S(Integer, nullptr, nullptr)         \
    S(HexNumber, nullptr, nullptr)       \
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
    S(Keyword29, nullptr, nullptr)

enum class TokenCode {
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str) code,
    ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
};

constexpr TokenCode TokenCode_by_char(int ch)
{
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str)          \
    {                                                 \
        auto c_str = static_cast<char const*>(c);     \
        if ((c_str != nullptr) && (ch == c_str[0])) { \
            return TokenCode::code;                   \
        }                                             \
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
    [[nodiscard]] Ptr<Object> to_object() const;
    [[nodiscard]] int compare(Token const& other) const;
    [[nodiscard]] bool is_whitespace() const;

    Span location { 0, 0, 0, 0 };

private:
    TokenCode m_code { TokenCode::Unknown };
    std::string m_value {};
    std::string m_token {};
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
