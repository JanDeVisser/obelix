/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <lexer/BasicParser.h>

namespace Obelix {

extern_logging_category(lexer);

ErrorOr<std::shared_ptr<BasicParser>,SystemError> BasicParser::create(std::string const& file_name, BufferLocator* locator)
{
    auto ret = std::shared_ptr<BasicParser>(new BasicParser);
    TRY_RETURN(ret->read_file(file_name, locator));
    return ret;
}

BasicParser::BasicParser(StringBuffer& src)
    : BasicParser()
{
    m_lexer.assign(src.str());
}

BasicParser::BasicParser()
    : m_lexer()
{
}

ErrorOr<void,SystemError> BasicParser::read_file(std::string const& file_name, BufferLocator* locator)
{
    auto buffer = TRY(FileBuffer::create(file_name, locator));

    m_file_name = file_name;
    m_file_path = buffer->file_path();
    m_lexer.assign(buffer->buffer()->str(), m_file_name);
    return {};
}

static Token s_eof(TokenCode::EndOfFile, "EOF triggered by lexer error");

Token const& BasicParser::peek()
{
    auto& ret = m_lexer.peek();
    debug(lexer, "Parser::peek(): {}", ret.to_string());
    if (ret.code() != TokenCode::Error)
        return ret;
    add_error(ret, ret.value());
    return s_eof;
}

TokenCode BasicParser::current_code()
{
    return peek().code();
}

Token const& BasicParser::lex()
{
    auto& ret = m_lexer.lex();
    debug(lexer, "Parser::lex(): {}", ret.to_string());
    if (ret.code() != TokenCode::Error)
        return ret;
    add_error(ret, ret.value());
    return s_eof;
}

Token const& BasicParser::replace(Token token)
{
    auto& ret = m_lexer.replace(std::move(token));
    debug(lexer, "Parser::replace({}): {}", peek(), ret);
    if (ret.code() != TokenCode::Error)
        return ret;
    add_error(ret, ret.value());
    return s_eof;
}

std::optional<Token const> BasicParser::match(TokenCode code, char const* where)
{
    debug(lexer, "Parser::match({})", TokenCode_name(code));
    auto token = peek();
    if (token.code() != code) {
        if (where)
            add_error(token, "Expected '{}' {}, got '{}' ({})", TokenCode_name(code), where, token.value());
        else
            add_error(token, "Expected '{}', got '{}' ({})", TokenCode_name(code), token.value());
        return {};
    }
    return lex();
}

std::optional<Token const> BasicParser::skip(TokenCode code)
{
    debug(lexer, "Parser::skip({})", TokenCode_name(code));
    auto token = peek();
    if (token.code() != code) {
        return {};
    }
    return lex();
}

bool BasicParser::expect(TokenCode code, char const* where)
{
    debug(lexer, "Parser::expect({})", TokenCode_name(code));
    auto token = peek();
    if (token.code() != code) {
        if (where)
            add_error(token, "Expected '{}' {}, got '{}' ({})", TokenCode_to_string(code), where, token.value(), token.code_name());
        else
            add_error(token, "Expected '{}', got '{}' ({})", TokenCode_to_string(code), token.value(), token.code_name());
        return false;
    }
    lex();
    return true;
}

bool BasicParser::expect(char const* expected, char const* where)
{
    debug(lexer, "Parser::expect({})", expected);
    auto token = peek();
    if (strcmp(token.value().c_str(), expected)) {
        if (where)
            add_error(token, "Expected '{}' {}, got '{}' ({})", expected, where, token.value(), token.code_name());
        else
            add_error(token, "Expected '{}', got '{}' ({})", expected, token.value(), token.code_name());
        return false;
    }
    lex();
    return true;
}

void BasicParser::add_error(Span const& location, std::string const& message)
{
    debug(lexer, "Parser::add_error({}, '{}')", location, message);
    m_errors.emplace_back(location, message);
}

}
