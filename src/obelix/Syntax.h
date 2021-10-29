//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <core/NativeFunction.h>
#include <lexer/Token.h>
#include <obelix/BoundFunction.h>
#include <obelix/Runtime.h>
#include <obelix/Scope.h>
#include <string>

namespace Obelix {

class Module;

class SyntaxNode {
public:
    explicit SyntaxNode(SyntaxNode* parent)
        : m_parent(parent)
    {
    }

    virtual ~SyntaxNode() = default;
    virtual void dump(int) = 0;
    static void indent_line(int num)
    {
        if (num > 0)
            printf("%*s", num, "");
    }

    [[nodiscard]] SyntaxNode* parent() const { return m_parent; }
    [[nodiscard]] virtual Runtime& runtime() const;
    [[nodiscard]] virtual Module const* module() const;

private:
    SyntaxNode* m_parent;
};

class Statement : public SyntaxNode {
public:
    explicit Statement(SyntaxNode* parent)
        : SyntaxNode(parent)
    {
    }

    virtual ExecutionResult execute(Ptr<Scope>)
    {
        fatal("Not implemented");
    }
};

class Import : public Statement {
public:
    explicit Import(SyntaxNode* parent, std::string name)
        : Statement(parent)
        , m_name(move(name))
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("import %s\n", m_name.c_str());
    }

    ExecutionResult execute(Ptr<Scope>) override;

private:
    std::string m_name;
};

class Pass : public Statement {
public:
    explicit Pass(SyntaxNode* parent)
        : Statement(parent)
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf(";\n");
    }

    ExecutionResult execute(Ptr<Scope>) override
    {
        return {};
    }
};

class Block : public Statement {
public:
    explicit Block(SyntaxNode* parent)
        : Statement(parent)
        , m_scope(make_null<Scope>())
    {
    }

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

    [[nodiscard]] ExecutionResult execute_block(Ptr<Scope> const& block_scope) const
    {
        ExecutionResult result;
        for (auto& statement : m_statements) {
            result = statement->execute(block_scope);
            if (result.code != ExecutionResultCode::None)
                return result;
        }
        return result;
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        m_scope = make_typed<Scope>(scope);
        return execute_block(m_scope);
    }

    [[nodiscard]] Ptr<Scope>& scope() { return m_scope; }

private:
    std::vector<std::shared_ptr<Statement>> m_statements {};
    Ptr<Scope> m_scope;
};

class Module : public Block {
public:
    explicit Module(std::string name, Runtime& runtime)
        : Block(nullptr)
        , m_name(move(name))
        , m_runtime(runtime)
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("module %s\n\n", m_name.c_str());
        Block::dump(indent);
    }

    [[nodiscard]] Module const* module() const override;
    [[nodiscard]] Runtime& runtime() const override;

private:
    std::string m_name;
    Runtime& m_runtime;
};

class FunctionDef : public Block {
public:
    FunctionDef(SyntaxNode* parent, std::string name, std::vector<std::string> parameters)
        : Block(parent)
        , m_name(move(name))
        , m_parameters(move(parameters))
    {
    }

    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::vector<std::string> const& parameters() const { return m_parameters; }

    void dump(int indent) override
    {
        dump_arguments(indent);
        printf("\n");
        Block::dump(indent);
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        auto bound_function = make_obj<BoundFunction>(scope, *this);
        scope->declare(name(), bound_function);
        return { ExecutionResultCode::None, bound_function };
    }

    std::string const& name() { return m_name; }

protected:
    void dump_arguments(int indent)
    {
        indent_line(indent);
        printf("func %s(", m_name.c_str());
        bool first = true;
        for (auto& arg : m_parameters) {
            if (!first)
                printf(", ");
            first = false;
            printf("%s", arg.c_str());
        }
        printf(")");
    }

    std::string m_name;
    std::vector<std::string> m_parameters;
};

class NativeFunctionDef : public FunctionDef {
public:
    NativeFunctionDef(SyntaxNode* parent, std::string name, std::vector<std::string> parameters, std::string native_function_name)
        : FunctionDef(parent, move(name), move(parameters))
        , m_native_function_name(move(native_function_name))
    {
    }

