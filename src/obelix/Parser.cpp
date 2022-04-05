/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/StringUtil.h"
#include "lexer/Token.h"
#include "obelix/Type.h"
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Intrinsics.h>
#include <obelix/Parser.h>
#include <obelix/Syntax.h>

namespace Obelix {

logging_category(parser);

Parser::Parser(Config const& config, std::string const& file_name)
    : BasicParser(file_name)
    , m_config(config)
{
    initialize();
}

Parser::Parser(Config const& config, StringBuffer& src)
    : BasicParser(src)
    , m_config(config)
{
    initialize();
}

Parser::Parser(Config const& config)
    : BasicParser()
    , m_config(config)
{
    initialize();
}

void Parser::initialize()
{
    lexer().add_scanner<QStringScanner>();
    lexer().add_scanner<IdentifierScanner>();
    lexer().add_scanner<NumberScanner>(Obelix::NumberScanner::Config { true, false, true, false, true });
    lexer().add_scanner<WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
    lexer().add_scanner<CommentScanner>(
        CommentScanner::CommentMarker { false, false, "/*", "*/" },
        CommentScanner::CommentMarker { false, true, "//", "" },
        CommentScanner::CommentMarker { true, true, "#", "" });
    lexer().filter_codes(TokenCode::Whitespace, TokenCode::Comment);
    lexer().add_scanner<KeywordScanner>(
        Token(KeywordVar, "var"),
        Token(KeywordFunc, "func"),
        Token(KeywordIf, "if"),
        Token(KeywordElse, "else"),
        Token(KeywordWhile, "while"),
        Token(KeywordTrue, "true"),
        Token(KeywordFalse, "false"),
        Token(KeywordReturn, "return"),
        Token(KeywordBreak, "break"),
        Token(KeywordContinue, "continue"),
        Token(KeywordElif, "elif"),
        Token(KeywordSwitch, "switch"),
        Token(KeywordCase, "case"),
        Token(KeywordDefault, "default"),
        Token(KeywordLink, "->"),
        Token(KeywordImport, "import"),
        Token(KeywordFor, "for"),
        Token(KeywordIn, "in"),
        Token(KeywordRange, ".."),
        Token(KeywordWhere, "where"),
        Token(KeywordConst, "const"),
        Token(KeywordIntrinsic, "intrinsic"),
        Token(KeywordStruct, "struct"),
        TokenCode::BinaryIncrement,
        TokenCode::BinaryDecrement,
        TokenCode::UnaryIncrement,
        TokenCode::UnaryDecrement,
        TokenCode::GreaterEqualThan,
        TokenCode::LessEqualThan,
        TokenCode::EqualsTo,
        TokenCode::NotEqualTo,
        TokenCode::LogicalAnd,
        TokenCode::LogicalOr,
        TokenCode::ShiftLeft,
        TokenCode::ShiftRight);
}

std::shared_ptr<Compilation> Parser::parse(std::string const& text)
{
    lexer().assign(text);
    return parse();
}

std::shared_ptr<Compilation> Parser::parse()
{
    clear_errors();
    m_modules.clear();
    auto main_module = parse_module();
    std::shared_ptr<Module> root = main_module;
    if (m_config.import_root)
        root = parse_module("");
    if (errors().empty()) {
        Modules modules { main_module };
        for (auto& module_name : m_modules) {
            auto imported_module = parse_module(module_name);
            if (imported_module)
                modules.push_back(imported_module);
        }
        return make_node<Compilation>(root, modules);
    }
    return nullptr;
}

std::shared_ptr<Module> Parser::parse_module(std::string const& module_name)
{
    if (auto ret = read_file(module_name); ret.is_error())
        return nullptr;
    return parse_module();
}

std::shared_ptr<Module> Parser::parse_module()
{
    Statements statements;
    parse_statements(statements);
    if (has_errors())
        return nullptr;
    return make_node<Module>(statements, file_name());
}

std::shared_ptr<Statement> Parser::parse_statement()
{
    debug(parser, "Parser::parse_statement");
    auto token = peek();
    std::shared_ptr<Statement> ret = nullptr;
    switch (token.code()) {
    case TokenCode::SemiColon:
        return make_node<Pass>(lex());
    case TokenCode::OpenBrace: {
        lex();
        Statements statements;
        return parse_block(statements);
    }
    case KeywordImport:
        ret = parse_import_statement(lex());
        break;
    case KeywordIf:
        return parse_if_statement(lex());
    case KeywordSwitch:
        return parse_switch_statement(lex());
    case KeywordWhile:
        return parse_while_statement(lex());
    case KeywordFor:
        return parse_for_statement(lex());
    case KeywordStruct:
        ret = parse_struct(lex());
        break;
    case KeywordVar:
    case KeywordConst:
        ret = parse_variable_declaration(lex(), token.code() == KeywordConst);
        break;
    case KeywordFunc:
    case KeywordIntrinsic:
        return parse_function_definition(lex());
    case KeywordReturn: {
        lex();
        auto expr = parse_expression();
        if (!expr)
            return nullptr;
        ret = make_node<Return>(token, expr);
    } break;
    case KeywordBreak:
        ret = make_node<Break>(lex());
        break;
    case KeywordContinue:
        ret = make_node<Continue>(lex());
        break;
    case TokenCode::CloseBrace:
    case TokenCode::EndOfFile:
        return nullptr;
    default: {
        auto expr = parse_expression();
        if (!expr)
            return nullptr;
        ret = make_node<ExpressionStatement>(expr);
    } break;
    }
    if (current_code() != TokenCode::SemiColon) {
        add_error(peek(), format("Expected ';', got '{}'", peek().value()));
        return nullptr;
    }
    lex();
    return ret;
}

void Parser::parse_statements(Statements& block)
{
    while (true) {
        auto statement = parse_statement();
        if (!statement)
            break;
        block.push_back(statement);
    }
}

std::shared_ptr<Block> Parser::parse_block(Statements& block)
{
    auto token = peek();
    parse_statements(block);
    if (!expect(TokenCode::CloseBrace)) {
        return nullptr;
    }
    return make_node<Block>(token, block);
}

std::shared_ptr<FunctionCall> Parser::parse_function_call(std::shared_ptr<Expression> const& function)
{
    // for now the function must be an identifier:
    if (function->node_type() != SyntaxNodeType::Identifier) {
        add_error(peek(), "Syntax Error: function reference must be identifier");
        return nullptr;
    }
    if (!expect(TokenCode::OpenParen, "after function expression")) {
        return nullptr;
    }
    Expressions args;
    auto done = current_code() == TokenCode::CloseParen;
    while (!done) {
        args.push_back(parse_expression());
        switch (current_code()) {
        case TokenCode::Comma:
            lex();
            break;
        case TokenCode::CloseParen:
            done = true;
            break;
        default:
            add_error(peek(), format("Syntax Error: Expected ',' or ')' in function argument list, got '{}'", peek().value()));
            return nullptr;
        }
    }
    lex();
    auto ident = std::dynamic_pointer_cast<Identifier>(function);
    return make_node<FunctionCall>(ident->token(), ident->name(), args);
}

std::shared_ptr<FunctionDef> Parser::parse_function_definition(Token const& func_token)
{
    auto name_maybe = match(TokenCode::Identifier);
    if (!name_maybe.has_value()) {
        add_error(peek(), "Syntax Error: expecting variable name after the 'func' keyword, got '{}'");
        return nullptr;
    }
    auto name = name_maybe.value();
    if (!expect(TokenCode::OpenParen, "after function name in definition")) {
        return nullptr;
    }
    Identifiers params {};
    auto done = current_code() == TokenCode::CloseParen;
    while (!done) {
        auto param_name_maybe = match(TokenCode::Identifier);
        if (!param_name_maybe.has_value()) {
            add_error(peek(), "Syntax Error: Expected parameter name, got '{}'");
            return nullptr;
        }
        auto param_name = param_name_maybe.value();
        if (!expect(TokenCode::Colon))
            return nullptr;

        auto param_type = parse_type();
        if (!param_type) {
            add_error(peek(), format("Syntax Error: Expected type name for parameter {}, got '{}'", param_name_maybe.value(), peek().value()));
            return nullptr;
        }
        params.push_back(std::make_shared<Identifier>(param_name, param_name.value(), param_type));
        switch (current_code()) {
        case TokenCode::Comma:
            lex();
            break;
        case TokenCode::CloseParen:
            done = true;
            break;
        default:
            add_error(peek(), format("Syntax Error: Expected ',' or ')' in function parameter list, got '{}'", peek().value()));
            return nullptr;
        }
    }
    lex(); // Eat the closing paren

    if (!expect(TokenCode::Colon))
        return nullptr;

    auto type = parse_type();
    if (!type) {
        add_error(peek(), format("Syntax Error: Expected return type name, got '{}'", peek().value()));
        return nullptr;
    }

    auto func_ident = std::make_shared<Identifier>(name, name.value(), type);
    std::shared_ptr<FunctionDecl> func_decl;
    if (current_code() == KeywordLink) {
        lex();
        if (auto link_target_maybe = match(TokenCode::DoubleQuotedString, "after '->'"); link_target_maybe.has_value()) {
            func_decl = std::make_shared<NativeFunctionDecl>(name, func_ident, params, link_target_maybe.value().value());
            expect(TokenCode::SemiColon);
            return make_node<FunctionDef>(func_token, func_decl);
        }
        return nullptr;
    }
    if (func_token.code() == KeywordIntrinsic) {
        func_decl = std::make_shared<IntrinsicDecl>(name, func_ident, params);
        return make_node<FunctionDef>(func_token, func_decl, nullptr);
    }
    func_decl = std::make_shared<FunctionDecl>(name, func_ident, params);
    auto stmt = parse_statement();
    if (stmt == nullptr)
        return nullptr;
    return make_node<FunctionDef>(func_token, func_decl, stmt);
}

std::shared_ptr<IfStatement> Parser::parse_if_statement(Token const& if_token)
{
    auto condition = parse_expression();
    if (!condition)
        return nullptr;
    auto if_stmt = parse_statement();
    if (!if_stmt)
        return nullptr;
    Branches branches;
    while (true) {
        switch (current_code()) {
        case KeywordElif: {
            auto elif_token = lex();
            auto elif_condition = parse_expression();
            if (!elif_condition)
                return nullptr;
            auto elif_stmt = parse_statement();
            if (!elif_stmt)
                return nullptr;
            branches.push_back(std::make_shared<Branch>(elif_token, elif_condition, elif_stmt));
        } break;
        case KeywordElse: {
            auto else_token = lex();
            auto else_stmt = parse_statement();
            if (!else_stmt)
                return nullptr;
            return make_node<IfStatement>(if_token, condition, if_stmt, branches, std::make_shared<Branch>(else_token, nullptr, else_stmt));
        }
        default:
            return make_node<IfStatement>(if_token, condition, if_stmt, branches, nullptr);
        }
    }
}

std::shared_ptr<SwitchStatement> Parser::parse_switch_statement(Token const& switch_token)
{
    auto switch_expr = parse_expression();
    if (!switch_expr)
        return nullptr;
    if (!expect(TokenCode::OpenBrace, "after switch expression")) {
        return nullptr;
    }
    CaseStatements cases;
    while (true) {
        switch (current_code()) {
        case KeywordCase: {
            auto case_token = lex();
            auto expr = parse_expression();
            if (!expr)
                return nullptr;
            if (!expect(TokenCode::Colon, "after switch expression")) {
                return nullptr;
            }
            auto stmt = parse_statement();
            if (!stmt)
                return nullptr;
            cases.push_back(std::make_shared<CaseStatement>(case_token, expr, stmt));
        } break;
        case KeywordDefault: {
            auto default_token = lex();
            if (!expect(TokenCode::Colon, "after 'default' keyword")) {
                return nullptr;
            }
            auto stmt = parse_statement();
            if (!stmt)
                return nullptr;
            return make_node<SwitchStatement>(switch_token, switch_expr, cases, std::make_shared<DefaultCase>(default_token, stmt));
        }
        case TokenCode::CloseBrace:
            lex();
            return make_node<SwitchStatement>(switch_token, switch_expr, cases, nullptr);
        default:
            add_error(peek(), "Syntax Error: Unexpected token '{}' in switch statement");
            return nullptr;
        }
    }
}

std::shared_ptr<WhileStatement> Parser::parse_while_statement(Token const& while_token)
{
    if (!expect(TokenCode::OpenParen, " in 'while' statement"))
        return nullptr;
    auto condition = parse_expression();
    if (!condition)
        return nullptr;
    if (!expect(TokenCode::CloseParen, " in 'while' statement"))
        return nullptr;
    auto stmt = parse_statement();
    if (!stmt)
        return nullptr;
    return make_node<WhileStatement>(while_token, condition, stmt);
}

std::shared_ptr<ForStatement> Parser::parse_for_statement(Token const& for_token)
{
    if (!expect(TokenCode::OpenParen, " in 'for' statement"))
        return nullptr;
    auto variable = match(TokenCode::Identifier, " in 'for' ststement");
    if (!variable.has_value())
        return nullptr;
    if (!expect("in", " in 'for' statement"))
        return nullptr;
    auto expr = parse_expression();
    if (!expr)
        return nullptr;
    if (!expect(TokenCode::CloseParen, " in 'for' statement"))
        return nullptr;
    auto stmt = parse_statement();
    if (!stmt)
        return nullptr;
    return make_node<ForStatement>(for_token, variable.value().value(), expr, stmt);
}

std::shared_ptr<Statement> Parser::parse_struct(Token const& struct_token)
{
    auto identifier_maybe = match(TokenCode::Identifier);
    if (!identifier_maybe.has_value())
        return nullptr;
    auto name = identifier_maybe->value();
    if (current_code() != TokenCode::OpenBrace)
        return make_node<StructForward>(lex(), name);
    lex();

    Identifiers fields;
    do {
        auto field_name_maybe = match(TokenCode::Identifier);
        if (!field_name_maybe.has_value())
            return nullptr;
        expect(TokenCode::Colon);
        auto field_type = parse_type();
        if (!field_type) {
            add_error(peek(), format("Syntax Error: Expected type after ':', got '{}' ({})", peek().value(), peek().code_name()));
            return nullptr;
        }
        expect(TokenCode::SemiColon);
        fields.push_back(make_node<Identifier>(field_name_maybe.value(), field_name_maybe.value().value(), field_type));
    } while (current_code() != TokenCode::CloseBrace);
    lex();
    return make_node<StructDefinition>(struct_token, name, fields);
}

std::shared_ptr<VariableDeclaration> Parser::parse_variable_declaration(Token const& var_token, bool constant)
{
    auto identifier_maybe = match(TokenCode::Identifier);
    if (!identifier_maybe.has_value()) {
        return nullptr;
    }
    auto identifier = identifier_maybe.value();
    std::shared_ptr<ExpressionType> type = nullptr;
    if (current_code() == TokenCode::Colon) {
        lex();
        auto var_type = parse_type();
        if (!var_type) {
            add_error(peek(), format("Syntax Error: Expected type after ':', got '{}' ({})", peek().value(), peek().code_name()));
            return nullptr;
        }
        type = var_type;
    }
    auto var_ident = std::make_shared<Identifier>(identifier, identifier.value(), type);
    if (current_code() != TokenCode::Equals) {
        if (constant) {
            add_error(peek(), format("Syntax Error: Expected expression after constant declaration, got '{}' ({})", peek().value(), peek().code_name()));
            return nullptr;
        }
        return make_node<VariableDeclaration>(var_token, var_ident, nullptr, constant);
    }
    lex();
    auto expr = parse_expression();
    if (!expr)
        return nullptr;
    return make_node<VariableDeclaration>(var_token, var_ident, expr, constant);
}

std::shared_ptr<Import> Parser::parse_import_statement(Token const& import_token)
{
    std::string module_name;
    while (true) {
        auto identifier_maybe = match(TokenCode::Identifier, "in import statement");
        if (!identifier_maybe.has_value())
            return nullptr;
        module_name += identifier_maybe.value().value();
        if (current_code() != TokenCode::Slash)
            break;
        lex();
        module_name += '/';
    }
    m_modules.insert(module_name);
    return make_node<Import>(import_token, module_name);
}

/*
 * Precedence climbing method (https://en.wikipedia.org/wiki/Operator-precedence_parser):
 *
 * parse_expression()
 *    return parse_expression_1(parse_primary(), 0)
 *
 * parse_expression_1(lhs, min_precedence)
 *    lookahead := peek next token
 *    while lookahead is a binary operator whose precedence is >= min_precedence
 *      *op := lookahead
 *      advance to next token
 *      rhs := parse_primary ()
 *      lookahead := peek next token
 *      while lookahead is a binary operator whose precedence is greater
 *              than op's, or a right-associative operator
 *              whose precedence is equal to op's
 *        rhs := parse_expression_1 (rhs, precedence of op + 1)
 *        lookahead := peek next token
 *      lhs := the result of applying op with operands lhs and rhs
 *    return lhs
 */
std::shared_ptr<Expression> Parser::parse_expression()
{
    auto primary = parse_primary_expression();
    if (!primary)
        return nullptr;
    return parse_expression_1(primary, 0);
}

/*
 * Precendeces according to https://en.cppreference.com/w/c/language/operator_precedence:
 */
int Parser::binary_precedence(TokenCode code)
{
    switch (code) {
    case TokenCode::Equals:
    case KeywordIncEquals:
    case KeywordDecEquals:
        return 1;
    case TokenCode::LogicalOr:
        return 3;
    case TokenCode::LogicalAnd:
        return 4;
    case TokenCode::Pipe:
        return 5;
    case TokenCode::Hat:
        return 6;
    case TokenCode::Ampersand:
        return 7;
    case TokenCode::EqualsTo:
    case TokenCode::NotEqualTo:
    case KeywordRange:
        return 8;
    case TokenCode::GreaterThan:
    case TokenCode::LessThan:
    case TokenCode::GreaterEqualThan:
    case TokenCode::LessEqualThan:
        return 9;
    case TokenCode::ShiftLeft:
    case TokenCode::ShiftRight:
        return 10;
    case TokenCode::Plus:
    case TokenCode::Minus:
        return 11;
    case TokenCode::Asterisk:
    case TokenCode::Slash:
    case TokenCode::Percent:
        return 12;
    case TokenCode::Period:
    case TokenCode::OpenBracket:
        return 14;
    case TokenCode::CloseBracket:
    default:
        return -1;
    }
}

Parser::Associativity Parser::associativity(TokenCode code)
{
    switch (code) {
    case TokenCode::Equals:
    case KeywordIncEquals:
    case KeywordDecEquals:
        return Associativity::RightToLeft;
    default:
        return Associativity::LeftToRight;
    }
}

int Parser::unary_precedence(TokenCode code)
{
    switch (code) {
    case TokenCode::Plus:
    case TokenCode::Minus:
    case TokenCode::Tilde:
    case TokenCode::ExclamationPoint:
    case TokenCode::Asterisk:
    case TokenCode::AtSign:
        return 13;
    case TokenCode::OpenParen:
        //    case TokenCode::OpenBracket:
        return 14;
    default:
        return -1;
    }
}

int Parser::is_postfix_unary_operator(TokenCode code)
{
    switch (code) {
    case TokenCode::OpenParen:
        //    case TokenCode::OpenBracket:
        return true;
    default:
        return false;
    }
}

int Parser::is_prefix_unary_operator(TokenCode code)
{
    switch (code) {
    case TokenCode::Plus:
    case TokenCode::Minus:
    case TokenCode::Tilde:
    case TokenCode::ExclamationPoint:
    case TokenCode::AtSign:
    case TokenCode::Asterisk:
        return true;
    default:
        return false;
    }
}

Token Parser::operator_for_assignment_operator(TokenCode code)
{
    switch (code) {
        case KeywordIncEquals:
            return Token(TokenCode::Plus, "+");
        case KeywordDecEquals:
            return Token(TokenCode::Minus, "-");
        default:
            return Token(TokenCode::Unknown, "??");
    }
}

bool Parser::is_assignment_operator(TokenCode code)
{
    switch (code) {
    case KeywordIncEquals:
    case KeywordDecEquals:
        return true;
    default:
        return false;
    }
}

std::shared_ptr<Expression> Parser::parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence)
{
    while (binary_precedence(current_code()) >= min_precedence) {
        auto op = lex();
        std::shared_ptr<Expression> rhs;
        if (associativity(op.code()) == Associativity::LeftToRight) {
            auto open_bracket = op.code() == TokenCode::OpenBracket;
            rhs = parse_primary_expression();
            if (!rhs)
                return nullptr;
            while (binary_precedence(current_code()) > binary_precedence(op.code())) {
                rhs = parse_expression_1(rhs, binary_precedence(op.code()) + 1);
            }
            if (open_bracket) {
                if (current_code() != TokenCode::CloseBracket)
                    return nullptr;
                lex();
                open_bracket = false;
            }
            if (is_postfix_unary_operator(current_code()) && (unary_precedence(current_code()) > binary_precedence(op.code())))
                lhs = parse_postfix_unary_operator(lhs);
        } else {
            rhs = parse_expression();
        }
        lhs = make_node<BinaryExpression>(lhs, op, rhs);
    }
    if (is_postfix_unary_operator(current_code())) {
        lhs = parse_postfix_unary_operator(lhs);
    }
    return lhs;
}

