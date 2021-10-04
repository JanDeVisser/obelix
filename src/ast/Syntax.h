//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include "lexer/Token.h"
#include <string>

class SyntaxNode {
public:
    [[nodiscard]] virtual std::shared_ptr<SyntaxNode> evaluate(std::shared_ptr<Object> scope, bool must_resolve) const = 0;
    [[nodiscard]] virtual bool is_exception() const { return false; }
    [[nodiscard]] virtual bool is_literal() const { return false; }
};

class Literal;

class Block : public SyntaxNode {
public:
    Block() : SyntaxNode() { }
    void append(std::shared_ptr<SyntaxNode> const& child) {
        m_children.push_back(child);
    }

    [[nodiscard]] std::shared_ptr<SyntaxNode> evaluate(std::shared_ptr<Object> scope, bool must_resolve) const override;

private:
    std::vector<std::shared_ptr<SyntaxNode>> m_children { };
};

class ErrorNode : public SyntaxNode {
public:
    ErrorNode(ErrorCode code)
        : SyntaxNode()
        , m_code(code)
    {
    }

    [[nodiscard]] bool is_exception() const override { return true; }
    [[nodiscard]] std::shared_ptr<SyntaxNode> evaluate(std::shared_ptr<Object> scope, bool must_resolve) const override
    {
        return std::make_shared<ErrorNode>(m_code);
    }

private:
    ErrorCode m_code { ErrorCode::NoError }
};

class Expression : public SyntaxNode {
};

class Literal : public Expression {
public:
    explicit Literal(Token token)
        : Expression()
        , m_literal(std::move(token))
        , m_object(m_literal.to_object())
    {
    }

    explicit Literal(std::shared_ptr<Object> object)
        : Expression()
        , m_literal(TokenType::Unknown, object->to_string())
        , m_object(std::move(object))
    {
    }

    [[nodiscard]] std::shared_ptr<SyntaxNode> evaluate(std::shared_ptr<Object> scope, bool must_resolve) const override
    {
        return std::make_shared<Literal>(m_object);
    }

    [[nodiscard]] bool is_literal() const override { return true; }
    [[nodiscard]] std::shared_ptr<Object> to_object() const { return m_object; }
private:
    Token m_literal;
    std::shared_ptr<Object> m_object;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs)
        : Expression()
        , m_lhs(std::move(lhs))
        , m_operator(std::move(op))
        , m_rhs(std::move(rhs))
    {
    }

    [[nodiscard]] std::shared_ptr<SyntaxNode> evaluate(std::shared_ptr<Object> scope, bool must_resolve) const override
    {
        auto lhs_result = std::dynamic_pointer_cast<Expression>(m_lhs->evaluate(scope, must_resolve));
        auto rhs_result = std::dynamic_pointer_cast<Expression>(m_rhs->evaluate(scope, must_resolve));
        if (lhs_result->is_exception()) {
            return lhs_result;
        }
        if (rhs_result->is_exception()) {
            return rhs_result;
        }
        if (lhs_result->is_literal() && rhs_result->is_literal()) {
            auto lhs_object = std::dynamic_pointer_cast<Literal>(lhs_result)->to_object();
            std::vector<std::shared_ptr<Object>> args;
            args.push_back(std::dynamic_pointer_cast<Literal>(rhs_result)->to_object());
            return std::make_shared<Literal>(lhs_object->evaluate(m_operator.to_string(), args));
        } else {
            return std::make_shared<BinaryExpression>(lhs_result, m_operator, rhs_result);
        }
    }

private:
    std::shared_ptr<Expression> m_lhs;
    Token m_operator;
    std::shared_ptr<Expression> m_rhs;
};

#if 0
class UnaryExpression : public Expression {
    UnaryExpression(Token op, std::shared_ptr<Expression> operand)
        : Expression()
        , m_operator(std::move(op))
        , m_operand(std::move(operand))
    {
    }

    [[nodiscard]] std::shared_ptr<SyntaxNode> evaluate(std::shared_ptr<Object> scope, bool must_resolve) const override
    {
        auto operand = std::dynamic_pointer_cast<Expression>(m_operand->evaluate(scope, must_resolve));
        if (operand->is_exception()) {
            return operand;
        }
        if (operand->is_literal()) {
            auto object = std::dynamic_pointer_cast<Literal>(operand)->to_object();
            std::vector<std::shared_ptr<Object>> args;
            return std::make_shared<Literal>(object->evaluate(m_operator.to_string(), args));
        } else if (!must_resolve) {
            auto n = new UnaryExpression(m_operator, operand);
            return std::make_shared<UnaryExpression>(m_operator, operand);
        } else {
            return std::make_shared<ErrorNode>(ErrorCode::CouldNotResolveNode);
        }
    }

private:
    Token m_operator;
    std::shared_ptr<Expression> m_operand;
};
#endif

class Identifier : public Expression {
public:
private:
    Token m_identifier;
};

class Assignment : public SyntaxNode {
public:
private:
    std::string m_variable;
    std::shared_ptr<Expression> m_expression;
};
