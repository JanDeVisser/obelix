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

logging_category(parser);

Parser::Parser(Runtime::Config const& config, std::string const& file_name)
    : Parser(config, FileBuffer(file_name).buffer())
{
    m_file_name = file_name;
}

Parser::Parser(Runtime::Config const& config, StringBuffer& src)
    : Parser(config)
{
    m_lexer.assign(src.str());
}

Parser::Parser(Runtime::Config const& config)
    : m_config(config)
    , m_lexer()
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
        Token(KeywordFor, "for"),
        Token(KeywordIn, "in"),
        Token(KeywordRange, ".."),
        Token(KeywordWhere, "where"),
        Token(KeywordIncEquals, "+="),
        Token(KeywordDecEquals, "-="),
        TokenCode::GreaterEqualThan,
        TokenCode::LessEqualThan,
        TokenCode::EqualsTo,
        TokenCode::NotEqualTo,
        TokenCode::LogicalAnd,
        TokenCode::LogicalOr,
        TokenCode::ShiftLeft,
        TokenCode::ShiftRight);
}

template<class T, class... Args>
std::shared_ptr<T> make_node(Args&&... args)
{
    auto ret = std::make_shared<T>(std::forward<Args>(args)...);
    debug(parser, "make_node<{}>", typeid(T).name());
    return ret;
}

std::shared_ptr<Module> Parser::parse(Runtime& runtime, std::string const& text)
{
    m_lexer.assign(text);
    return parse(runtime);
}

std::shared_ptr<Module> Parser::parse(Runtime& runtime)
{
    Statements statements;
    parse_statements(nullptr, statements);
    if (has_errors())
        return nullptr;
    auto ret = make_node<Module>(m_file_name, runtime, statements);
    if (m_config.show_tree)
        fprintf(stdout, "%s\n", ret->to_string(0).c_str());
    return ret;
}

std::shared_ptr<Statement> Parser::parse_statement(SyntaxNode* parent)
{
    debug(parser, "Parser::parse_statement");
    auto token = peek();
    std::shared_ptr<Statement> ret = nullptr;
    switch (token.code()) {
    case TokenCode::SemiColon:
        lex();
        return make_node<Pass>(parent);
    case TokenCode::OpenBrace: {
        lex();
        Statements statements;
        return parse_block(parent, statements);
    }
    case KeywordImport:
        lex();
        ret = parse_import_statement(parent);
        break;
    case KeywordIf:
        lex();
        return parse_if_statement(parent);
    case KeywordSwitch:
        lex();
        return parse_switch_statement(parent);
    case KeywordWhile:
        lex();
        return parse_while_statement(parent);
    case KeywordFor:
        lex();
        return parse_for_statement(parent);
    case KeywordVar:
        lex();
        ret = parse_variable_declaration(parent);
        break;
    case KeywordFunc:
        lex();
        ret = parse_function_definition(parent);
        break;
    case KeywordReturn: {
        lex();
        auto expr = parse_expression(parent);
        if (!expr)
            return nullptr;
        ret = make_node<Return>(parent, expr);
    } break;
    case KeywordBreak:
        lex();
        ret = make_node<Break>(parent);
        break;
    case KeywordContinue:
        lex();
        ret = make_node<Continue>(parent);
        break;
    case TokenCode::CloseBrace:
    case TokenCode::EndOfFile:
        return nullptr;
    default: {
        auto expr = parse_expression(parent);
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

void Parser::parse_statements(SyntaxNode* parent, Statements& block)
{
    while (true) {
        auto statement = parse_statement(parent);
        if (!statement)
            break;
        block.push_back(statement);
    }
}

std::shared_ptr<Block> Parser::parse_block(SyntaxNode* parent, Statements& block)
{
    parse_statements(parent, block);
    if (!expect(TokenCode::CloseBrace)) {
        return nullptr;
    }
    return make_node<Block>(parent, block);
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
    return make_node<FunctionCall>(function, args);
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
        Statements statements;
        parse_block(parent, statements);
        return make_node<FunctionDef>(parent, name_maybe.value().value(), params, statements);
    }
    case KeywordLink: {
        lex();
        if (auto link_target_maybe = match(TokenCode::DoubleQuotedString, "after '->'"); link_target_maybe.has_value()) {
            return make_node<NativeFunctionDef>(parent, name_maybe.value().value(), params, link_target_maybe.value().value());
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
    ElifStatements elifs;
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
            elifs.push_back(std::make_shared<ElifStatement>(elif_condition, elif_stmt));
        } break;
        case KeywordElse: {
            lex();
            auto else_stmt = parse_statement(parent);
            if (!else_stmt)
                return nullptr;
            return make_node<IfStatement>(condition, if_stmt, elifs, std::make_shared<ElseStatement>(else_stmt));
        }
        default:
            return make_node<IfStatement>(condition, if_stmt, elifs, nullptr);
        }
    }
}

std::shared_ptr<SwitchStatement> Parser::parse_switch_statement(SyntaxNode* parent)
{
    auto switch_expr = parse_expression(parent);
    if (!switch_expr)
        return nullptr;
    if (!expect(TokenCode::OpenBrace, "after switch expression")) {
        return nullptr;
    }
    CaseStatements cases;
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
            cases.push_back(std::make_shared<CaseStatement>(expr, stmt));
        } break;
        case KeywordDefault: {
            auto default_token = lex();
            if (!expect(TokenCode::Colon, "after 'default' keyword")) {
                return nullptr;
            }
            auto stmt = parse_statement(parent);
            if (!stmt)
                return nullptr;
            return make_node<SwitchStatement>(parent, switch_expr, cases, std::make_shared<DefaultCase>(stmt));
        }
        case TokenCode::CloseBrace:
            lex();
            return make_node<SwitchStatement>(parent, switch_expr, cases, nullptr);
        default:
            add_error(peek(), "Syntax Error: Unexpected token '{}' in switch statement");
            return nullptr;
        }
    }
}

