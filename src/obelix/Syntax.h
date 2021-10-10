//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <lexer/Token.h>
#include <string>

namespace Obelix {

class SyntaxNode {
public:
    ~SyntaxNode() = default;
    virtual void dump() = 0;
};

class Statement : public SyntaxNode {
public:
    virtual void execute()
    {
        fatal("Not implemented");
    }
};

class Pass : public Statement {
    void dump() override
    {
        printf(";\n");
    }

    void execute() override
    {
    }
};

class Block : public Statement {
public:
    Block()
        : Statement()
    {
    }
    ~Block() = default;

    void append(std::shared_ptr<Statement> const& statement)
    {
        m_statements.push_back(statement);
    }

    void dump() override
    {
        printf("{\n");
        for (auto& statement : m_statements) {
            statement->dump();
        }
        printf("}\n");
    }

    void execute() override
    {
        for (auto& statement : m_statements) {
            statement->execute();
        }
    }

private:
    std::vector<std::shared_ptr<Statement>> m_statements {};
};

class Module : public Block {
public:
    explicit Module(std::string name)
        : Block()
        , m_name(move(name))
    {
    }

    void dump() override
    {
        printf("module %s\n\n", m_name.c_str());
        Block::dump();
    }

private:
    std::string m_name;
};

class FunctionDef : public Block {
public:
    FunctionDef(std::string name, std::vector<std::string> arguments)
        : Block()
        , m_name(move(name))
        , m_arguments(move(arguments))
    {
    }

    void dump() override
    {
        printf("func %s(", m_name.c_str());
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                printf(", ");
            first = false;
            printf("%s", arg.c_str());
        }
        printf(")\n");
    }

private:
    std::string m_name;
    std::vector<std::string> m_arguments;
};

class NativeFunctionDef : public SyntaxNode {
public:
    NativeFunctionDef(std::string name, std::vector<std::string> arguments, std::function<Obj(Ptr<Arguments>)> function)
        : SyntaxNode()
        , m_name(move(name))
        , m_arguments(move(arguments))
        , m_function(move(function))
    {
    }

    void dump() override
    {
        printf("native func %s(", m_name.c_str());
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                printf(", ");
            first = false;
            printf("%s", arg.c_str());
        }
        printf(")\n");
    }

private:
    std::string m_name;
    std::vector<std::string> m_arguments;
    std::function<Obj(Ptr<Arguments>)> m_function;
};

class ErrorNode : public SyntaxNode {
public:
    explicit ErrorNode(ErrorCode code)
        : SyntaxNode()
        , m_code(code)
    {
    }

    void dump() override
    {
        printf("ERROR %d\n", (int)m_code);
    }

private:
    ErrorCode m_code { ErrorCode::NoError };
};

class Expression : public SyntaxNode {
public:
    virtual Obj evaluate()
    {
        fatal("Not yet implemented");
    }
};

class Literal : public Expression {
public:
    explicit Literal(Token token)
        : Expression()
        , m_literal(std::move(token))
    {
    }

    void dump() override
    {
        printf("%s", m_literal.value().c_str());
    }

    Obj evaluate() override
    {
        return m_literal.to_object();
    }

private:
    Token m_literal;
};

class VariableReference : public Expression {
public:
    explicit VariableReference(std::string name)
        : Expression()
        , m_name(std::move(name))
    {
    }

    void dump() override
    {
        printf("%s", m_name.c_str());
    }

private:
    std::string m_name;
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

    void dump() override
    {
        printf("(");
        m_lhs->dump();
        printf(") %s (", m_operator.value().c_str());
        m_rhs->dump();
        printf(")");
    }

    Obj evaluate() override
    {
        Obj lhs = m_lhs->evaluate();
        Obj rhs = m_rhs->evaluate();
        oassert(lhs->type() == "integer" && rhs.type() == "integer", "Binary expression only works on integers");
        switch (m_operator.code()) {
        case TokenCode::Plus:
            return make_obj<Integer>(lhs.to_long().value() + rhs.to_long().value());
        case TokenCode::Minus:
            return make_obj<Integer>(lhs.to_long().value() - rhs.to_long().value());
        case TokenCode::Asterisk:
            return make_obj<Integer>(lhs.to_long().value() * rhs.to_long().value());
        case TokenCode::Slash:
            return make_obj<Integer>(lhs.to_long().value() / rhs.to_long().value());
        default:
            fatal("Unreached");
        }
    }

private:
    std::shared_ptr<Expression> m_lhs;
    Token m_operator;
    std::shared_ptr<Expression> m_rhs;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(Token op, std::shared_ptr<Expression> operand)
        : Expression()
        , m_operator(std::move(op))
        , m_operand(std::move(operand))
    {
    }

    void dump() override
    {
        printf(" %s (", m_operator.to_string().c_str());
        m_operand->dump();
        printf(")");
    }

    Obj evaluate() override
    {
        Obj operand = m_operand->evaluate();
        oassert(operand->type() == "integer", "Unary expression only works on integers");
        switch (m_operator.code()) {
        case TokenCode::Plus:
            return operand;
        case TokenCode::Minus:
            return make_obj<Integer>(-operand.to_long().value());
        default:
            fatal("Unreached");
        }
    }

private:
    Token m_operator;
    std::shared_ptr<Expression> m_operand;
};

class FunctionCall : public Expression {
public:
    FunctionCall(std::string name, std::vector<std::shared_ptr<Expression>> arguments)
        : Expression()
        , m_name(move(name))
        , m_arguments(move(arguments))
    {
    }

    void dump() override
    {
        printf("%s(", m_name.c_str());
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                printf(", ");
            first = false;
            arg->dump();
        }
        printf(")");
    }

    Obj evaluate() override
    {
        if (m_name == "print") {
            bool first = true;
            for (auto& arg : m_arguments) {
                if (!first)
                    printf(", ");
                first = false;
                auto val = arg->evaluate();
                printf("%s", val.to_string().c_str());
            }
            printf("\n");
        }
        return Obj::null();
    }

private:
    std::string m_name;
    std::vector<std::shared_ptr<Expression>> m_arguments;
};

class Assignment : public Statement {
public:
    Assignment(std::string variable, std::shared_ptr<Expression> expression)
        : Statement()
        , m_variable(move(variable))
        , m_expression(move(expression))
    {
    }

    void dump() override
    {
        printf("var %s = ", m_variable.c_str());
        m_expression->dump();
        printf("\n");
    }

private:
    std::string m_variable;
    std::shared_ptr<Expression> m_expression;
};

class ProcedureCall : public Statement {
public:
    explicit ProcedureCall(std::shared_ptr<FunctionCall> call_expression)
        : Statement()
        , m_call_expression(move(call_expression))
    {
    }

    void dump() override
    {
        m_call_expression->dump();
        printf("\n");
    }

    void execute() override
    {
        m_call_expression->evaluate();
    }

private:
    std::shared_ptr<FunctionCall> m_call_expression;
};

class IfStatement : public Statement {
public:
    IfStatement(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> if_stmt, std::shared_ptr<Statement> else_stmt)
        : Statement()
        , m_condition(move(condition))
        , m_if(move(if_stmt))
        , m_else(move(else_stmt))
    {
    }

    void execute() override
    {
        Obj condition = m_condition->evaluate();
        if (condition->to_bool().value()) {
            m_if->execute();
        } else {
            m_else->execute();
        }
    }

    void dump() override
    {
        printf("if ");
        m_condition->dump();
        printf("\n");
        m_if->dump();
        if (m_else) {
            printf("else\n");
            m_else->dump();
        }
    }

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_if;
    std::shared_ptr<Statement> m_else;
};

}