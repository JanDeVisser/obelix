/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <obelix/syntax/Syntax.h>
#include <obelix/syntax/Type.h>

namespace Obelix {

ABSTRACT_NODE_CLASS(Expression, SyntaxNode)
public:
    explicit Expression(Span = {}, std::shared_ptr<ExpressionType> = nullptr);
    [[nodiscard]] std::shared_ptr<ExpressionType> const& type() const;
    [[nodiscard]] std::string type_name() const;
    [[nodiscard]] bool is_typed();
    [[nodiscard]] std::string attributes() const override;

private:
    std::shared_ptr<ExpressionType> m_type { nullptr };
};

using Expressions = std::vector<std::shared_ptr<Expression>>;

NODE_CLASS(ExpressionList, Expression)
public:
    ExpressionList(Span, Expressions);
    [[nodiscard]] Expressions const& expressions() const;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
private:
    Expressions m_expressions;
};

NODE_CLASS(Identifier, Expression)
public:
    explicit Identifier(Span, std::string, std::shared_ptr<ExpressionType> = nullptr);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_identifier;
};

using Identifiers = std::vector<std::shared_ptr<Identifier>>;

NODE_CLASS(Variable, Identifier)
public:
    Variable(Span, std::string, std::shared_ptr<ExpressionType> type = nullptr);
};

NODE_CLASS(This, Expression)
public:
    explicit This(Span);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(BinaryExpression, Expression)
public:
    BinaryExpression(std::shared_ptr<Expression>, Token, std::shared_ptr<Expression>, std::shared_ptr<ExpressionType> = nullptr);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

    [[nodiscard]] std::shared_ptr<Expression> const& lhs() const;
    [[nodiscard]] std::shared_ptr<Expression> const& rhs() const;
    [[nodiscard]] Token op() const;

private:
    std::shared_ptr<Expression> m_lhs;
    Token m_operator;
    std::shared_ptr<Expression> m_rhs;
};

NODE_CLASS(UnaryExpression, Expression)
public:
    UnaryExpression(Token, std::shared_ptr<Expression>, std::shared_ptr<ExpressionType> = nullptr);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] Token op() const;
    [[nodiscard]] std::shared_ptr<Expression> const& operand() const;

private:
    Token m_operator;
    std::shared_ptr<Expression> m_operand;
};

NODE_CLASS(CastExpression, Expression)
public:
    CastExpression(Span location, std::shared_ptr<Expression> expression, std::shared_ptr<ExpressionType> cast_to);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const;

private:
    std::shared_ptr<Expression> m_expression;
};

}