    void dump(int indent) override
    {
        dump_arguments(indent);
        printf(" -> \"%s\"\n", m_native_function_name.c_str());
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        auto native_function = Obelix::make_obj<Obelix::NativeFunction>(m_native_function_name);
        scope->declare(m_name, native_function);
        return { ExecutionResultCode::None, native_function };
    }

private:
    std::string m_native_function_name;
};

class Expression : public SyntaxNode {
public:
    explicit Expression(SyntaxNode* parent)
        : SyntaxNode(parent)
    {
    }

    virtual Obj evaluate(Ptr<Scope>)
    {
        fatal("Not yet implemented");
    }

    virtual std::optional<Obj> assign(Ptr<Scope>, Obj const&)
    {
        return {};
    }
};

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::shared_ptr<Expression> expression)
        : Statement(expression->parent())
        , m_expression(move(expression))
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        m_expression->dump(indent);
        printf("\n");
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        m_expression->evaluate(scope);
        return {};
    }

private:
    std::shared_ptr<Expression> m_expression;
};

class Literal : public Expression {
public:
    Literal(SyntaxNode* parent, Token const& token)
        : Expression(parent)
        , m_literal(token.to_object())
    {
    }

    Literal(SyntaxNode* parent, Obj value)
        : Expression(parent)
        , m_literal(std::move(value))
    {
    }

    void dump(int indent) override
    {
        printf("%s", m_literal->to_string().c_str());
    }

    Obj evaluate(Ptr<Scope>) override
    {
        return m_literal;
    }

private:
    Obj m_literal;
};

class This : public Expression {
public:
    explicit This(SyntaxNode* parent)
        : Expression(parent)
    {
    }

    void dump(int indent) override
    {
        printf("this");
    }

    Obj evaluate(Ptr<Scope> scope) override
    {
        return to_obj(scope);
    }
};

class VariableReference : public Expression {
public:
    VariableReference(SyntaxNode* parent, std::string name)
        : Expression(parent)
        , m_name(std::move(name))
    {
    }

    void dump(int indent) override
    {
        printf("%s", m_name.c_str());
    }

    [[nodiscard]] Obj evaluate(Ptr<Scope> scope) override
    {
        auto value_maybe = scope->resolve(m_name);
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
        : Expression(lhs->parent())
        , m_lhs(std::move(lhs))
        , m_operator(op.value())
        , m_rhs(std::move(rhs))
    {
    }

    BinaryExpression(std::shared_ptr<Expression> lhs, std::string op, std::shared_ptr<Expression> rhs)
        : Expression(lhs->parent())
        , m_lhs(std::move(lhs))
        , m_operator(move(op))
        , m_rhs(std::move(rhs))
    {
    }

    void dump(int indent) override
    {
        printf("(");
        m_lhs->dump(indent);
        printf(") %s (", m_operator.c_str());
        m_rhs->dump(indent);
        printf(")");
    }

    std::optional<Obj> assign(Ptr<Scope> scope, Obj const& value) override
    {
        Obj lhs = m_lhs->evaluate(scope);
        Obj rhs = m_rhs->evaluate(scope);
        return lhs->assign(rhs->to_string(), value);
    }

    Obj evaluate(Ptr<Scope> scope) override
    {
        Obj rhs = m_rhs->evaluate(scope);
        if (m_operator == "=") {
            if (auto ret_maybe = m_lhs->assign(scope, rhs); ret_maybe.has_value()) {
                return ret_maybe.value();
            } else {
                return make_obj<Exception>(ErrorCode::SyntaxError, "Could not assign to non-lvalue");
            }
        }
        Obj lhs = m_lhs->evaluate(scope);
        auto ret_maybe = lhs.evaluate(m_operator, make_typed<Arguments>(rhs));
        if (!ret_maybe.has_value())
            return make_obj<Exception>(ErrorCode::FunctionUndefined, "Could not resolve operator");
        return ret_maybe.value();
    }

private:
    std::shared_ptr<Expression> m_lhs;
    std::string m_operator;
    std::shared_ptr<Expression> m_rhs;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(Token op, std::shared_ptr<Expression> operand)
        : Expression(operand->parent())
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

    Obj evaluate(Ptr<Scope> scope) override
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
    FunctionCall(std::shared_ptr<Expression> function, std::vector<std::shared_ptr<Expression>> arguments)
        : Expression(function->parent())
        , m_function(move(function))
        , m_arguments(move(arguments))
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        m_function->dump(indent);
        printf("(");
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                printf(", ");
            first = false;
            arg->dump(indent);
        }
        printf(")");
    }

    Obj evaluate(Ptr<Scope> scope) override
    {
        auto callable = m_function->evaluate(scope);
        auto args = make_typed<Arguments>();
        for (auto& arg : m_arguments) {
            args->add(arg->evaluate(scope));
        }
        return callable->call(args);
    }

