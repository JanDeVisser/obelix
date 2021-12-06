/*
 * Parser.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
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
    [[nodiscard]] Lexer& lexer() { return m_lexer; }
    [[nodiscard]] std::string const& file_name() const { return m_file_name; }

private:
    std::string m_file_name { "<literal>" };
    Lexer m_lexer;
    std::vector<ParseError> m_errors {};
};

}
