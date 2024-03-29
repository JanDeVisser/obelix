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

ErrorOr<std::shared_ptr<Parser>, SystemError> Parser::create(ParserContext& ctx, std::string const& file_name)
{
    auto ret = std::shared_ptr<Parser>(new Parser(ctx));
    TRY_RETURN(ret->read_file(file_name, new ObelixBufferLocator(ctx.config)));
    ret->m_current_module = sanitize_module_name(file_name);
    return ret;
}

Parser::Parser(ParserContext& ctx)
    : BasicParser()
    , m_ctx(ctx)
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
        Token(KeywordStatic, "static"),
        Token(KeywordEnum, "enum"),
        Token(KeywordGlobal, "global"),
        Token(KeywordExtend, "extend"),
        Token(KeywordAs, "as"),
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
        return std::make_shared<Pass>(lex().location());
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
        return std::make_shared<Pass>(lex().location());
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
        return std::make_shared<Return>(token.location(), expr);
    }
    case TokenCode::Identifier: {
        if (token.value() == "error") {
            lex();
            auto expr = parse_expression();
            if (!expr)
                return nullptr;
            return std::make_shared<Return>(token.location(), expr, true);
        }
        break;
    }
    case KeywordBreak:
        return std::make_shared<Break>(lex().location());
    case KeywordContinue:
        return std::make_shared<Continue>(lex().location());
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
    return std::make_shared<Block>(token.location(), block);
}

std::shared_ptr<Statement> Parser::parse_function_definition(Token const& func_token)
{
    auto name_maybe = match(TokenCode::Identifier);
    if (!name_maybe.has_value()) {
        add_error(peek(), "Expecting variable name after the 'func' keyword, got '{}'");
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
            add_error(peek(), "Expected parameter name, got '{}'");
            return nullptr;
        }
        auto param_name = param_name_maybe.value();
        if (!expect(TokenCode::Colon))
            return nullptr;

        auto param_type = parse_type();
        if (!param_type) {
            add_error(peek(), "Syntax Error: Expected type name for parameter {}, got '{}'", param_name_maybe.value(), peek().value());
            return nullptr;
        }
        params.push_back(std::make_shared<Identifier>(param_name.location(), param_name.value(), param_type));
        switch (current_code()) {
        case TokenCode::Comma:
            lex();
            break;
        case TokenCode::CloseParen:
            done = true;
            break;
        default:
            add_error(peek(), "Syntax Error: Expected ',' or ')' in function parameter list, got '{}'", peek().value());
            return nullptr;
        }
    }
    lex(); // Eat the closing paren

    if (!expect(TokenCode::Colon))
        return nullptr;

    auto type = parse_type();
    if (!type) {
        add_error(peek(), "Syntax Error: Expected return type name, got '{}'", peek().value());
        return nullptr;
    }

    auto func_ident = std::make_shared<Identifier>(name.location(), name.value(), type);
    std::shared_ptr<FunctionDecl> func_decl;
    if (current_code() == KeywordLink) {
        lex();
        if (auto link_target_maybe = match(TokenCode::DoubleQuotedString, "after '->'"); link_target_maybe.has_value()) {
            return std::make_shared<NativeFunctionDecl>(name.location(), m_current_module, func_ident, params, link_target_maybe.value().value());
        }
        return nullptr;
    }
    if (func_token.code() == KeywordIntrinsic) {
        return std::make_shared<IntrinsicDecl>(name.location(), m_current_module, func_ident, params);
    }
    func_decl = std::make_shared<FunctionDecl>(name.location(), m_current_module, func_ident, params);
    auto stmt = parse_statement();
    if (stmt == nullptr)
        return nullptr;
    return std::make_shared<FunctionDef>(func_token.location(), func_decl, stmt);
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
            branches.push_back(std::make_shared<Branch>(elif_token.location(), elif_condition, elif_stmt));
        } break;
        case KeywordElse: {
            auto else_token = lex();
            auto else_stmt = parse_statement();
            if (!else_stmt)
                return nullptr;
            return std::make_shared<IfStatement>(if_token.location(), condition, if_stmt, branches, else_stmt);
        }
        default:
            return std::make_shared<IfStatement>(if_token.location(), condition, if_stmt, branches, nullptr);
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
            cases.push_back(std::make_shared<CaseStatement>(case_token.location(), expr, stmt));
        } break;
        case KeywordDefault: {
            auto default_token = lex();
            if (!expect(TokenCode::Colon, "after 'default' keyword")) {
                return nullptr;
            }
            auto stmt = parse_statement();
            if (!stmt)
                return nullptr;
            default_case = std::make_shared<DefaultCase>(default_token.location(), stmt);
            break;
        }
        case TokenCode::CloseBrace:
            lex();
            return std::make_shared<SwitchStatement>(switch_token.location(), switch_expr, cases, default_case);
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
    return std::make_shared<WhileStatement>(while_token.location(), condition, stmt);
}

