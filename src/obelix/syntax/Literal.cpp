/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/Literal.h>

namespace Obelix {

extern_logging_category(parser);

// -- BooleanLiteral --------------------------------------------------------

BooleanLiteral::BooleanLiteral(Token token, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(token), std::move(type))
{
}

// -- CharLiteral -----------------------------------------------------------

CharLiteral::CharLiteral(Token token, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(token), std::move(type))
{
}

// -- FloatLiteral ----------------------------------------------------------

FloatLiteral::FloatLiteral(Token token, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(token), std::move(type))
{
}
// -- IntLiteral ------------------------------------------------------------

IntLiteral::IntLiteral(Token token, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(token), std::move(type))
{
}

// -- Literal ---------------------------------------------------------------

Literal::Literal(Token token, std::shared_ptr<ExpressionType> type)
    : Expression(token.location(), std::move(type))
    , m_token(std::move(token))
{
}

Token const& Literal::token() const
{
    return m_token;
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

// -- StringLiteral ---------------------------------------------------------

StringLiteral::StringLiteral(Token token, std::shared_ptr<ExpressionType> type)
    : Literal(std::move(token), std::move(type))
{
}

}
