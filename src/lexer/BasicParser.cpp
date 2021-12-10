/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <lexer/BasicParser.h>

namespace Obelix {

extern_logging_category(lexer);

BasicParser::BasicParser(std::string const& file_name)
    : BasicParser(FileBuffer(file_name).buffer())
{
    m_file_name = file_name;
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

Token const& BasicParser::peek()
{
    static Token s_eof(TokenCode::EndOfFile, "EOF triggered by lexer error");
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
    static Token s_eof(TokenCode::EndOfFile, "EOF triggered by lexer error");
    auto& ret = m_lexer.lex();
    debug(lexer, "Parser::lex(): {}", ret.to_string());
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
        std::string msg = (where)
            ? format("Syntax Error: expected '{}' {}, got '{}'", TokenCode_name(code), where, token.value())
            : format("Syntax Error: expected '{}', got '{}'", TokenCode_name(code), token.value());
        add_error(token, msg);
        return {};
    }
    return lex();
}

bool BasicParser::expect(TokenCode code, char const* where)
{
    debug(lexer, "Parser::expect({})", TokenCode_name(code));
    auto token = peek();
    if (token.code() != code) {
        std::string msg = (where)
            ? format("Syntax Error: expected '{}' {}, got '{}' ({})", TokenCode_to_string(code), where, token.value(), token.code_name())
            : format("Syntax Error: expected '{}', got '{}' ({})", TokenCode_to_string(code), token.value(), token.code_name());
        add_error(token, msg);
        return false;
    }
    lex();
    return true;
}

void BasicParser::add_error(Token const& token, std::string const& message)
{
    debug(lexer, "Parser::add_error({}, '{}')", token.to_string(), message);
    m_errors.emplace_back(message, m_file_name, token);
}

}