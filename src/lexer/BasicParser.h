/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <lexer/FileBuffer.h>
#include <lexer/Lexer.h>

namespace Obelix {

struct ParseError {
    ParseError(std::string const& msg, std::string fname, Token tok)
        : message(msg)
        , filename(move(fname))
        , token(std::move(tok))
    {
        if (msg.find("{}") != std::string::npos)
            message = format(msg, token.value());
    }

    [[nodiscard]] std::string to_string() const
    {
        return format("{}:{} {}", filename, token.location.to_string(), message);
    }

    std::string message;
    std::string filename;
    Token token;
};

class BasicParser {
public:
    explicit BasicParser(StringBuffer& src);
    explicit BasicParser(std::string const& file_name);
    explicit BasicParser();

    [[nodiscard]] std::vector<ParseError> const& errors() const { return m_errors; };
    [[nodiscard]] bool has_errors() const { return !m_errors.empty(); }
    Token const& peek();
    TokenCode current_code();

    Token const& lex();
    std::optional<Token const> match(TokenCode, char const* = nullptr);
    bool expect(TokenCode, char const* = nullptr);
    void add_error(Token const&, std::string const&);
    void clear_errors() { m_errors.clear(); }
    [[nodiscard]] bool was_successful() const { return m_errors.empty(); }
    [[nodiscard]] Lexer& lexer() { return m_lexer; }
    [[nodiscard]] std::string const& file_name() const { return m_file_name; }
    void mark() { m_lexer.mark(); }
    void discard_mark() { m_lexer.discard_mark(); }
    void rewind() { m_lexer.rewind(); }

private:
    std::string m_file_name { "<literal>" };
    Lexer m_lexer;
    std::vector<ParseError> m_errors {};
};

}
