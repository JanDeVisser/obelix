//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <lexer/Token.h>
#include <obelix/Scope.h>
#include <string>

namespace Obelix {

class SyntaxNode {
public:
    ~SyntaxNode() = default;
    virtual void dump(int) = 0;
    static void indent_line(int num)
    {
        if (num > 0)
            printf("%*s", num, "");
    }
};

class Statement : public SyntaxNode {
public:
    virtual void execute(Scope&)
    {
        fatal("Not implemented");
    }
};

class Pass : public Statement {
    void dump(int indent) override
    {
        indent_line(indent);
        printf(";\n");
    }

    void execute(Scope&) override
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

    void dump(int indent) override
    {
        indent_line(indent);
        printf("{\n");
        for (auto& statement : m_statements) {
            statement->dump(indent + 2);
        }
        indent_line(indent);
        printf("}\n");
    }

    void execute(Scope& scope) override
    {
        for (auto& statement : m_statements) {
            statement->execute(scope);
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

    void dump(int indent) override
    {
        indent_line(indent);
        printf("module %s\n\n", m_name.c_str());
        Block::dump(indent);
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

    void dump(int indent) override
    {
        indent_line(indent);
        printf("func %s(", m_name.c_str());
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                printf(", ");
            first = false;
            printf("%s", arg.c_str());
        }
        printf(")\n");
        Block::dump(indent);
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

    void dump(int indent) override
    {
        indent_line(indent);
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

    void dump(int indent) override
    {
        indent_line(indent);
        printf("ERROR %d\n", (int)m_code);
    }

private:
    ErrorCode m_code { ErrorCode::NoError };
};

class Expression : public SyntaxNode {
public:
    virtual Obj evaluate(Scope const&)
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

    void dump(int indent) override
    {
        printf("%s", m_literal.value().c_str());
    }

    Obj evaluate(Scope const&) override
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

    void dump(int indent) override
    {
        printf("%s", m_name.c_str());
    }

    [[nodiscard]] Obj evaluate(Scope const& scope) override
    {
        auto value_maybe = scope.get(m_name);
        if (!value_maybe.has_value()) {
            fprintf(stderr, "Runtime Error: unknown variable '%s'", m_name.c_str());
            exit(1);
        }
        return value_maybe.value();
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

    void dump(int indent) override
    {
        printf("(");
        m_lhs->dump(indent);
        printf(") %s (", m_operator.value().c_str());
        m_rhs->dump(indent);
        printf(")");
    }

    Obj evaluate(Scope const& scope) override
    {
        Obj lhs = m_lhs->evaluate(scope);
        Obj rhs = m_rhs->evaluate(scope);
        auto ret_maybe = lhs.evaluate(m_operator.value(), make_typed<Arguments>(rhs));
        if (!ret_maybe.has_value())
            return make_obj<Exception>(ErrorCode::RegexpSyntaxError, "Could not resolve operator");
        return ret_maybe.value();
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

    void dump(int indent) override
    {
        printf(" %s (", m_operator.to_string().c_str());
        m_operand->dump(indent);
        printf(")");
    }

    Obj evaluate(Scope const& scope) override
    {
        Obj operand = m_operand->evaluate(scope);
        auto ret_maybe = operand.evaluate(m_operator.value(), make_typed<Arguments>());
        if (!ret_maybe.has_value())
            return make_obj<Exception>(ErrorCode::RegexpSyntaxError, "Could not resolve operator");
        return ret_maybe.value();
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

    void dump(int indent) override
    {
        indent_line(indent);
        printf("%s(", m_name.c_str());
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                printf(", ");
            first = false;
            arg->dump(indent);
        }
        printf(")");
    }

    Obj evaluate(Scope const& scope) override
    {
        if (m_name == "print") {
            bool first = true;
            for (auto& arg : m_arguments) {
                if (!first)
                    printf(", ");
                first = false;
                auto val = arg->evaluate(scope);
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

    void dump(int indent) override
    {
        printf("var %s = ", m_variable.c_str());
        m_expression->dump(indent);
        printf("\n");
    }

    void execute(Scope& scope) override
    {
        scope.set(m_variable, m_expression->evaluate(scope));
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

    void dump(int indent) override
    {
        m_call_expression->dump(indent);
        printf("\n");
    }

    void execute(Scope& scope) override
    {
        m_call_expression->evaluate(scope);
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

    void execute(Scope& scope) override
    {
        Obj condition = m_condition->evaluate(scope);
        if (condition->to_bool().value()) {
            m_if->execute(scope);
        } else {
            m_else->execute(scope);
        }
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("if ");
        m_condition->dump(indent);
        printf("\n");
        m_if->dump(indent + 2);
        if (m_else) {
            indent_line(indent);
            printf("else\n");
            m_else->dump(indent + 2);
        }
    }

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_if;
    std::shared_ptr<Statement> m_else;
};


class WhileStatement : public Statement {
public:
    WhileStatement(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> stmt)
        : Statement()
        , m_condition(move(condition))
        , m_stmt(move(stmt))
    {
    }

    void execute(Scope& scope) override
    {
        for (Obj condition = m_condition->evaluate(scope); condition->to_bool().value(); condition = m_condition->evaluate(scope)) {
            m_stmt->execute(scope);
        }
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("while ");
        m_condition->dump(indent);
        printf("\n");
        m_stmt->dump(indent + 2);
    }

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

}