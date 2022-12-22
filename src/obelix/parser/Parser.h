/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <map>
#include <set>

#include <lexer/BasicParser.h>
#include <obelix/syntax/Forward.h>
#include <obelix/Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/Processor.h>

namespace Obelix {

struct ParserContext {
    Config const& config;
    std::set<std::string> modules;
};

template<>
inline ParserContext& make_subcontext(ParserContext& ctx)
{
    return ctx;
}

enum class OperandKind {
    None,
    Value,
    Type
};

struct OperatorDef {
    TokenCode op;
    OperandKind lhs_kind;
    OperandKind rhs_kind;
    int precedence;
    OperandKind unary_kind { OperandKind::None };
    int unary_precedence { -1 };
};

enum class Associativity {
    LeftToRight,
    RightToLeft,
};

class Parser : public BasicParser {
public:
    enum class VariableKind {
        Local,
        Static,
        ModuleLocal,
        Global
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
    constexpr static TokenCode KeywordEnum = TokenCode::Keyword26;
    constexpr static TokenCode KeywordGlobal = TokenCode::Keyword27;
    constexpr static TokenCode KeywordExtend = TokenCode::Keyword28;
    constexpr static TokenCode KeywordAs = TokenCode::Keyword29;

    static ErrorOr<std::shared_ptr<Parser>,SystemError> create(ParserContext&, std::string const&);
    std::shared_ptr<Module> parse();

protected:
    Parser(ParserContext&);

private:
    std::shared_ptr<Statement> parse_statement();
    std::shared_ptr<Statement> parse_top_level_statement();
    void parse_statements(Statements&, bool = false);
    std::shared_ptr<Block> parse_block(Statements&);
    std::shared_ptr<Statement> parse_function_definition(Token const&);
    std::shared_ptr<IfStatement> parse_if_statement(Token const&);
    std::shared_ptr<SwitchStatement> parse_switch_statement(Token const&);
    std::shared_ptr<WhileStatement> parse_while_statement(Token const&);
    std::shared_ptr<ForStatement> parse_for_statement(Token const&);
    std::shared_ptr<VariableDeclaration> parse_static_variable_declaration();
    std::shared_ptr<VariableDeclaration> parse_global_variable_declaration();
    std::shared_ptr<VariableDeclaration> parse_variable_declaration(Token const&, bool, VariableKind = VariableKind::Local);
    std::shared_ptr<Statement> parse_struct(Token const&);
    std::shared_ptr<Import> parse_import_statement(Token const&);
    std::shared_ptr<Expression> parse_expression();
    std::shared_ptr<EnumDef> parse_enum_definition(Token const&);
    std::shared_ptr<TypeDef> parse_type_definition(Token const&);

    std::shared_ptr<Expression> parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence);
    std::shared_ptr<Expression> parse_primary_expression();
    std::shared_ptr<ExpressionType> parse_type();

    ParserContext& m_ctx;
    std::vector<std::string> m_modules;
    std::string m_current_module;
};

[[nodiscard]] std::string sanitize_module_name(std::string const&);
[[nodiscard]] ProcessResult parse(std::string const&);
[[nodiscard]] ProcessResult compile_project(Config const&);

}