std::shared_ptr<Expression> Parser::parse_postfix_unary_operator(std::shared_ptr<Expression> const& expression)
{
    assert(is_postfix_unary_operator(current_code()));
    switch (current_code()) {
    case TokenCode::OpenParen:
        // rhs := function call with rhs as function expr.
        return parse_function_call(expression);
    default:
        fatal("Unreachable");
    }
}

std::shared_ptr<Expression> Parser::parse_primary_expression()
{
    auto t = lex();
    switch (t.code()) {
    case TokenCode::OpenParen: {
        auto ret = parse_expression();
        if (!expect(TokenCode::CloseParen)) {
            return nullptr;
        }
        return ret;
    }
    case TokenCode::Asterisk:
    case TokenCode::AtSign:
    case TokenCode::ExclamationPoint:
    case TokenCode::Minus:
    case TokenCode::Plus:
    case TokenCode::UnaryIncrement:
    case TokenCode::UnaryDecrement:
    case TokenCode::Tilde: {
        auto operand = parse_primary_expression();
        if (!operand)
            return nullptr;
        return make_node<UnaryExpression>(t, operand);
    }
    case TokenCode::Integer:
        return make_node<Literal>(t, PrimitiveType::Int);
    case TokenCode::HexNumber:
        return make_node<Literal>(t, PrimitiveType::Int);
    case TokenCode::Float:
        return make_node<Literal>(t, PrimitiveType::Float);
    case TokenCode::DoubleQuotedString:
        return make_node<Literal>(t, PrimitiveType::String);
    case TokenCode::SingleQuotedString:
        if (t.value().length() != 1) {
            add_error(t, format("Syntax Error: Single-quoted string should only hold a single character, not '{}'", t.value()));
            return nullptr;
        }
        return make_node<Literal>(t, PrimitiveType::Char);
    case KeywordTrue:
    case KeywordFalse:
        return make_node<Literal>(t, PrimitiveType::Boolean);
    case TokenCode::Identifier: {
        return make_node<Identifier>(t, t.value());
    }
    default:
        add_error(t, format("Syntax Error: Expected literal or variable, got '{}' ({})", t.value(), t.code_name()));
        return nullptr;
    }
}