private:
    std::shared_ptr<Expression> m_function;
    std::vector<std::shared_ptr<Expression>> m_arguments;
};

class Assignment : public Statement {
public:
    Assignment(std::string variable, std::shared_ptr<Expression> expression, bool declaration = false)
        : Statement(expression->parent())
        , m_variable(move(variable))
        , m_declaration(declaration)
        , m_expression(move(expression))
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        if (m_declaration)
            printf("var ");
        printf("%s = ", m_variable.c_str());
        m_expression->dump(indent);
        printf("\n");
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        auto value = m_expression->evaluate(scope);
        if (m_declaration) {
            scope->declare(m_variable, value);
        } else {
            scope->set(m_variable, value);
        }
        return { ExecutionResultCode::None, value };
    }

private:
    std::string m_variable;
    bool m_declaration { false };
    std::shared_ptr<Expression> m_expression;
};

class Return : public Statement {
public:
    explicit Return(SyntaxNode* parent, std::shared_ptr<Expression> expression)
        : Statement(parent)
        , m_expression(move(expression))
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("return ");
        m_expression->dump(indent);
        printf("\n");
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        return { ExecutionResultCode::Return, m_expression->evaluate(scope) };
    }

private:
    std::shared_ptr<Expression> m_expression;
};

class Break : public Statement {
public:
    explicit Break(SyntaxNode* parent)
        : Statement(parent)
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("break\n");
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        return { ExecutionResultCode::Break, {} };
    }
};

class Continue : public Statement {
public:
    explicit Continue(SyntaxNode* parent)
        : Statement(parent)
    {
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("continue\n");
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        return { ExecutionResultCode::Continue, {} };
    }
};

class ProcedureCall : public Statement {
public:
    ProcedureCall(SyntaxNode* parent, std::shared_ptr<FunctionCall> call_expression)
        : Statement(parent)
        , m_call_expression(move(call_expression))
    {
    }

    void dump(int indent) override
    {
        m_call_expression->dump(indent);
        printf("\n");
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        return { ExecutionResultCode::None, m_call_expression->evaluate(scope) };
    }

private:
    std::shared_ptr<FunctionCall> m_call_expression;
};

class Branch : public Statement {
public:
    Branch(std::string keyword, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
        : Statement(statement->parent())
        , m_keyword(move(keyword))
        , m_condition(move(condition))
        , m_statement(move(statement))
    {
    }

    Branch(std::string keyword, std::shared_ptr<Statement> statement)
        : Statement(statement->parent())
        , m_keyword(move(keyword))
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] std::shared_ptr<Expression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("%s ", m_keyword.c_str());
        if (m_condition)
            m_condition->dump(indent);
        printf("\n");
        m_statement->dump(indent + 2);
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        if (!m_condition || m_condition->evaluate(scope)) {
            return m_statement->execute(scope);
        }
        return { ExecutionResultCode::Skipped };
    }

private:
    std::string m_keyword;
    std::shared_ptr<Expression> m_condition { nullptr };
    std::shared_ptr<Statement> m_statement;
};

class ElseStatement : public Branch {
public:
    explicit ElseStatement(std::shared_ptr<Statement> const& stmt)
        : Branch("else", stmt)
    {
    }
};

class ElifStatement : public Branch {
public:
    ElifStatement(std::shared_ptr<Expression> const& condition, std::shared_ptr<Statement> const& stmt)
        : Branch("elif", stmt)
    {
    }
};

