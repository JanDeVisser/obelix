/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/syntax/Literal.h>

namespace Obelix {

extern_logging_category(parser);

// -- BooleanLiteral --------------------------------------------------------

BooleanLiteral::BooleanLiteral(Token t, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(t), std::move(type))
{
}

std::shared_ptr<Expression> BooleanLiteral::apply(Token const& t)
{
    if (t.code() == TokenCode::ExclamationPoint) {
        auto val_maybe = token().to_bool();
        if (!val_maybe.has_value())
            fatal("Cannot negate {}", token().value());
        return std::make_shared<BooleanLiteral>(Token { token().code(), Obelix::to_string(!(val_maybe.value())) }, type());
    }
    return Literal::apply(t);
}

// -- CharLiteral -----------------------------------------------------------

CharLiteral::CharLiteral(Token t, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(t), std::move(type))
{
}

// -- FloatLiteral ----------------------------------------------------------

FloatLiteral::FloatLiteral(Token t, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(t), std::move(type))
{
}

std::shared_ptr<Expression> FloatLiteral::apply(Token const& t)
{
    switch (t.code()) {
    case TokenCode::Minus: {
        auto val_maybe = token().to_double();
        if (!val_maybe.has_value())
            fatal("Cannot negate {}", token().value());
        return std::make_shared<FloatLiteral>(Token { token().code(), Obelix::to_string(-(val_maybe.value())) }, type());
    }
    case TokenCode::Plus:
        return std::dynamic_pointer_cast<Expression>(shared_from_this());
    default:
        return Literal::apply(t);
    }
}

// -- IntLiteral ------------------------------------------------------------

IntLiteral::IntLiteral(Token t, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(t), std::move(type))
{
}

std::shared_ptr<Expression> IntLiteral::apply(Token const& t)
{
    switch (t.code()) {
    case TokenCode::Minus: {
        auto val_maybe = token().to_long();
        if (!val_maybe.has_value())
            fatal("Cannot negate {}", token().value());
        return std::make_shared<IntLiteral>(Token { token().code(), Obelix::to_string(-(val_maybe.value())) }, type());
    }
    case TokenCode::Plus:
        return std::dynamic_pointer_cast<Expression>(shared_from_this());
    case TokenCode::Tilde: {
        auto val_maybe = token().to_long();
        if (!val_maybe.has_value())
            fatal("Cannot bitwise invert {}", token().value());
        return std::make_shared<IntLiteral>(Token { token().code(), Obelix::to_string(~(val_maybe.value())) }, type());
    }
    default:
        return Literal::apply(t);
    }
}

// -- Literal ---------------------------------------------------------------

Literal::Literal(Token t, std::shared_ptr<ExpressionType> type)
    : Expression(std::move(t), std::move(type))
{
}

std::string Literal::attributes() const
{
    return format(R"(value="{}" type="{}")", token().value(), type_name());
}

std::string Literal::to_string() const
{
    if (type() != nullptr)
        return format("{}: {}", token().value(), type()->to_string());
    return token().value();
}

std::shared_ptr<Expression> Literal::apply(Token const& token)
{
    return std::make_shared<UnaryExpression>(token, std::dynamic_pointer_cast<Expression>(shared_from_this()));
}

// -- StringLiteral ---------------------------------------------------------

StringLiteral::StringLiteral(Token t, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(t), std::move(type))
{
}

}
