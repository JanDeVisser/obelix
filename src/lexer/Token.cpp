/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

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
        return format("Custom ({})", (int)t);
    }
}

Span Span::merge(Span const& other) const
{
    size_t new_start_line = start_line;
    if (other.start_line < new_start_line)
        new_start_line = other.start_line;
    size_t new_end_line = end_line;
    if (other.end_line > new_end_line)
        new_end_line = other.end_line;
    size_t new_start_column = start_column;
    if (other.start_column < new_start_column)
        new_start_column = other.start_column;
    size_t new_end_column = end_column;
    if (other.end_column > new_end_column)
        new_end_column = other.end_column;
    return Span { file_name, new_start_line, new_start_column, new_end_line, new_end_column };
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
