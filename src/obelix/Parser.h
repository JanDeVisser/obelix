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
#include <obelix/Runtime.h>
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
    constexpr static TokenCode KeywordVar = TokenCode::Keyword0;
    constexpr static TokenCode KeywordFunc = TokenCode::Keyword1;
    constexpr static TokenCode KeywordIf = TokenCode::Keyword2;
    constexpr static TokenCode KeywordElse = TokenCode::Keyword3;
    constexpr static TokenCode KeywordWhile = TokenCode::Keyword4;
    constexpr static TokenCode KeywordTrue = TokenCode::Keyword5;
    constexpr static TokenCode KeywordFalse = TokenCode::Keyword6;
    constexpr static TokenCode KeywordReturn = TokenCode::Keyword7;
    constexpr static TokenCode KeywordBreak = TokenCode::Keyword8;
    constexpr static TokenCode KeywordContinue = TokenCode::Keyword9;
    constexpr static TokenCode KeywordElif = TokenCode::Keyword10;
    constexpr static TokenCode KeywordSwitch = TokenCode::Keyword11;
    constexpr static TokenCode KeywordCase = TokenCode::Keyword12;
    constexpr static TokenCode KeywordDefault = TokenCode::Keyword13;
    constexpr static TokenCode KeywordLink = TokenCode::Keyword14;
    constexpr static TokenCode KeywordImport = TokenCode::Keyword15;
    constexpr static TokenCode KeywordFor = TokenCode::Keyword16;
    constexpr static TokenCode KeywordIn = TokenCode::Keyword17;
    constexpr static TokenCode KeywordRange = TokenCode::Keyword18;

    Parser(Runtime::Config const& parser_config, StringBuffer& src);
    Parser(Runtime::Config const& parser_config, std::string const& file_name);

    std::shared_ptr<Module> parse(Runtime&);
    [[nodiscard]] std::vector<ParseError> const& errors() const { return m_errors; };
    [[nodiscard]] bool has_errors() const { return !m_errors.empty(); }

private:
    std::shared_ptr<Statement> parse_statement(SyntaxNode*);
    void parse_statements(std::shared_ptr<Block> const& block);
    std::shared_ptr<Block> parse_block(std::shared_ptr<Block> block);
    std::shared_ptr<FunctionCall> parse_function_call(std::shared_ptr<Expression>);
    std::shared_ptr<Assignment> parse_assignment(SyntaxNode*, std::string const& identifier);
    std::shared_ptr<FunctionDef> parse_function_definition(SyntaxNode*);
    std::shared_ptr<IfStatement> parse_if_statement(SyntaxNode*);
    std::shared_ptr<SwitchStatement> parse_switch_statement(SyntaxNode*);
    std::shared_ptr<WhileStatement> parse_while_statement(SyntaxNode*);
    std::shared_ptr<ForStatement> parse_for_statement(SyntaxNode*);
    std::shared_ptr<Assignment> parse_variable_declaration(SyntaxNode*);
    std::shared_ptr<Import> parse_import_statement(SyntaxNode*);
    std::shared_ptr<Expression> parse_expression(SyntaxNode*);
    static int binary_precedence(Token const& token);
    static int unary_precedence(Token const& token);
    static int is_postfix_unary_operator(Token const& token);
    static int is_prefix_unary_operator(Token const& token);

    std::shared_ptr<Expression> parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence);
    std::shared_ptr<Expression> parse_postfix_unary_operator(std::shared_ptr<Expression> expression);
    std::shared_ptr<Expression> parse_primary_expression(SyntaxNode*, bool);

    Token const& peek();
    TokenCode current_code();
    Token const& lex();
    std::optional<Token const> match(TokenCode, char const* = nullptr);
    bool expect(TokenCode, char const* = nullptr);
    void add_error(Token const&, std::string const&);

    Runtime::Config m_config;
    std::string m_file_name { "<literal>" };
    StringBuffer m_src;
    Lexer m_lexer;
    std::vector<ParseError> m_errors {};
};

}
