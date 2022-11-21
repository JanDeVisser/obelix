/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include <obelix/Syntax.h>
#include <obelix/parser/Parser.h>

namespace Obelix {

logging_category(parser);

Parser::Parser(ParserContext& ctx, std::string const& file_name)
    : BasicParser(file_name, new ObelixBufferLocator(ctx.config))
    , m_ctx(ctx)
{
    m_current_module = sanitize_module_name(file_name);
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
        Token(KeywordStatic, "static"),
        Token(KeywordEnum, "enum"),
        Token(KeywordGlobal, "global"),
        Token(KeywordExtend, "extend"),
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

std::shared_ptr<Module> Parser::parse()
{
    if (!errors().empty())
        return nullptr;
    Statements statements;
    parse_statements(statements, true);
    if (has_errors())
        return nullptr;
    return std::make_shared<Module>(statements, m_current_module);
}

std::shared_ptr<Statement> Parser::parse_top_level_statement()
{
    debug(parser, "Parser::parse_top_level_statement");
    auto token = peek();
    std::shared_ptr<Statement> ret;
    switch (token.code()) {
    case TokenCode::SemiColon:
        return std::make_shared<Pass>(lex());
    case TokenCode::OpenBrace: {
        lex();
        Statements statements;
        return parse_block(statements);
    }
    case KeywordImport:
        return parse_import_statement(lex());
    case KeywordStruct:
        return parse_struct(lex());
    case KeywordGlobal:
        lex();
        return parse_global_variable_declaration();
    case KeywordVar:
    case KeywordConst:
        return parse_variable_declaration(lex(), token.code() == KeywordConst, VariableKind::ModuleLocal);
    case KeywordFunc:
    case KeywordIntrinsic:
        return parse_function_definition(lex());
    case KeywordEnum:
        return parse_enum_definition(lex());
    case TokenCode::Identifier: {
        if (token.value() == "type") {
            return parse_type_definition(lex());
        }
        break;
    }
    case TokenCode::CloseBrace:
    case TokenCode::EndOfFile:
        return nullptr;
    default:
        break;
    }
    auto expr = parse_expression();
    if (!expr)
        return nullptr;
    return std::make_shared<ExpressionStatement>(expr);
}

std::shared_ptr<Statement> Parser::parse_statement()
{
    debug(parser, "Parser::parse_statement");
    auto token = peek();
    std::shared_ptr<Statement> ret;
    switch (token.code()) {
    case TokenCode::SemiColon:
        return std::make_shared<Pass>(lex());
    case TokenCode::OpenBrace: {
        lex();
        Statements statements;
        return parse_block(statements);
    }
    case KeywordImport:
        return parse_import_statement(lex());
    case KeywordIf:
        return parse_if_statement(lex());
    case KeywordSwitch:
        return parse_switch_statement(lex());
    case KeywordWhile:
        return parse_while_statement(lex());
    case KeywordFor:
        return parse_for_statement(lex());
    case KeywordStatic:
        lex();
        return parse_static_variable_declaration();
    case KeywordVar:
    case KeywordConst:
        return parse_variable_declaration(lex(), token.code() == KeywordConst);
    case KeywordReturn: {
        lex();
        auto expr = parse_expression();
        if (!expr)
            return nullptr;
        return std::make_shared<Return>(token, expr);
    }
    case TokenCode::Identifier: {
        if (token.value() == "error") {
            lex();
            auto expr = parse_expression();
            if (!expr)
                return nullptr;
            return std::make_shared<Return>(token, expr, true);
        }
        break;
    }
    case KeywordBreak:
        return std::make_shared<Break>(lex());
    case KeywordContinue:
        return std::make_shared<Continue>(lex());
    case TokenCode::CloseBrace:
    case TokenCode::EndOfFile:
        return nullptr;
    default: {
        break;
    }
    }
    auto expr = parse_expression();
    if (!expr)
        return nullptr;
    return std::make_shared<ExpressionStatement>(expr);
}

void Parser::parse_statements(Statements& block, bool top_level)
{
    while (true) {
        auto statement = (top_level) ? parse_top_level_statement() : parse_statement();
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
    return std::make_shared<Block>(token, block);
}

std::shared_ptr<Statement> Parser::parse_function_definition(Token const& func_token)
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
            return std::make_shared<NativeFunctionDecl>(name, func_ident, params, link_target_maybe.value().value());
        }
        return nullptr;
    }
    if (func_token.code() == KeywordIntrinsic) {
        return std::make_shared<IntrinsicDecl>(name, func_ident, params);
    }
    func_decl = std::make_shared<FunctionDecl>(name, func_ident, params);
    auto stmt = parse_statement();
    if (stmt == nullptr)
        return nullptr;
    return std::make_shared<FunctionDef>(func_token, func_decl, stmt);
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
            return std::make_shared<IfStatement>(if_token, condition, if_stmt, branches, else_stmt);
        }
        default:
            return std::make_shared<IfStatement>(if_token, condition, if_stmt, branches, nullptr);
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
    std::shared_ptr<DefaultCase> default_case { nullptr };
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
            default_case = std::make_shared<DefaultCase>(default_token, stmt);
            break;
        }
        case TokenCode::CloseBrace:
            lex();
            return std::make_shared<SwitchStatement>(switch_token, switch_expr, cases, default_case);
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
    return std::make_shared<WhileStatement>(while_token, condition, stmt);
}

