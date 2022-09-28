/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/syntax/Literal.h>

namespace Obelix {

extern_logging_category(parser);

// -- BooleanLiteral --------------------------------------------------------

BooleanLiteral::BooleanLiteral(Token const& t)
    : Literal(t, ObjectType::get("bool"))
{
}

BooleanLiteral::BooleanLiteral(Token const& t, std::shared_ptr<ExpressionType> type)
    : Literal(t, std::move(type))
{
}

BooleanLiteral::BooleanLiteral(Token const& t, std::shared_ptr<ObjectType> const& type)
    : Literal(t, type)
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

CharLiteral::CharLiteral(Token const& t)
    : Literal(t, ObjectType::get("char"))
{
}

CharLiteral::CharLiteral(Token const& t, std::shared_ptr<ExpressionType> type)
    : Literal(t, std::move(type))
{
}

CharLiteral::CharLiteral(Token const& t, std::shared_ptr<ObjectType> const& type)
    : Literal(t, type)
{
}

// -- FloatLiteral ----------------------------------------------------------

FloatLiteral::FloatLiteral(Token const& t)
    : Literal(t, ObjectType::get("s64"))
{
}

FloatLiteral::FloatLiteral(Token const& t, std::shared_ptr<ExpressionType> type)
    : Literal(t, std::move(type))
{
}

FloatLiteral::FloatLiteral(Token const& t, std::shared_ptr<ObjectType> const& type)
    : Literal(t, type)
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

IntLiteral::IntLiteral(Token const& t)
    : Literal(t, ObjectType::get("s64"))
{
}

IntLiteral::IntLiteral(Token const& t, std::shared_ptr<ExpressionType> type)
    : Literal(t, std::move(type))
{
}

IntLiteral::IntLiteral(Token const& t, std::shared_ptr<ObjectType> const& type)
    : Literal(t, type)
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

Literal::Literal(Token const& t, std::shared_ptr<ExpressionType> type)
    : Expression(t, std::move(type))
{
}

Literal::Literal(Token const& t, std::shared_ptr<ObjectType> const& type)
    : Expression(t, std::make_shared<ExpressionType>(t, type))
{
}

std::string Literal::attributes() const
{
    return format(R"(value="{}" type="{}")", Expression::token().value(), type_name());
}

std::string Literal::to_string() const
{
    return format("{}: {}", Expression::token().value(), type()->to_string());
}

std::shared_ptr<Expression> Literal::apply(Token const& token)
{
    return std::make_shared<UnaryExpression>(token, std::dynamic_pointer_cast<Expression>(shared_from_this()));
}

// -- StringLiteral ---------------------------------------------------------

StringLiteral::StringLiteral(Token const& t)
    : Literal(t, ObjectType::get("string"))
{
}

StringLiteral::StringLiteral(Token const& t, std::shared_ptr<ExpressionType> type)
    : Literal(t, std::move(type))
{
}

StringLiteral::StringLiteral(Token const& t, std::shared_ptr<ObjectType> const& type)
    : Literal(t, type)
{
}

}