std::shared_ptr<WhileStatement> Parser::parse_while_statement(SyntaxNode* parent)
{
    if (!expect(TokenCode::OpenParen, " in 'while' statement"))
        return nullptr;
    auto condition = parse_expression(parent);
    if (!condition)
        return nullptr;
    if (!expect(TokenCode::CloseParen, " in 'while' statement"))
        return nullptr;
    auto stmt = parse_statement(parent);
    if (!stmt)
        return nullptr;
    return make_node<WhileStatement>(parent, condition, stmt);
}

std::shared_ptr<ForStatement> Parser::parse_for_statement(SyntaxNode* parent)
{
    if (!expect(TokenCode::OpenParen, " in 'for' statement"))
        return nullptr;
    auto variable = match(TokenCode::Identifier, " in 'for' ststement");
    if (!variable.has_value())
        return nullptr;
    if (!expect(KeywordIn, " in 'for' statement"))
        return nullptr;
    auto expr = parse_expression(parent);
    if (!expr)
        return nullptr;
    if (!expect(TokenCode::CloseParen, " in 'for' statement"))
        return nullptr;
    auto stmt = parse_statement(parent);
    if (!stmt)
        return nullptr;
    return make_node<ForStatement>(parent, variable.value().value(), expr, stmt);
}

std::shared_ptr<VariableDeclaration> Parser::parse_variable_declaration(SyntaxNode* parent)
{
    auto identifier_maybe = match(TokenCode::Identifier);
    if (!identifier_maybe.has_value()) {
        return nullptr;
    }
    if (current_code() != TokenCode::Equals) {
        return make_node<VariableDeclaration>(parent, identifier_maybe.value().value());
    }
    lex();
    auto expr = parse_expression(parent);
    if (!expr)
        return nullptr;
    return make_node<VariableDeclaration>(parent, identifier_maybe.value().value(), expr);
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
    return make_node<Import>(parent, module_name);
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
        return 14;
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
            rhs = parse_primary_expression(lhs->parent(), op.code() == TokenCode::Period);
            if (!rhs)
                return nullptr;
            while (binary_precedence(current_code()) > binary_precedence(op.code())) {
                rhs = parse_expression_1(rhs, binary_precedence(op.code()) + 1);
            }
            if (is_postfix_unary_operator(current_code()) && (unary_precedence(current_code()) > binary_precedence(op.code()))) {
                lhs = parse_postfix_unary_operator(lhs);
            }
        } else {
            rhs = parse_expression(lhs->parent());
        }
        lhs = make_node<BinaryExpression>(lhs, op, rhs);
    }
    if (is_postfix_unary_operator(current_code())) {
        lhs = parse_postfix_unary_operator(lhs);
    }
    return lhs;
}

