/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <unordered_map>
#include <unordered_set>

#include <lexer/BasicParser.h>
#include <lexer/Lexer.h>
#include <lexer/OblBuffer.h>
#include <obelix/Syntax.h>

namespace Obelix {

struct Config {
public:
    Config(int argc, char const** argv);

    std::string filename { "" };
    bool help { false };
    bool show_tree { false };
    bool import_root { true };
    bool lex { true };
    bool bind { true };
    bool lower { true };
    bool fold_constants { true };
    bool materialize { true };
    bool compile { true };
    bool run { false };
    bool cmdline_flag(std::string const& flag) const;

private:
    std::unordered_map<std::string, bool> m_cmdline_flags;
};

class Parser : public BasicParser {
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
    constexpr static TokenCode KeywordIntrinsic = TokenCode::Keyword23;
    constexpr static TokenCode KeywordStruct = TokenCode::Keyword24;
    constexpr static TokenCode KeywordStatic = TokenCode::Keyword25;

    Parser(Config const& parser_config, StringBuffer& src);
    Parser(Config const& parser_config, std::string const& file_name);
    explicit Parser(Config const& parser_config);

    std::shared_ptr<Compilation> parse();
    std::shared_ptr<Compilation> parse(std::string const&);

    std::shared_ptr<Module> parse_module(std::string const&);
    std::shared_ptr<Module> parse_module();

    static int binary_precedence(TokenCode);
    static int unary_precedence(TokenCode);
    static Associativity associativity(TokenCode);
    static int is_postfix_unary_operator(TokenCode);
    static int is_prefix_unary_operator(TokenCode);
    static Token operator_for_assignment_operator(TokenCode);
    static bool is_assignment_operator(TokenCode);

private:
    void initialize();
    std::shared_ptr<Statement> parse_statement();
    std::shared_ptr<Statement> parse_top_level_statement();
    void parse_statements(Statements&, bool = false);
    std::shared_ptr<Block> parse_block(Statements&);
    std::shared_ptr<FunctionCall> parse_function_call(std::shared_ptr<Expression> const&);
    std::shared_ptr<Statement> parse_function_definition(Token const&);
    std::shared_ptr<IfStatement> parse_if_statement(Token const&);
    std::shared_ptr<SwitchStatement> parse_switch_statement(Token const&);
    std::shared_ptr<WhileStatement> parse_while_statement(Token const&);
    std::shared_ptr<ForStatement> parse_for_statement(Token const&);
    std::shared_ptr<VariableDeclaration> parse_static_variable_declaration();
    std::shared_ptr<VariableDeclaration> parse_variable_declaration(Token const&, bool, bool = false);
    std::shared_ptr<Statement> parse_struct(Token const&);
    std::shared_ptr<Import> parse_import_statement(Token const&);
    std::shared_ptr<Expression> parse_expression();

    std::shared_ptr<Expression> parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence);
    std::shared_ptr<Expression> parse_postfix_unary_operator(std::shared_ptr<Expression> const& expression);
    std::shared_ptr<Expression> parse_primary_expression();
    std::shared_ptr<ExpressionType> parse_type();

    Config m_config;
    std::unordered_set<std::string> m_modules;
};

}
