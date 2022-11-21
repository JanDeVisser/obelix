/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <lexer/Lexer.h>

namespace Obelix {

logging_category(lexer);

Lexer::Lexer(char const* text, std::string file_name)
    : m_file_name(std::move(file_name))
    , m_buffer((text) ? text : "")
{
}

Lexer::Lexer(StringBuffer text, std::string file_name)
    : m_file_name(std::move(file_name))
    , m_buffer(std::move(text))
{
}

void Lexer::assign(char const* text, std::string file_name)
{
    m_file_name = std::move(file_name);
    m_buffer.assign(text);
    m_tokens.clear();
    m_current = 0;
}

void Lexer::assign(std::string const& text, std::string file_name)
{
    assign(text.c_str(), std::move(file_name));
}

StringBuffer const& Lexer::buffer() const
{
    return m_buffer;
}

std::vector<Token> const& Lexer::tokenize(char const* text, std::string file_name)
{
    if (text)
        assign(text, std::move(file_name));
    Tokenizer tokenizer(m_buffer, m_file_name.c_str());
    tokenizer.add_scanners(m_scanners);
    tokenizer.filter_codes(m_filtered_codes);
    m_tokens = tokenizer.tokenize();
    return m_tokens;
}

Token const& Lexer::peek(size_t how_many)
{
    if (m_tokens.empty())
        tokenize();
    oassert(m_current + how_many < m_tokens.size(), "Token buffer underflow");
    return m_tokens[m_current + how_many];
}

Token const& Lexer::lex()
{
    auto const& ret = peek(0);
    if (m_current < (m_tokens.size() - 1))
        m_current++;
    return ret;
}

Token const& Lexer::replace(Token token)
{
    auto const& ret = peek(0);
    m_tokens[m_current] = std::move(token);
    return ret;
}

std::optional<Token const> Lexer::match(TokenCode code)
{
    if (peek().code() != code)
        return {};
    return lex();
}

TokenCode Lexer::current_code()
{
    return peek().code();
}

bool Lexer::expect(TokenCode code)
{
    auto token_maybe = match(code);
    return token_maybe.has_value();
}

void Lexer::mark()
{
    m_bookmarks.push_back(m_current);
}

void Lexer::discard_mark()
{
    m_bookmarks.pop_back();
}

void Lexer::rewind()
{
    m_current = m_bookmarks.back();
    discard_mark();
}

}