std::shared_ptr<ExpressionType> Parser::parse_type()
{
    if (current_code() != TokenCode::Identifier)
        return nullptr;
    auto type_token = lex();
    auto type_name = type_token.value();
    if (current_code() == TokenCode::LessThan) {
        auto lt_token = lex();
        TemplateArgumentNodes arguments;
        while (true) {
            switch (current_code()) {
            case TokenCode::DoubleQuotedString: {
                auto token = lex();
                arguments.push_back(make_node<StringTemplateArgument>(token, token.value()));
                break;
            }
            case TokenCode::Integer:
            case TokenCode::HexNumber: {
                auto token = lex();
                arguments.push_back(make_node<IntegerTemplateArgument>(token, token.token_value<long>()));
                break;
            }
            case TokenCode::Identifier: {
                auto parameter = parse_type();
                if (!parameter) {
                    add_error(peek(), format("Syntax Error: Expected type, got '{}' ({})", peek().value(), peek().code_name()));
                    return nullptr;
                }
                arguments.push_back(parameter);
                break;
            }
            default:
                return nullptr;
            }
            if (current_code() == TokenCode::GreaterThan) {
                lex();
                return make_node<ExpressionType>(lt_token, type_name, arguments);
            }
            if (current_code() == TokenCode::ShiftRight) {
                replace(Token { TokenCode::GreaterThan, ">" });
                return make_node<ExpressionType>(lt_token, type_name, arguments);
            }
            if (!expect(TokenCode::Comma))
                return nullptr;
        }
    }
    return make_node<ExpressionType>(type_token, type_name);
}

}
