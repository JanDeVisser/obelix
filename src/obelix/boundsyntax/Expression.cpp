/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/boundsyntax/Expression.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundExpression -------------------------------------------------------

BoundExpression::BoundExpression(Token token, std::shared_ptr<ObjectType> type)
    : SyntaxNode(std::move(token))
    , m_type(std::move(type))
{
}

BoundExpression::BoundExpression(std::shared_ptr<Expression> const& expr, std::shared_ptr<ObjectType> type)
    : SyntaxNode(expr->token())
    , m_type(std::move(type))
{
}

BoundExpression::BoundExpression(std::shared_ptr<BoundExpression> const& expr)
    : SyntaxNode(expr->token())
    , m_type(expr->type())
{
}

BoundExpression::BoundExpression(Token token, PrimitiveType type)
    : SyntaxNode(std::move(token))
    , m_type(ObjectType::get(type))
{
}

std::shared_ptr<ObjectType> const& BoundExpression::type() const
{
    return m_type;
}

std::string const& BoundExpression::type_name() const
{
    return m_type->name();
}

std::string BoundExpression::attributes() const
{
    return format(R"(type="{}")", type()->name());
}

// -- BoundExpressionList ---------------------------------------------------

BoundExpressionList::BoundExpressionList(Token token, BoundExpressions expressions)
    : BoundExpression(std::move(token), PrimitiveType::List)
    , m_expressions(std::move(expressions))
{
}

BoundExpressions const& BoundExpressionList::expressions() const
{
    return m_expressions;
}

ObjectTypes BoundExpressionList::expression_types() const
{
    ObjectTypes ret;
    for (auto const& expr : expressions()) {
        ret.push_back(expr->type());
    }
    return ret;
}

Nodes BoundExpressionList::children() const
{
    Nodes ret;
    for (auto const& expr : expressions()) {
        ret.push_back(expr);
    }
    return ret;
}

std::string BoundExpressionList::to_string() const
{
    Strings ret;
    for (auto const& expr : expressions()) {
        ret.push_back(expr->to_string());
    }
    return join(ret, ", ");
}

// -- BoundBinaryExpression -------------------------------------------------

BoundBinaryExpression::BoundBinaryExpression(std::shared_ptr<BinaryExpression> const& expr, std::shared_ptr<BoundExpression> lhs, BinaryOperator op, std::shared_ptr<BoundExpression> rhs, std::shared_ptr<ObjectType> type)
    : BoundExpression(expr, std::move(type))
    , m_lhs(std::move(lhs))
    , m_operator(op)
    , m_rhs(std::move(rhs))
{
}

BoundBinaryExpression::BoundBinaryExpression(Token token, std::shared_ptr<BoundExpression> lhs, BinaryOperator op, std::shared_ptr<BoundExpression> rhs, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(token), std::move(type))
    , m_lhs(std::move(lhs))
    , m_operator(op)
    , m_rhs(std::move(rhs))
{
}

std::string BoundBinaryExpression::attributes() const
{
    return format(R"(operator="{}" type="{}")", m_operator, type());
}

Nodes BoundBinaryExpression::children() const
{
    return { m_lhs, m_rhs };
}

std::string BoundBinaryExpression::to_string() const
{
    return format("({} {} {}): {}", lhs(), op(), rhs(), type());
}

std::shared_ptr<BoundExpression> const& BoundBinaryExpression::lhs() const
{
    return m_lhs;
}

std::shared_ptr<BoundExpression> const& BoundBinaryExpression::rhs() const
{
    return m_rhs;
}

BinaryOperator BoundBinaryExpression::op() const
{
    return m_operator;
}

// -- BoundUnaryExpression --------------------------------------------------

BoundUnaryExpression::BoundUnaryExpression(std::shared_ptr<UnaryExpression> const& expr, std::shared_ptr<BoundExpression> operand, UnaryOperator op, std::shared_ptr<ObjectType> type)
    : BoundExpression(expr, std::move(type))
    , m_operator(op)
    , m_operand(std::move(operand))
{
}

BoundUnaryExpression::BoundUnaryExpression(Token token, std::shared_ptr<BoundExpression> operand, UnaryOperator op, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(token), std::move(type))
    , m_operator(op)
    , m_operand(std::move(operand))
{
}

std::string BoundUnaryExpression::attributes() const
{
    return format(R"(operator="{}" type="{}")", m_operator, type());
}

Nodes BoundUnaryExpression::children() const
{
    return { m_operand };
}

std::string BoundUnaryExpression::to_string() const
{
    return format("{} ({}): {}", op(), operand()->to_string(), type());
}

UnaryOperator BoundUnaryExpression::op() const
{
    return m_operator;
}

std::shared_ptr<BoundExpression> const& BoundUnaryExpression::operand() const
{
    return m_operand;
}

// -- BoundCastExpression ---------------------------------------------------

BoundCastExpression::BoundCastExpression(Token token, std::shared_ptr<BoundExpression> expression, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(token), std::move(type))
    , m_expression(std::move(expression))
{
}

std::string BoundCastExpression::attributes() const
{
    return format(R"(type="{}")", type());
}

Nodes BoundCastExpression::children() const
{
    return { m_expression };
}

std::string BoundCastExpression::to_string() const
{
    return format("{} as {}", expression(), type());
}

std::shared_ptr<BoundExpression> const& BoundCastExpression::expression() const
{
    return m_expression;
}

// -- BoundExpressionStatement ----------------------------------------------

BoundExpressionStatement::BoundExpressionStatement(std::shared_ptr<ExpressionStatement> const& stmt, std::shared_ptr<BoundExpression> expression)
    : Statement(stmt->token())
    , m_expression(std::move(expression))
{
}

BoundExpressionStatement::BoundExpressionStatement(Token token, std::shared_ptr<BoundExpression> expression)
    : Statement(std::move(token))
    , m_expression(std::move(expression))
{
}

std::shared_ptr<BoundExpression> const& BoundExpressionStatement::expression() const
{
    return m_expression;
}

Nodes BoundExpressionStatement::children() const
{
    return { m_expression };
}

std::string BoundExpressionStatement::to_string() const
{
    return m_expression->to_string();
}

// -- BoundConditionalValue -------------------------------------------------

BoundConditionalValue::BoundConditionalValue(Token token, std::shared_ptr<BoundExpression> expression, bool success, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(token), std::move(type))
    , m_expression(expression)
    , m_success(success)
{
}

std::shared_ptr<BoundExpression> const& BoundConditionalValue::expression() const
{
    return m_expression;
}

bool BoundConditionalValue::success() const
{
    return m_success;
}

std::string BoundConditionalValue::attributes() const
{
    return format(R"(success="{}" type="{}")", success(), type());
}

Nodes BoundConditionalValue::children() const
{
    return Nodes { m_expression };
}

std::string BoundConditionalValue::BoundConditionalValue::to_string() const
{
    return format(R"({}: {})", ((m_success) ? "value" : "error"), expression());
}

}