class IfStatement : public Branch {
public:
    IfStatement(std::shared_ptr<Expression> const& condition, std::shared_ptr<Statement> const& if_stmt)
        : Branch("if", condition, if_stmt)
    {
    }

    void append_elif(std::shared_ptr<Expression> const& condition, std::shared_ptr<Statement> const& stmt)
    {
        m_elseifs.push_back(std::make_shared<ElifStatement>(condition, stmt));
    }

    void append_else(std::shared_ptr<Statement> const& else_stmt)
    {
        m_else = std::make_shared<ElseStatement>(else_stmt);
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        if (auto result = Branch::execute(scope); result.code != ExecutionResultCode::Skipped)
            return result;
        for (auto& elif : m_elseifs) {
            if (auto result = elif->execute(scope); result.code != ExecutionResultCode::Skipped)
                return result;
        }
        if (m_else)
            return m_else->execute(scope);
        return {};
    }

    void dump(int indent) override
    {
        Branch::dump(indent);
        for (auto& elif : m_elseifs) {
            elif->dump(indent);
        }
        if (m_else)
            m_else->dump(indent);
    }

private:
    std::vector<std::shared_ptr<ElifStatement>> m_elseifs {};
    std::shared_ptr<Statement> m_else {};
};

class WhileStatement : public Statement {
public:
    WhileStatement(SyntaxNode* parent, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> stmt)
        : Statement(parent)
        , m_condition(move(condition))
        , m_stmt(move(stmt))
    {
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        ExecutionResult result;
        for (Obj condition = m_condition->evaluate(scope); condition->to_bool().value(); condition = m_condition->evaluate(scope)) {
            result = m_stmt->execute(scope);
            if (result.code == ExecutionResultCode::Break || result.code == ExecutionResultCode::Return)
                return {};
        }
        return result;
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

class CaseStatement : public Branch {
public:
    explicit CaseStatement(std::shared_ptr<Expression> case_expression, std::shared_ptr<Statement> const& stmt)
        : Branch("case", case_expression, stmt)
    {
    }
};

class DefaultCase : public Branch {
public:
    explicit DefaultCase(std::shared_ptr<Statement> const& stmt)
        : Branch("default", stmt)
    {
    }
};

class SwitchStatement : public Statement {
public:
    explicit SwitchStatement(SyntaxNode* parent, std::shared_ptr<Expression> switch_expression)
        : Statement(parent)
        , m_switch_expression(move(switch_expression))
    {
    }

    void append_case(std::shared_ptr<Expression> case_expression, std::shared_ptr<Statement> statement)
    {
        m_cases.push_back(std::make_shared<CaseStatement>(
            std::make_shared<BinaryExpression>(m_switch_expression, Token(TokenCode::EqualsTo, "=="), move(case_expression)),
            move(statement)));
    }

    void set_default(std::shared_ptr<Statement> statement)
    {
        if (m_default) {
            fprintf(stderr, "Switch statement already has a default case\n");
            exit(1);
        }
        m_default = std::make_shared<DefaultCase>(move(statement));
    }

    ExecutionResult execute(Ptr<Scope> scope) override
    {
        for (auto& case_stmt : m_cases) {
            if (auto result = case_stmt->execute(scope); result.code != ExecutionResultCode::Skipped)
                return result;
        }
        if (m_default)
            return m_default->execute(scope);
        return {};
    }

    void dump(int indent) override
    {
        indent_line(indent);
        printf("switch (");
        m_switch_expression->dump(indent);
        printf(") {\n");
        for (auto& case_stmt : m_cases) {
            case_stmt->dump(indent);
        }
        if (m_default)
            m_default->dump(indent);
        indent_line(indent);
        printf("}\n");
    }

    [[nodiscard]] std::shared_ptr<DefaultCase> const& default_case() const { return m_default; }

private:
    std::shared_ptr<Expression> m_switch_expression;
    std::vector<std::shared_ptr<CaseStatement>> m_cases {};
    std::shared_ptr<DefaultCase> m_default {};
};

}