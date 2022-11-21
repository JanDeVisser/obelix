/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/Expression.h>

namespace Obelix {

extern_logging_category(parser);

// -- Expression ------------------------------------------------------------

Expression::Expression(Token token, std::shared_ptr<ExpressionType> type)
    : SyntaxNode(std::move(token))
    , m_type(std::move(type))
{
}

std::shared_ptr<ExpressionType> const& Expression::type() const
{
    return m_type;
}

std::string Expression::type_name() const
{
    return (m_type) ? type()->type_name() : "[Unresolved]";
}

bool Expression::is_typed()
{
    return m_type != nullptr;
}

std::string Expression::attributes() const
{
    return format(R"(type="{}")", type_name());
}

// -- ExpressionList --------------------------------------------------------

ExpressionList::ExpressionList(Token token, Expressions expressions)
    : Expression(std::move(token))
    , m_expressions(std::move(expressions))
{
}

Expressions const& ExpressionList::expressions() const
{
    return m_expressions;
}

Nodes ExpressionList::children() const
{
    Nodes ret;
    for (auto const& expr : expressions()) {
        ret.push_back(expr);
    }
    return ret;
}

std::string ExpressionList::to_string() const
{
    Strings ret;
    for (auto const& expr : expressions()) {
        ret.push_back(expr->to_string());
    }
    return join(ret, ", ");
}

// -- Identifier ------------------------------------------------------------

Identifier::Identifier(Token token, std::string name, std::shared_ptr<ExpressionType> type)
    : Expression(std::move(token), std::move(type))
    , m_identifier(std::move(name))
{
}

std::string const& Identifier::name() const
{
    return m_identifier;
}

std::string Identifier::attributes() const
{
    return format(R"(name="{}" type="{}")", name(), type());
}

std::string Identifier::to_string() const
{
    return format("{}: {}", name(), type());
}

// -- Variable --------------------------------------------------------------

Variable::Variable(Token token, std::string name, std::shared_ptr<ExpressionType> type)
    : Identifier(std::move(token), std::move(name), std::move(type))
{
}

// -- This ------------------------------------------------------------------

This::This(Token token)
    : Expression(std::move(token))
{
}

std::string This::to_string() const
{
    return "this";
}

// -- BinaryExpression ------------------------------------------------------

BinaryExpression::BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs, std::shared_ptr<ExpressionType> type)
    : Expression(op, std::move(type))
    , m_lhs(std::move(lhs))
    , m_operator(std::move(op))
    , m_rhs(std::move(rhs))
{
}

std::string BinaryExpression::attributes() const
{
    return format(R"(operator="{}" type="{}")", m_operator.value(), type_name());
}

Nodes BinaryExpression::children() const
{
    return { m_lhs, m_rhs };
}

std::string BinaryExpression::to_string() const
{
    return format("{} {} {}", lhs()->to_string(), op().value(), rhs()->to_string());
}

std::shared_ptr<Expression> const& BinaryExpression::lhs() const
{
    return m_lhs;
}

std::shared_ptr<Expression> const& BinaryExpression::rhs() const
{
    return m_rhs;
}

Token BinaryExpression::op() const
{
    return m_operator;
}

// -- UnaryExpression -------------------------------------------------------

UnaryExpression::UnaryExpression(Token op, std::shared_ptr<Expression> operand, std::shared_ptr<ExpressionType> type)
    : Expression(op, std::move(type))
    , m_operator(std::move(op))
    , m_operand(std::move(operand))
{
}

std::string UnaryExpression::attributes() const
{
    return format(R"(operator="{}" type="{}")", m_operator.value(), type());
}

Nodes UnaryExpression::children() const
{
    return { m_operand };
}

std::string UnaryExpression::to_string() const
{
    return format("{} {}", op().value(), operand()->to_string());
}

Token UnaryExpression::op() const
{
    return m_operator;
}

std::shared_ptr<Expression> const& UnaryExpression::operand() const
{
    return m_operand;
}

// -- CastExpression --------------------------------------------------------

CastExpression::CastExpression(Token token, std::shared_ptr<Expression> expression, std::shared_ptr<ExpressionType> cast_to)
    : Expression(std::move(token), std::move(cast_to))
    , m_expression(std::move(expression))
{
}

std::string CastExpression::attributes() const
{
    return format(R"(type="{}")", type());
}

Nodes CastExpression::children() const
{
    return { m_expression };
}

std::string CastExpression::to_string() const
{
    return format("{} as {}", m_expression, type());
}

std::shared_ptr<Expression> const& CastExpression::expression() const
{
    return m_expression;
}

}