std::shared_ptr<ForStatement> Parser::parse_for_statement(Token const& for_token)
{
    if (!expect(TokenCode::OpenParen, " in 'for' statement"))
        return nullptr;
    auto variable = match(TokenCode::Identifier, " in 'for' ststement");
    if (!variable.has_value())
        return nullptr;

    std::shared_ptr<ExpressionType> type = nullptr;
    if (current_code() == TokenCode::Colon) {
        lex();
        type = parse_type();
    }
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
    auto variable_node = std::make_shared<Variable>(variable.value(), variable.value().value(), type);
    return std::make_shared<ForStatement>(for_token, variable_node, expr, stmt);
}

std::shared_ptr<Statement> Parser::parse_struct(Token const& struct_token)
{
    auto identifier_maybe = match(TokenCode::Identifier);
    if (!identifier_maybe.has_value())
        return nullptr;
    auto name = identifier_maybe->value();
    if (current_code() != TokenCode::OpenBrace)
        return std::make_shared<StructForward>(lex(), name);
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
        fields.push_back(std::make_shared<Identifier>(field_name_maybe.value(), field_name_maybe.value().value(), field_type));
    } while (current_code() != TokenCode::CloseBrace);
    lex();
    return std::make_shared<StructDefinition>(struct_token, name, fields);
}

std::shared_ptr<VariableDeclaration> Parser::parse_static_variable_declaration()
{
    auto code = current_code();
    switch (code) {
    case KeywordVar:
    case KeywordConst:
        return parse_variable_declaration(lex(), code == KeywordConst, VariableKind::Static);
    default:
        add_error(peek(), format("Syntax Error: Expected 'const' or 'var' after 'static', got '{}' ({})", peek().value(), peek().code_name()));
        return nullptr;
    }
}

std::shared_ptr<VariableDeclaration> Parser::parse_global_variable_declaration()
{
    auto code = current_code();
    switch (code) {
    case KeywordVar:
    case KeywordConst:
        return parse_variable_declaration(lex(), code == KeywordConst, VariableKind::Global);
    default:
        add_error(peek(), format("Syntax Error: Expected 'const' or 'var' after 'static', got '{}' ({})", peek().value(), peek().code_name()));
        return nullptr;
    }
}