std::shared_ptr<Expression> Parser::parse_postfix_unary_operator(std::shared_ptr<Expression> expression)
{
    assert(is_postfix_unary_operator(current_code()));
    switch (current_code()) {
    case TokenCode::OpenParen:
        // rhs := function call with rhs as function expr.
        return parse_function_call(move(expression));
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
        return make_node<UnaryExpression>(t, operand);
    }
    case TokenCode::OpenBracket:
        return parse_list_literal(parent);
    case TokenCode::OpenBrace:
        return parse_dictionary_literal(parent);
    case TokenCode::Integer:
    case TokenCode::Float:
    case TokenCode::DoubleQuotedString:
    case TokenCode::SingleQuotedString:
    case KeywordTrue:
    case KeywordFalse:
        return make_node<Literal>(parent, t);
    case TokenCode::Identifier: {
        if (in_deref_chain)
            return make_node<Identifier>(parent, t.value());
        return make_node<BinaryExpression>(make_node<This>(parent), Token(TokenCode::Period, "."), make_node<Identifier>(parent, t.value()));
    }
    default:
        add_error(t, format("Syntax Error: Expected literal or variable, got '{}' ({})", t.value(), t.code_name()));
        return nullptr;
    }
}

std::shared_ptr<Expression> Parser::parse_list_literal(SyntaxNode* parent)
{
    std::vector<std::shared_ptr<Expression>> elements;
    while (current_code() != TokenCode::CloseBracket) {
        auto element = parse_expression(parent);
        if (!element)
            return nullptr;
        if (elements.empty() && (current_code() == KeywordFor)) {
            lex();
            return parse_list_comprehension(element);
        }
        elements.push_back(element);
        if (current_code() == TokenCode::Comma) {
            lex();
        } else if (current_code() != TokenCode::CloseBracket) {
            add_error(peek(), format("Expecting ',' after list element, got '{}'", peek().value()));
            return nullptr;
        }
    }
    lex();
    return make_node<ListLiteral>(parent, elements);
}

std::shared_ptr<ListComprehension> Parser::parse_list_comprehension(std::shared_ptr<Expression> element)
{
    auto rangevar_maybe = match(TokenCode::Identifier, "after 'for' in list comprehension");
    if (!rangevar_maybe.has_value())
        return nullptr;
    if (!expect(KeywordIn, "after range variable in list comprehensiom"))
        return nullptr;
    auto generator = parse_expression(element->parent());
    if (!generator)
        return nullptr;
    if (current_code() != KeywordWhere) {
        if (!expect(TokenCode::CloseBracket, "after generator expression in list comprehension"))
            return nullptr;
        return make_node<ListComprehension>(element, rangevar_maybe.value().value(), generator);
    }
    lex();
    auto where = parse_expression(element->parent());
    if (!expect(TokenCode::CloseBracket, "after generator condition in list comprehension"))
        return nullptr;
    return make_node<ListComprehension>(element, rangevar_maybe.value().value(), generator, where);
}

std::shared_ptr<DictionaryLiteral> Parser::parse_dictionary_literal(SyntaxNode* parent)
{
    DictionaryLiteralEntries entries;
    while (current_code() != TokenCode::CloseBrace) {
        auto name = match(TokenCode::Identifier, "Expecting entry name in dictionary literal");
        if (!name.has_value())
            return nullptr;
        if (!expect(TokenCode::Colon, "in dictionary literal"))
            return nullptr;
        auto value = parse_expression(parent);
        if (!value)
            return nullptr;
        entries.push_back({ name.value().value(), value });
        if (current_code() == TokenCode::Comma) {
            lex();
        } else if (current_code() != TokenCode::CloseBrace) {
            add_error(peek(), format("Expecting ',' after dictionary element, got '{}'", peek().value()));
            return nullptr;
        }
    }
    lex();
    return make_node<DictionaryLiteral>(parent, entries);
}

Token const& Parser::peek()
{
    static Token s_eof(TokenCode::EndOfFile, "EOF triggered by lexer error");
    auto& ret = m_lexer.peek();
    debug(parser, "Parser::peek(): {}", ret.to_string());
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
    debug(parser, "Parser::lex(): {}", ret.to_string());
    if (ret.code() != TokenCode::Error)
        return ret;
    add_error(ret, ret.value());
    return s_eof;
}

std::optional<Token const> Parser::match(TokenCode code, char const* where)
{
    debug(parser, "Parser::match({})", TokenCode_name(code));
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
    debug(parser, "Parser::expect({})", TokenCode_name(code));
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

void Parser::add_error(Token const& token, std::string const& message)
{
    debug(parser, "Parser::add_error({}, '{}')", token.to_string(), message);
    m_errors.emplace_back(message, m_file_name, token);
}

}
