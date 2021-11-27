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

struct Config {
    bool show_tree { false };
};

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

class Parser {
public:
    enum class Associativity {
        LeftToRight,
        RightToLeft,
    };

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
    constexpr static TokenCode KeywordWhere = TokenCode::Keyword19;
    constexpr static TokenCode KeywordIncEquals = TokenCode::Keyword20;
    constexpr static TokenCode KeywordDecEquals = TokenCode::Keyword21;
    constexpr static TokenCode KeywordConst = TokenCode::Keyword22;

    Parser(Config const& parser_config, StringBuffer& src);
    Parser(Config const& parser_config, std::string const& file_name);
    explicit Parser(Config const& parser_config);

    std::shared_ptr<Module> parse();
    std::shared_ptr<Module> parse(std::string const&);
    [[nodiscard]] std::vector<ParseError> const& errors() const { return m_errors; };
    [[nodiscard]] bool has_errors() const { return !m_errors.empty(); }
    Token const& peek();
    TokenCode current_code();

    static int binary_precedence(TokenCode);
    static int unary_precedence(TokenCode);
    static Associativity associativity(TokenCode);
    static int is_postfix_unary_operator(TokenCode);
    static int is_prefix_unary_operator(TokenCode);
    static TokenCode operator_for_assignment_operator(TokenCode);
    static bool is_assignment_operator(TokenCode);

private:
    std::shared_ptr<Statement> parse_statement();
    void parse_statements(Statements&);
    std::shared_ptr<Block> parse_block(Statements&);
    std::shared_ptr<FunctionCall> parse_function_call(std::shared_ptr<Expression> const&);
    std::shared_ptr<FunctionDef> parse_function_definition();
    std::shared_ptr<IfStatement> parse_if_statement();
    std::shared_ptr<SwitchStatement> parse_switch_statement();
    std::shared_ptr<WhileStatement> parse_while_statement();
    std::shared_ptr<ForStatement> parse_for_statement();
    std::shared_ptr<VariableDeclaration> parse_variable_declaration(bool);
    std::shared_ptr<Import> parse_import_statement();
    std::shared_ptr<Expression> parse_expression();

    std::shared_ptr<Expression> parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence);
    std::shared_ptr<Expression> parse_postfix_unary_operator(std::shared_ptr<Expression> const& expression);
    std::shared_ptr<Expression> parse_primary_expression(bool);

    Token const& lex();
    std::optional<Token const> match(TokenCode, char const* = nullptr);
    bool expect(TokenCode, char const* = nullptr);
    void add_error(Token const&, std::string const&);

    Config m_config;
    std::string m_file_name { "<literal>" };
    Lexer m_lexer;
    std::vector<ParseError> m_errors {};
};

}
