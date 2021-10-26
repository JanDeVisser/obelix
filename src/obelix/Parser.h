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
#include <obelix/Syntax.h>

namespace Obelix {

struct ParseError {
    ParseError(std::string const& msg, std::string const& fname, Token const& tok)
        : message(msg)
        , filename(fname)
        , token(tok)
    {
        if (msg.find("{}") != std::string::npos)
            message = format(msg, token.value());
    }

    [[nodiscard]] std::string to_string() const
    {
        return format("{}:{}:{} {}", filename, token.location.start_line, token.location.start_column, message);
    }

    std::string message;
    std::string filename;
    Token token;
};

class Parser {
public:
    constexpr static TokenCode KeywordVar = ((TokenCode)200);
    constexpr static TokenCode KeywordFunc = ((TokenCode)201);
    constexpr static TokenCode KeywordIf = ((TokenCode)202);
    constexpr static TokenCode KeywordElse = ((TokenCode)203);
    constexpr static TokenCode KeywordWhile = ((TokenCode)204);
    constexpr static TokenCode KeywordTrue = ((TokenCode)205);
    constexpr static TokenCode KeywordFalse = ((TokenCode)206);
    constexpr static TokenCode KeywordReturn = ((TokenCode)207);
    constexpr static TokenCode KeywordBreak = ((TokenCode)208);
    constexpr static TokenCode KeywordContinue = ((TokenCode)209);
    constexpr static TokenCode KeywordElif = ((TokenCode)210);
    constexpr static TokenCode KeywordSwitch = ((TokenCode)211);
    constexpr static TokenCode KeywordCase = ((TokenCode)212);
    constexpr static TokenCode KeywordDefault = ((TokenCode)213);
    constexpr static TokenCode KeywordLink = ((TokenCode)214);

    explicit Parser(std::string const& file_name);
    std::shared_ptr<Module> parse();
    [[nodiscard]] std::vector<ParseError> const& errors() const { return m_errors; };
    [[nodiscard]] bool has_errors() const { return !m_errors.empty(); }

private:
    std::shared_ptr<Statement> parse_statement();
    void parse_statements(std::shared_ptr<Block> const& block);
    std::shared_ptr<Block> parse_block(std::shared_ptr<Block> block);
    std::shared_ptr<FunctionCall> parse_function_call(std::string const& func_name);
    std::shared_ptr<Assignment> parse_assignment(std::string const& identifier);
    std::shared_ptr<FunctionDef> parse_function_definition();
    std::shared_ptr<IfStatement> parse_if_statement();
    std::shared_ptr<SwitchStatement> parse_switch_statement();
    std::shared_ptr<WhileStatement> parse_while_statement();
    std::shared_ptr<Assignment> parse_variable_declaration();
    std::shared_ptr<Expression> parse_expression();
    static int binary_precedence(Token const& token);
    std::shared_ptr<Expression> parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence);
    std::shared_ptr<Expression> parse_primary_expression();

    Token const& peek();
    TokenCode current_code();
    Token const& lex();
    std::optional<Token const> match(TokenCode, char const* = nullptr);
    bool expect(TokenCode, char const* = nullptr);
    void add_error(Token const&, std::string const&);

    FileBuffer m_file_buffer;
    std::string m_file_name;
    Lexer m_lexer;
    std::vector<ParseError> m_errors {};
};

}
