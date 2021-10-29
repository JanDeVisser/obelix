/*
 * Parser.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <obelix/Parser.h>
#include <obelix/Syntax.h>

namespace Obelix {

Parser::Parser(std::string const& file_name)
    : m_file_buffer(file_name)
    , m_file_name(file_name)
    , m_lexer(m_file_buffer.buffer())
{
    m_lexer.add_scanner<QStringScanner>();
    m_lexer.add_scanner<IdentifierScanner>();
    m_lexer.add_scanner<NumberScanner>(Obelix::NumberScanner::Config { true, false, true, true });
    m_lexer.add_scanner<WhitespaceScanner>(Obelix::WhitespaceScanner::Config { true, true, false });
    m_lexer.add_scanner<CommentScanner>(
        CommentScanner::CommentMarker { false, false, "/*", "*/" },
        CommentScanner::CommentMarker { false, true, "//", "" },
        CommentScanner::CommentMarker { true, true, "#", "" });
    m_lexer.filter_codes(TokenCode::Whitespace, TokenCode::Comment);
    m_lexer.add_scanner<KeywordScanner>(
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
        TokenCode::GreaterEqualThan,
        TokenCode::LessEqualThan,
        TokenCode::EqualsTo,
        TokenCode::NotEqualTo,
        TokenCode::LogicalAnd,
        TokenCode::LogicalOr,
        TokenCode::ShiftLeft,
        TokenCode::ShiftRight);
}

std::shared_ptr<Module> Parser::parse(Runtime& runtime)
{
    std::shared_ptr<Module> module = std::make_shared<Module>(m_file_name, runtime);
    parse_statements(module);
    if (has_errors())
        return nullptr;
    return module;
}

std::shared_ptr<Statement> Parser::parse_statement(SyntaxNode* parent)
{
    auto token = peek();
    switch (token.code()) {
    case TokenCode::SemiColon:
        lex();
        return std::make_shared<Pass>(parent);
    case TokenCode::OpenBrace: {
        lex();
        auto block = parse_block(std::make_shared<Block>(parent));
        if (has_errors())
            return nullptr;
        return block;
    }
    case KeywordImport:
        lex();
        return parse_import_statement(parent);
    case KeywordIf:
        lex();
        return parse_if_statement(parent);
    case KeywordSwitch:
        lex();
        return parse_switch_statement(parent);
    case KeywordWhile:
        lex();
        return parse_while_statement(parent);
    case KeywordVar:
        lex();
        return parse_variable_declaration(parent);
    case KeywordFunc:
        lex();
        return parse_function_definition(parent);
    case KeywordReturn: {
        lex();
        auto expr = parse_expression(parent);
        if (!expr)
            return nullptr;
        return std::make_shared<Return>(parent, expr);
    }
    case KeywordBreak:
        lex();
        return std::make_shared<Break>(parent);
    case KeywordContinue:
        lex();
        return std::make_shared<Continue>(parent);
    case TokenCode::CloseBrace:
    case TokenCode::EndOfFile:
        break;
    default:
        auto expr = parse_expression(parent);
        return std::make_shared<ExpressionStatement>(expr);
    }
    return nullptr;
}

void Parser::parse_statements(std::shared_ptr<Block> const& block)
{
    while (true) {
        auto statement = parse_statement(std::dynamic_pointer_cast<SyntaxNode>(block).get());
        if (!statement)
            break;
        block->append(statement);
    }
}

std::shared_ptr<Block> Parser::parse_block(std::shared_ptr<Block> block)
{
    parse_statements(block);
    if (!expect(TokenCode::CloseBrace)) {
        return nullptr;
    }
    return block;
}