std::shared_ptr<VariableDeclaration> Parser::parse_variable_declaration(Token const& var_token, bool constant, VariableKind variable_kind)
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
    std::shared_ptr<Expression> expr { nullptr };
    if (current_code() == TokenCode::Equals) {
        lex();
        expr = parse_expression();
        if (!expr)
            return nullptr;
    } else {
        if (constant) {
            add_error(peek(), format("Syntax Error: Expected expression after constant declaration, got '{}' ({})", peek().value(), peek().code_name()));
            return nullptr;
        }
    }
    switch (variable_kind) {
    case VariableKind::Local:
        return std::make_shared<VariableDeclaration>(var_token, var_ident, expr, constant);
    case VariableKind::Static:
        return std::make_shared<StaticVariableDeclaration>(var_token, var_ident, expr, constant);
    case VariableKind::ModuleLocal:
        return std::make_shared<LocalVariableDeclaration>(var_token, var_ident, expr, constant);
    case VariableKind::Global:
        return std::make_shared<GlobalVariableDeclaration>(var_token, var_ident, expr, constant);
    }
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
    m_ctx.modules.insert(module_name);
    return std::make_shared<Import>(import_token, module_name);
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
    case TokenCode::OpenParen:
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
            switch (op.code()) {
            case TokenCode::OpenParen: {
                Expressions expressions;
                if (current_code() != TokenCode::CloseParen) {
                    while (true) {
                        auto expr = parse_expression();
                        if (expr == nullptr)
                            return nullptr;
                        expressions.push_back(expr);
                        if (current_code() == TokenCode::CloseParen)
                            break;
                        if (!expect(TokenCode::Comma))
                            return nullptr;
                    }
                }
                lex();
                rhs = std::make_shared<ExpressionList>(op, expressions);
                break;
            }
            default: {
                rhs = parse_primary_expression();
                if (!rhs)
                    return nullptr;
                while ((open_bracket && (current_code() != TokenCode::CloseBracket)) || (binary_precedence(current_code()) > binary_precedence(op.code())))
                    rhs = parse_expression_1(rhs, (open_bracket) ? 0 : (binary_precedence(op.code()) + 1));
                break;
            }
            }
            if (open_bracket) {
                if (!expect(TokenCode::CloseBracket))
                    return nullptr;
            }
            if (is_postfix_unary_operator(current_code()) && (unary_precedence(current_code()) > binary_precedence(op.code())))
                lhs = parse_postfix_unary_operator(lhs);
        } else {
            rhs = parse_expression();
        }
        lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
    }
    if (is_postfix_unary_operator(current_code())) {
        lhs = parse_postfix_unary_operator(lhs);
    }
    return lhs;
}

std::shared_ptr<Expression> Parser::parse_postfix_unary_operator(std::shared_ptr<Expression> const& expression)
{
    assert(is_postfix_unary_operator(current_code()));
    fatal("Postfix operator '{}' not implemented yet", current_code());
}

std::shared_ptr<Expression> Parser::parse_primary_expression()
{
    std::shared_ptr<Expression> expr { nullptr };
    auto t = lex();
    switch (t.code()) {
    case TokenCode::OpenParen: {
        expr = parse_expression();
        if (!expect(TokenCode::CloseParen)) {
            return nullptr;
        }
        break;
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
        auto literal = std::dynamic_pointer_cast<Literal>(operand);
        if (literal != nullptr)
            return literal->apply(t);
        expr = std::make_shared<UnaryExpression>(t, operand);
        break;
    }
    case TokenCode::Integer:
    case TokenCode::HexNumber: {
        std::string type_mnemonic;
        debug(parser, "next after number: {}", peek());
        if (auto int_type = peek(); int_type.code() == TokenCode::Identifier) {
            if (int_type.value() == "u8" || int_type.value() == "s8" || int_type.value() == "u16" || int_type.value() == "s16" || int_type.value() == "u32" || int_type.value() == "s32" || int_type.value() == "u64" || int_type.value() == "s64") {
                type_mnemonic = int_type.value();
                lex();
            } else if (int_type.value() == "uc" || int_type.value() == "sc") {
                type_mnemonic = format("{}8", int_type.value()[0]);
                lex();
            } else if (int_type.value() == "us" || int_type.value() == "ss") {
                type_mnemonic = format("{}16", int_type.value()[0]);
                lex();
            } else if (int_type.value() == "uw" || int_type.value() == "sw") {
                type_mnemonic = format("{}32", int_type.value()[0]);
                lex();
            } else if (int_type.value() == "ul" || int_type.value() == "sl") {
                type_mnemonic = format("{}64", int_type.value()[0]);
                lex();
            }
        }
        expr = (!type_mnemonic.empty())
            ? std::make_shared<IntLiteral>(t, std::make_shared<ExpressionType>(t, type_mnemonic))
            : std::make_shared<IntLiteral>(t);
        break;
    }
    case TokenCode::Float:
        expr = std::make_shared<FloatLiteral>(t);
        break;
    case TokenCode::DoubleQuotedString:
        expr = std::make_shared<StringLiteral>(t);
        break;
    case TokenCode::SingleQuotedString:
        if (t.value().length() != 1) {
            add_error(t, format("Syntax Error: Single-quoted string should only hold a single character, not '{}'", t.value()));
            return nullptr;
        }
        expr = std::make_shared<CharLiteral>(t);
        break;
    case KeywordTrue:
    case KeywordFalse:
        expr = std::make_shared<BooleanLiteral>(t);
        break;
    case TokenCode::Identifier: {
        expr = std::make_shared<Variable>(t, t.value());
        break;
    }
    default:
        add_error(t, format("Syntax Error: Expected literal or variable, got '{}' ({})", t.value(), t.code_name()));
        return nullptr;
    }
    if (current_code() != TokenCode::Identifier || peek().value() != "as")
        return expr;
    lex();
    auto type = parse_type();
    if (type == nullptr) {
        return nullptr;
    }
    return std::make_shared<CastExpression>(t, expr, type);
}

