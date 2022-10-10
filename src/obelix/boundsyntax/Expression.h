/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/syntax/Expression.h>
#include <obelix/syntax/Statement.h>

namespace Obelix {

NODE_CLASS(BoundExpression, SyntaxNode)
public:
    BoundExpression() = default;
    BoundExpression(Token, std::shared_ptr<ObjectType>);
    BoundExpression(std::shared_ptr<Expression> const&, std::shared_ptr<ObjectType>);
    BoundExpression(std::shared_ptr<BoundExpression> const&);
    BoundExpression(Token, PrimitiveType);
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const;
    [[nodiscard]] std::string const& type_name() const;
    [[nodiscard]] std::string attributes() const override;

private:
    std::shared_ptr<ObjectType> m_type { nullptr };
};

using BoundExpressions = std::vector<std::shared_ptr<BoundExpression>>;

NODE_CLASS(BoundExpressionList, BoundExpression)
public:
    BoundExpressionList(Token, BoundExpressions);
    [[nodiscard]] BoundExpressions const& expressions() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    BoundExpressions m_expressions;
};

NODE_CLASS(BoundBinaryExpression, BoundExpression)
public:
    BoundBinaryExpression(std::shared_ptr<BinaryExpression> const&, std::shared_ptr<BoundExpression>, BinaryOperator, std::shared_ptr<BoundExpression>, std::shared_ptr<ObjectType> = nullptr);
    BoundBinaryExpression(Token, std::shared_ptr<BoundExpression>, BinaryOperator, std::shared_ptr<BoundExpression>, std::shared_ptr<ObjectType>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& lhs() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& rhs() const;
    [[nodiscard]] BinaryOperator op() const;

private:
    std::shared_ptr<BoundExpression> m_lhs;
    BinaryOperator m_operator;
    std::shared_ptr<BoundExpression> m_rhs;
};

NODE_CLASS(BoundUnaryExpression, BoundExpression)
public:
    BoundUnaryExpression(std::shared_ptr<UnaryExpression> const&, std::shared_ptr<BoundExpression>, UnaryOperator, std::shared_ptr<ObjectType> = nullptr);
    BoundUnaryExpression(Token, std::shared_ptr<BoundExpression>, UnaryOperator, std::shared_ptr<ObjectType> = nullptr);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] UnaryOperator op() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& operand() const;

private:
    UnaryOperator m_operator;
    std::shared_ptr<BoundExpression> m_operand;
};

NODE_CLASS(BoundCastExpression, BoundExpression)
public:
    BoundCastExpression(Token token, std::shared_ptr<BoundExpression>, std::shared_ptr<ObjectType>);

    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;

private:
    std::shared_ptr<BoundExpression> m_expression;
};

NODE_CLASS(BoundExpressionStatement, Statement)
public:
    BoundExpressionStatement(std::shared_ptr<ExpressionStatement> const&, std::shared_ptr<BoundExpression>);
    BoundExpressionStatement(Token, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<BoundExpression> m_expression;
};

}