std::shared_ptr<FunctionCall> Parser::parse_function_call(std::shared_ptr<Expression> function)
{
    if (!expect(TokenCode::OpenParen, "after function expression")) {
        return nullptr;
    }
    std::vector<std::shared_ptr<Expression>> args {};
    auto done = current_code() == TokenCode::CloseParen;
    while (!done) {
        args.push_back(parse_expression(function->parent()));
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
    return std::make_shared<FunctionCall>(function, args);
}

std::shared_ptr<Assignment> Parser::parse_assignment(SyntaxNode* parent, std::string const& identifier)
{
    lex(); // Consume the equals
    auto expr = parse_expression(parent);
    if (!expr)
        return nullptr;
    return std::make_shared<Assignment>(identifier, expr);
}

std::shared_ptr<FunctionDef> Parser::parse_function_definition(SyntaxNode* parent)
{
    auto name_maybe = match(TokenCode::Identifier);
    if (!name_maybe.has_value()) {
        add_error(peek(), "Syntax Error: expecting variable name after the 'func' keyword, got '{}'");
        return nullptr;
    }
    if (!expect(TokenCode::OpenParen, "after function name in definition")) {
        return nullptr;
    }
    std::vector<std::string> params {};
    auto done = current_code() == TokenCode::CloseParen;
    while (!done) {
        auto param_name_maybe = match(TokenCode::Identifier);
        if (!param_name_maybe.has_value()) {
            add_error(peek(), "Syntax Error: Expected parameter name, got '{}'");
            return nullptr;
        }
        params.push_back(param_name_maybe.value().value());
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
    lex();
    switch (current_code()) {
    case TokenCode::OpenBrace: {
        lex();
        auto function_def = std::make_shared<FunctionDef>(parent, name_maybe.value().value(), params);
        parse_block(function_def);
        return function_def;
    }
    case KeywordLink: {
        lex();
        if (auto link_target_maybe = match(TokenCode::DoubleQuotedString, "after '->'"); link_target_maybe.has_value()) {
            return std::make_shared<NativeFunctionDef>(parent, name_maybe.value().value(), params, link_target_maybe.value().value());
        } else {
            return nullptr;
        }
    }
    default:
        auto t = peek();
        add_error(lex(), format("Expected '{{' or '->' after function declaration, got '{}'", t.value()));
        return nullptr;
    }
}

std::shared_ptr<IfStatement> Parser::parse_if_statement(SyntaxNode* parent)
{
    auto condition = parse_expression(parent);
    if (!condition)
        return nullptr;
    auto if_stmt = parse_statement(parent);
    if (!if_stmt)
        return nullptr;
    std::shared_ptr<IfStatement> ret = std::make_shared<IfStatement>(condition, if_stmt);
    while (true) {
        switch (current_code()) {
        case KeywordElif: {
            lex();
            auto elif_condition = parse_expression(parent);
            if (!elif_condition)
                return nullptr;
            auto elif_stmt = parse_statement(parent);
            if (!elif_stmt)
                return nullptr;
            ret->append_elif(elif_condition, elif_stmt);
        } break;
        case KeywordElse: {
            lex();
            auto else_stmt = parse_statement(parent);
            if (!else_stmt)
                return nullptr;
            ret->append_else(else_stmt);
            return ret;
        }
        default:
            return ret;
        }
    }
}

std::shared_ptr<SwitchStatement> Parser::parse_switch_statement(SyntaxNode* parent)
{
    auto switch_expr = parse_expression(parent);
    if (!switch_expr)
        return nullptr;
    std::shared_ptr<SwitchStatement> ret = std::make_shared<SwitchStatement>(parent, switch_expr);
    if (!expect(TokenCode::OpenBrace, "after switch expression")) {
        return nullptr;
    }
    while (true) {
        switch (current_code()) {
        case KeywordCase: {
            lex();
            auto expr = parse_expression(parent);
            if (!expr)
                return nullptr;
            if (!expect(TokenCode::Colon, "after switch expression")) {
                return nullptr;
            }
            auto stmt = parse_statement(parent);
            if (!stmt)
                return nullptr;
            ret->append_case(expr, stmt);
        } break;
        case KeywordDefault: {
            auto default_token = lex();
            if (!expect(TokenCode::Colon, "after 'default' keyword")) {
                return nullptr;
            }
            if (ret->default_case()) {
                add_error(default_token, "Syntax Error: Cam only specify one 'default' clause in a switch statement");
                return nullptr;
            }
            auto stmt = parse_statement(parent);
            if (!stmt)
                return nullptr;
            ret->set_default(stmt);
        } break;
        case TokenCode::CloseBrace:
            lex();
            return ret;
        default:
            add_error(peek(), "Syntax Error: Unexpected token '{}' in switch statement");
            return nullptr;
        }
    }
}

std::shared_ptr<WhileStatement> Parser::parse_while_statement(SyntaxNode* parent)
{
    auto condition = parse_expression(parent);
    if (!condition)
        return nullptr;
    auto stmt = parse_statement(parent);
    if (!stmt)
        return nullptr;
    return std::make_shared<WhileStatement>(parent, condition, stmt);
}

std::shared_ptr<Assignment> Parser::parse_variable_declaration(SyntaxNode* parent)
{
    auto identifier_maybe = match(TokenCode::Identifier);
    if (!identifier_maybe.has_value()) {
        return nullptr;
    }
    if (!expect(TokenCode::Equals, "after variable name in assignment")) {
        return nullptr;
    }
    auto expr = parse_expression(parent);
    if (!expr)
        return nullptr;
    return std::make_shared<Assignment>(identifier_maybe.value().value(), expr, true);
}

std::shared_ptr<Import> Parser::parse_import_statement(SyntaxNode* parent)
{
    std::string module_name;
    while (true) {
        auto identifier_maybe = match(TokenCode::Identifier, "in import statement");
        if (!identifier_maybe.has_value())
            return nullptr;
        module_name += identifier_maybe.value().value();
        if (current_code() != TokenCode::Period)
            break;
        module_name += '.';
    }
    return std::make_shared<Import>(parent, module_name);
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
std::shared_ptr<Expression> Parser::parse_expression(SyntaxNode* parent)
{
    auto primary = parse_primary_expression(parent, false);
    if (!primary)
        return nullptr;
    return parse_expression_1(primary, 0);
}

int Parser::binary_precedence(Token const& token)
{
    switch (token.code()) {
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
        return 14;
    default:
        return -1;
    }
}

int Parser::unary_precedence(Token const& token)
{
    switch (token.code()) {
    case TokenCode::Plus:
    case TokenCode::Minus:
    case TokenCode::Tilde:
    case TokenCode::ExclamationPoint:
        return 13;
    case TokenCode::OpenParen:
        //    case TokenCode::OpenBracket:
        return 14;
    default:
        return -1;
    }
}

int Parser::is_postfix_unary_operator(Token const& token)
{
    switch (token.code()) {
    case TokenCode::OpenParen:
        //    case TokenCode::OpenBracket:
        return true;
    default:
        return false;
    }
}

int Parser::is_prefix_unary_operator(Token const& token)
{
    switch (token.code()) {
    case TokenCode::Plus:
    case TokenCode::Minus:
    case TokenCode::Tilde:
    case TokenCode::ExclamationPoint:
        return true;
    default:
        return false;
    }
}

std::shared_ptr<Expression> Parser::parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence)
{
    auto next = peek();
    while (binary_precedence(next) >= min_precedence) {
        auto op = lex();
        auto rhs = parse_primary_expression(lhs->parent(), op.code() == TokenCode::Period);
        if (!rhs)
            return nullptr;
        next = peek();
        while (binary_precedence(next) > binary_precedence(op)) {
            rhs = parse_expression_1(rhs, binary_precedence(op) + 1);
            next = peek();
        }
        if (is_postfix_unary_operator(next) && (unary_precedence(next) > binary_precedence(op))) {
            lhs = parse_postfix_unary_operator(lhs);
        }
        lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
    }
    if (is_postfix_unary_operator(next)) {
        lhs = parse_postfix_unary_operator(lhs);
    }
    return lhs;
}

std::shared_ptr<Expression> Parser::parse_postfix_unary_operator(std::shared_ptr<Expression> expression)
{
    assert(is_postfix_unary_operator(peek()));
    switch (current_code()) {
    case TokenCode::OpenParen:
        // rhs := function call with rhs as function expr.
        return parse_function_call(expression);
    default:
        fatal("Unreachable");
    }
}

std::shared_ptr<Expression> Parser::parse_primary_expression(SyntaxNode* parent, bool in_deref_chain)
{
    auto t = lex();
    switch (t.code()) {
    case TokenCode::OpenParen: {
        auto ret = parse_expression(parent);
        if (!expect(TokenCode::CloseParen)) {
            return nullptr;
        }
        return ret;
    }
    case TokenCode::Tilde:
    case TokenCode::ExclamationPoint:
    case TokenCode::Plus:
    case TokenCode::Minus: {
        auto operand = parse_primary_expression(parent, false);
        if (!operand)
            return nullptr;
        return std::make_shared<UnaryExpression>(t, operand);
    }
    case TokenCode::Integer:
    case TokenCode::Float:
    case TokenCode::DoubleQuotedString:
    case TokenCode::SingleQuotedString:
    case KeywordTrue:
    case KeywordFalse:
        return std::make_shared<Literal>(parent, t);
    case TokenCode::Identifier: {
        if (in_deref_chain)
            return std::make_shared<Literal>(parent, make_obj<String>(t.value()));
        return std::make_shared<BinaryExpression>(
            std::make_shared<This>(parent), ".", std::make_shared<Literal>(parent, make_obj<String>(t.value())));
    }
    default:
        add_error(t, "Syntax Error: Expected literal or variable, got '{}'");
        return nullptr;
    }
}

Token const& Parser::peek()
{
    static Token s_eof(TokenCode::EndOfFile, "EOF triggered by lexer error");
    auto& ret = m_lexer.peek();
    if (ret.code() != TokenCode::Error)
        return ret;
    add_error(ret, ret.value());
    return s_eof;
}

TokenCode Parser::current_code()
{
    return peek().code();
}

Token const& Parser::lex()
{
    static Token s_eof(TokenCode::EndOfFile, "EOF triggered by lexer error");
    auto& ret = m_lexer.lex();
    if (ret.code() != TokenCode::Error)
        return ret;
    add_error(ret, ret.value());
    return s_eof;
}

std::optional<Token const> Parser::match(TokenCode code, char const* where)
{
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

bool Parser::expect(TokenCode code, char const* where)
{
    auto token = peek();
    if (token.code() != code) {
        std::string msg = (where)
            ? format("Syntax Error: expected '{}' {}, got '{}'", TokenCode_to_string(code), where, token.value())
            : format("Syntax Error: expected '{}', got '{}'", TokenCode_to_string(code), token.value());
        add_error(token, msg);
        return false;
    }
    lex();
    return true;
}

void Parser::add_error(Token const& token, std::string const& message)
{
    m_errors.emplace_back(message, m_file_name, token);
}

}
