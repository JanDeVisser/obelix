//
// Created by Jan de Visser on 2021-09-18.
//

#include <core/Regex.h>
#include <lexer/Token.h>

namespace Obelix {

std::string TokenCode_name(TokenCode t)
{
    switch (t) {
#undef __ENUMERATE_TOKEN_CODE
#define __ENUMERATE_TOKEN_CODE(code, c, str) \
    case TokenCode::code:                    \
        if (str != nullptr)                  \
            return str;                      \
        return #code;
        ENUMERATE_TOKEN_CODES(__ENUMERATE_TOKEN_CODE)
#undef __ENUMERATE_TOKEN_CODE
    default:
        return format("Custom ({})", (int) t);
    }
}

std::string Token::to_string() const
{
    std::string ret = code_name();
    if (!m_value.empty()) {
        ret += " [" + value() + "]";
    }
    return ret;
}

std::optional<long> Token::to_long() const
{
    return Obelix::to_long(m_value);
}

std::optional<double> Token::to_double() const
{
    return Obelix::to_double(m_value);
}

std::optional<bool> Token::to_bool() const
{
    auto number_maybe = to_long();
    if (number_maybe.has_value())
        return number_maybe.value() != 0;
    return Obelix::to_bool(m_value);
}

Ptr<Object> Token::to_object() const
{
    switch (code()) {
    case TokenCode::Integer: {
        auto maybe_number = to_long();
        assert(maybe_number.has_value());
        return make_obj<Integer>(maybe_number.value());
    };
    case TokenCode::Float: {
        auto maybe_number = to_double();
        assert(maybe_number.has_value());
        return make_obj<Float>(maybe_number.value());
    };
    case TokenCode::DoubleQuotedString:
    case TokenCode::SingleQuotedString:
    case TokenCode::BackQuotedString:
        return make_obj<String>(m_value);
    case TokenCode::Slash:
        if (value() != "/") {
            return make_obj<Regex>(value());
        }
    default:
        if (value() == "true")
            return to_obj(Boolean::True());
        if (value() == "false")
            return to_obj(Boolean::False());
        if (value() == "null")
            return Object::null();
        break;
    }
    return make_obj<String>(code_name());
}

int Token::compare(Token const& other) const
{
    if (m_code == other.m_code)
        return m_value.compare(other.m_value);
    else
        return (int)m_code - (int)other.m_code;
}

bool Token::is_whitespace() const
{
    return (code() == TokenCode::Whitespace) || (code() == TokenCode::NewLine);
}
}