std::shared_ptr<ForStatement> Parser::parse_for_statement(Token const& for_token)
{
    if (!expect(TokenCode::OpenParen, " in 'for' statement"))
        return nullptr;
    auto variable = match(TokenCode::Identifier, " in 'for' statement");
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
    auto variable_node = std::make_shared<Variable>(variable.value().location(), variable.value().value(), type);
    return std::make_shared<ForStatement>(for_token.location(), variable_node, expr, stmt);
}

std::shared_ptr<Statement> Parser::parse_struct(Token const& struct_token)
{
    auto identifier_maybe = match(TokenCode::Identifier);
    if (!identifier_maybe.has_value())
        return nullptr;
    auto name = identifier_maybe->value();
    if (current_code() != TokenCode::OpenBrace)
        return std::make_shared<StructForward>(lex().location(), name);
    lex();

    Identifiers fields;
    FunctionDefs  methods;
    do {
        switch (current_code()) {
        case TokenCode::Identifier: {
            auto field_name = lex();
            expect(TokenCode::Colon);
            auto field_type = parse_type();
            if (!field_type) {
                add_error(peek(), "Syntax Error: Expected type after ':', got '{}' ({})", peek().value(), peek().code_name());
                return nullptr;
            }
            fields.push_back(std::make_shared<Identifier>(field_name.location(), field_name.value(), field_type));
            break;
        }
        case Parser::KeywordFunc: {
            auto func_def = parse_function_definition(lex());
            if (func_def == nullptr)
                return nullptr;
            methods.push_back(std::dynamic_pointer_cast<FunctionDef>(func_def));
            break;
        }
        default:
            return nullptr;
        }
    } while (current_code() != TokenCode::CloseBrace);
    lex();
    return std::make_shared<StructDefinition>(struct_token.location(), name, fields, methods);
}