std::shared_ptr<ExpressionType> Parser::parse_type()
{
    if (current_code() != TokenCode::Identifier)
        return nullptr;
    auto type_token = lex();
    auto type_name = type_token.value();
    switch (current_code()) {
    case TokenCode::LessThan: {
        auto lt_token = lex();
        TemplateArgumentNodes arguments;
        while (true) {
            switch (current_code()) {
            case TokenCode::DoubleQuotedString: {
                auto token = lex();
                arguments.push_back(std::make_shared<StringTemplateArgument>(token, token.value()));
                break;
            }
            case TokenCode::Integer:
            case TokenCode::HexNumber: {
                auto token = lex();
                arguments.push_back(std::make_shared<IntegerTemplateArgument>(token, token_value<long>(token).value()));
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
                return std::make_shared<ExpressionType>(lt_token, type_name, arguments);
            }
            if (current_code() == TokenCode::ShiftRight) {
                replace(Token { TokenCode::GreaterThan, ">" });
                return std::make_shared<ExpressionType>(lt_token, type_name, arguments);
            }
            if (!expect(TokenCode::Comma))
                return nullptr;
        }
    }
    case TokenCode::Slash: {
        auto success_type = std::make_shared<ExpressionType>(type_token, type_name);
        auto slash = lex();
        if (current_code() != TokenCode::Identifier) {
            add_error(peek(), format("Syntax Error: Expected type, got '{}' ({})", peek().value(), peek().code_name()));
            return nullptr;
        }
        auto error_type = parse_type();
        if (error_type == nullptr)
            return nullptr;
        return std::make_shared<ExpressionType>(slash, "conditional", TemplateArgumentNodes { success_type, error_type });
    }
    default:
        return std::make_shared<ExpressionType>(type_token, type_name);
    }
}

std::shared_ptr<EnumDef> Parser::parse_enum_definition(Token const& enum_token)
{
    auto extend { false };
    if (current_code() == KeywordExtend) {
        lex();
        extend = true;
    }
    auto name_maybe = match(TokenCode::Identifier);
    if (!name_maybe.has_value()) {
        add_error(peek(), "Syntax Error: expecting enumeration name after the 'enum' keyword, got '{}'");
        return nullptr;
    }
    auto name = name_maybe.value();
    if (!expect(TokenCode::OpenBrace, "after enum name in definition")) {
        return nullptr;
    }
    EnumValues values {};
    for (auto done = current_code() == TokenCode::CloseBrace; !done; done = current_code() == TokenCode::CloseBrace) {
        auto value_label_maybe = match(TokenCode::Identifier);
        if (!value_label_maybe.has_value()) {
            return nullptr;
        }
        auto value_label = value_label_maybe.value();
        std::optional<long> value_value {};
        if (skip(TokenCode::Equals).has_value()) {
            if (auto value_value_maybe = match(TokenCode::Integer); value_value_maybe.has_value()) {
                value_value.emplace(value_value_maybe->to_long().value());
            } else {
                add_error(peek(), "Syntax Error: Expected enum value, got '{}'");
                return nullptr;
            }
        }
        skip(TokenCode::Comma);
        values.push_back(std::make_shared<EnumValue>(value_label, value_label.value(), value_value));
    }
    lex(); // Eat the closing brace
    return std::make_shared<EnumDef>(enum_token, name.value(), values, extend);
}

std::shared_ptr<TypeDef> Parser::parse_type_definition(Token const& type_token)
{
    auto name_maybe = match(TokenCode::Identifier);
    if (!name_maybe.has_value()) {
        add_error(peek(), "Syntax Error: expecting type alias after the 'type' keyword, got '{}'");
        return nullptr;
    }
    (void) skip(TokenCode::Equals);
    auto type = parse_type();
    if (type == nullptr)
        return nullptr;
    return std::make_shared<TypeDef>(type_token, name_maybe.value().value(), type);
}

}