std::shared_ptr<VariableDeclaration> Parser::parse_static_variable_declaration()
{
    auto code = current_code();
    switch (code) {
    case KeywordVar:
    case KeywordConst:
        return parse_variable_declaration(lex(), code == KeywordConst, VariableKind::Static);
    default:
        add_error(peek(), "Syntax Error: Expected 'const' or 'var' after 'static', got '{}' ({})", peek().value(), peek().code_name());
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
        add_error(peek(), "Syntax Error: Expected 'const' or 'var' after 'static', got '{}' ({})", peek().value(), peek().code_name());
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
            add_error(peek(), "Syntax Error: Expected type after ':', got '{}' ({})", peek().value(), peek().code_name());
            return nullptr;
        }
        type = var_type;
    }
    auto var_ident = std::make_shared<Identifier>(identifier.location(), identifier.value(), type);
    std::shared_ptr<Expression> expr { nullptr };
    if (current_code() == TokenCode::Equals) {
        lex();
        expr = parse_expression();
        if (!expr)
            return nullptr;
    } else {
        if (constant) {
            add_error(peek(), "Syntax Error: Expected expression after constant declaration, got '{}' ({})", peek().value(), peek().code_name());
            return nullptr;
        }
    }
    switch (variable_kind) {
    case VariableKind::Local:
        return std::make_shared<VariableDeclaration>(var_token.location(), var_ident, expr, constant);
    case VariableKind::Static:
        return std::make_shared<StaticVariableDeclaration>(var_token.location(), var_ident, expr, constant);
    case VariableKind::ModuleLocal:
        return std::make_shared<LocalVariableDeclaration>(var_token.location(), var_ident, expr, constant);
    case VariableKind::Global:
        return std::make_shared<GlobalVariableDeclaration>(var_token.location(), var_ident, expr, constant);
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
    return std::make_shared<Import>(import_token.location(), module_name);
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
 * Precedences according to https://en.cppreference.com/w/c/language/operator_precedence:
 */

template<TokenCode Count>
class OperatorDefs {
public:
    OperatorDefs()
    {
        for (auto ix = 0u; ix < (int) Count; ++ix) {
            defs_by_code[ix] = { (TokenCode) ix, OperandKind::None, OperandKind::None, -1, OperandKind::None, -1 };
        }
        for (auto const& def : operators) {
            if (def.op == TokenCode::Unknown)
                break;
            defs_by_code[(int)def.op] = { def.op, def.lhs_kind, def.rhs_kind, def.precedence, def.unary_kind, def.unary_precedence };
        }
    }

    OperatorDef const& find(TokenCode code) const
    {
        int c = static_cast<int>(code);
        if (code >= TokenCode::count) {
            return defs_by_code[0];
        }
        assert(defs_by_code[c].op == code);
        return defs_by_code[c];
    }

    bool is_binary(TokenCode code) const
    {
        return find(code).lhs_kind != OperandKind::None;
    }

    bool is_unary(TokenCode code) const
    {
        return find(code).unary_kind != OperandKind::None;
    }

    int binary_precedence(TokenCode code) const
    {
        return find(code).precedence;
    }

    int unary_precedence(TokenCode code) const
    {
        return find(code).unary_precedence;
    }

    Associativity associativity(TokenCode code)
    {
        switch (code) {
        case TokenCode::Equals:
        case Parser::KeywordIncEquals:
        case Parser::KeywordDecEquals:
            return Associativity::RightToLeft;
        default:
            return Associativity::LeftToRight;
        }
    }

private:
    OperatorDef operators[(int)Count] = {
        { TokenCode::Equals, OperandKind::Value, OperandKind::Value, 1 },
        { Parser::KeywordIncEquals, OperandKind::Value, OperandKind::Value, 1 },
        { Parser::KeywordDecEquals, OperandKind::Value, OperandKind::Value, 1 },
        { TokenCode::LogicalOr, OperandKind::Value, OperandKind::Value, 3 },
        { TokenCode::LogicalAnd, OperandKind::Value, OperandKind::Value, 4 },
        { TokenCode::Pipe, OperandKind::Value, OperandKind::Value, 5 },
        { TokenCode::Hat, OperandKind::Value, OperandKind::Value, 6 },
        { TokenCode::Ampersand, OperandKind::Value, OperandKind::Value, 7 },
        { TokenCode::EqualsTo, OperandKind::Value, OperandKind::Value, 8 },
        { TokenCode::NotEqualTo, OperandKind::Value, OperandKind::Value, 8 },
        { Parser::KeywordRange, OperandKind::Value, OperandKind::Value, 8 },
        { TokenCode::GreaterThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::LessThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::GreaterEqualThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::LessEqualThan, OperandKind::Value, OperandKind::Value, 9 },
        { TokenCode::ShiftLeft, OperandKind::Value, OperandKind::Value, 10 },
        { TokenCode::ShiftRight, OperandKind::Value, OperandKind::Value, 10 },
        { TokenCode::Plus, OperandKind::Value, OperandKind::Value, 11, OperandKind::Value, 13 },
        { TokenCode::Minus, OperandKind::Value, OperandKind::Value, 11, OperandKind::Value, 13 },
        { TokenCode::Asterisk, OperandKind::Value, OperandKind::Value, 12, OperandKind::Value, 13 },
        { TokenCode::Slash, OperandKind::Value, OperandKind::Value, 12 },
        { TokenCode::Percent, OperandKind::Value, OperandKind::Value, 12 },
        { TokenCode::Tilde, OperandKind::None, OperandKind::None, -1, OperandKind::Value, 13 },
        { TokenCode::ExclamationPoint, OperandKind::None, OperandKind::None, -1, OperandKind::Value, 13 },
        { TokenCode::AtSign, OperandKind::None, OperandKind::None, -1, OperandKind::Value, 13 },
        { TokenCode::Period, OperandKind::Value, OperandKind::Value, 14, OperandKind::Value, 14 },
        { TokenCode::OpenBracket, OperandKind::Value, OperandKind::Value, 14 },
        { TokenCode::OpenParen, OperandKind::Value, OperandKind::Value, 14 },
        { Parser::KeywordAs, OperandKind::Value, OperandKind::Type, 14 },
        { TokenCode::CloseBracket, OperandKind::Value, OperandKind::Value, -1 },
        { TokenCode::Unknown, OperandKind::None, OperandKind::None, -1 },
    };

    OperatorDef defs_by_code[(int)Count] = {
        { TokenCode::Unknown, OperandKind::None, OperandKind::None, -1, OperandKind::None, -1 }
    };
};

static OperatorDefs<TokenCode::count> operator_defs;

std::shared_ptr<Expression> Parser::parse_expression_1(std::shared_ptr<Expression> lhs, int min_precedence)
{
    while (operator_defs.is_binary(current_code()) && operator_defs.binary_precedence(current_code()) >= min_precedence) {
        auto op = lex();
        std::shared_ptr<Expression> rhs;
        if (operator_defs.associativity(op.code()) == Associativity::LeftToRight) {
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
                rhs = std::make_shared<ExpressionList>(op.location(), expressions);
                break;
            }
            default: {
                switch (operator_defs.find(op.code()).rhs_kind) {
                case OperandKind::Type: {
                    auto type = parse_type();
                    if (type == nullptr) {
                        return nullptr;
                    }
                    return std::make_shared<CastExpression>(lhs->location(), lhs, type);
                }
                default:
                    rhs = parse_primary_expression();
                    if (!rhs)
                        return nullptr;
                    while ((open_bracket && (current_code() != TokenCode::CloseBracket)) || (operator_defs.binary_precedence(current_code()) > operator_defs.binary_precedence(op.code())))
                        rhs = parse_expression_1(rhs, (open_bracket) ? 0 : (operator_defs.binary_precedence(op.code()) + 1));
                    break;
                }
                break;
            }
            }
            if (open_bracket) {
                if (!expect(TokenCode::CloseBracket))
                    return nullptr;
            }
        } else {
            rhs = parse_expression();
        }
        lhs = std::make_shared<BinaryExpression>(lhs, op, rhs);
    }

    // Pull up unary expressions with lower precedence than the binary we just parsed.
    // This is for cases like @var.error.
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(lhs); binary != nullptr) {
        if (auto lhs_unary = std::dynamic_pointer_cast<UnaryExpression>(binary->lhs()); lhs_unary != nullptr && operator_defs.unary_precedence(lhs_unary->op().code()) < operator_defs.binary_precedence(binary->op().code())) {
            auto pushed_down = std::make_shared<BinaryExpression>(lhs_unary->operand(), binary->op(), binary->rhs());
            lhs = std::make_shared<UnaryExpression>(lhs_unary->op(), pushed_down);
        }
    }
    return lhs;
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
            ? std::make_shared<IntLiteral>(t, std::make_shared<ExpressionType>(t.location(), type_mnemonic))
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
            add_error(t, "Syntax Error: Single-quoted string should only hold a single character, not '{}'", t.value());
            return nullptr;
        }
        expr = std::make_shared<CharLiteral>(t);
        break;
    case KeywordTrue:
    case KeywordFalse:
        expr = std::make_shared<BooleanLiteral>(t);
        break;
    case TokenCode::Identifier:
        expr = std::make_shared<Variable>(t.location(), t.value());
        break;
    default:
        if (operator_defs.is_unary(t.code())) {
            auto operand = parse_primary_expression();
            if (!operand)
                return nullptr;
            expr = std::make_shared<UnaryExpression>(t, operand);
            break;
        }
        add_error(t, "Syntax Error: Expected literal or variable, got '{}' ({})", t.value(), t.code_name());
        return nullptr;
    }
    return expr;
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
                arguments.push_back(std::make_shared<StringTemplateArgument>(token.location(), token.value()));
                break;
            }
            case TokenCode::Integer:
            case TokenCode::HexNumber: {
                auto token = lex();
                arguments.push_back(std::make_shared<IntegerTemplateArgument>(token.location(), token_value<long>(token).value()));
                break;
            }
            case TokenCode::Identifier: {
                auto parameter = parse_type();
                if (!parameter) {
                    add_error(peek(), "Syntax Error: Expected type, got '{}' ({})", peek().value(), peek().code_name());
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
                return std::make_shared<ExpressionType>(lt_token.location(), type_name, arguments);
            }
            if (current_code() == TokenCode::ShiftRight) {
                replace(Token { TokenCode::GreaterThan, ">" });
                return std::make_shared<ExpressionType>(lt_token.location(), type_name, arguments);
            }
            if (!expect(TokenCode::Comma))
                return nullptr;
        }
    }
    case TokenCode::Slash: {
        auto success_type = std::make_shared<ExpressionType>(type_token.location(), type_name);
        auto slash = lex();
        if (current_code() != TokenCode::Identifier) {
            add_error(peek(), "Syntax Error: Expected type, got '{}' ({})", peek().value(), peek().code_name());
            return nullptr;
        }
        auto error_type = parse_type();
        if (error_type == nullptr)
            return nullptr;
        return std::make_shared<ExpressionType>(slash.location(), "conditional", TemplateArgumentNodes { success_type, error_type });
    }
    default:
        return std::make_shared<ExpressionType>(type_token.location(), type_name);
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
        add_error(peek(), "Expecting enumeration name after the 'enum' keyword, got '{}'");
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
        values.push_back(std::make_shared<EnumValue>(value_label.location(), value_label.value(), value_value));
    }
    lex(); // Eat the closing brace
    return std::make_shared<EnumDef>(enum_token.location(), name.value(), values, extend);
}

std::shared_ptr<TypeDef> Parser::parse_type_definition(Token const& type_token)
{
    auto name_maybe = match(TokenCode::Identifier);
    if (!name_maybe.has_value()) {
        add_error(peek(), "Expecting type alias after the 'type' keyword, got '{}'");
        return nullptr;
    }
    (void)skip(TokenCode::Equals);
    auto type = parse_type();
    if (type == nullptr)
        return nullptr;
    return std::make_shared<TypeDef>(type_token.location(), name_maybe.value().value(), type);
}
}
