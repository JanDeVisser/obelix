//
// Created by Jan de Visser on 2021-09-18.
//

#pragma once

#include <core/List.h>
#include <core/Logging.h>
#include <core/NativeFunction.h>
#include <core/Range.h>
#include <lexer/Token.h>
#include <obelix/BoundFunction.h>
#include <obelix/Runtime.h>
#include <obelix/Scope.h>
#include <string>

namespace Obelix {

extern_logging_category(parser);

class Module;

static inline std::string pad(int num)
{
    std::string ret;
    for (auto ix = 0; ix < num; ++ix)
        ret += ' ';
    return ret;
}

class SyntaxNode {
public:
    explicit SyntaxNode(SyntaxNode* parent)
        : m_parent(parent)
    {
    }

    virtual ~SyntaxNode() = default;
    virtual std::string to_string(int) = 0;

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

    virtual ExecutionResult execute(Ptr<Scope>&)
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

    std::string to_string(int indent) override
    {
        return format("{}import {}", pad(indent), m_name);
    }

    ExecutionResult execute(Ptr<Scope>&) override;

private:
    std::string m_name;
};

class Pass : public Statement {
public:
    explicit Pass(SyntaxNode* parent)
        : Statement(parent)
    {
    }

    std::string to_string(int indent) override
    {
        return format("{};", pad(indent));
    }

    ExecutionResult execute(Ptr<Scope>&) override
    {
        return {};
    }
};

class Block : public Statement {
public:
    explicit Block(SyntaxNode* parent, std::vector<std::shared_ptr<Statement>> statements)
        : Statement(parent)
        , m_statements(move(statements))
        , m_scope(make_null<Scope>())
    {
    }

    std::string to_string(int indent) override
    {
        auto ret = format("{}{\n", pad(indent));
        for (auto& statement : m_statements) {
            ret += statement->to_string(indent + 2);
            ret += "\n";
        }
        ret += format("{}}", pad(indent));
        return ret;
    }

    [[nodiscard]] ExecutionResult execute_block(Ptr<Scope>& block_scope) const
    {
        ExecutionResult result;
        for (auto& statement : m_statements) {
            result = statement->execute(block_scope);
            if (result.code != ExecutionResultCode::None)
                return result;
        }
        block_scope->set_result(result);
        return result;
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        m_scope = make_typed<Scope>(scope);
        return execute_block(m_scope);
    }

    [[nodiscard]] Ptr<Scope>& scope() { return m_scope; }

protected:
    std::vector<std::shared_ptr<Statement>> m_statements {};
    Ptr<Scope> m_scope;
};

class Module : public Block {
public:
    explicit Module(std::string name, Runtime& runtime, std::vector<std::shared_ptr<Statement>> const& statements)
        : Block(nullptr, statements)
        , m_name(move(name))
        , m_runtime(runtime)
    {
    }

    std::string to_string(int indent) override
    {
        auto ret = format("{}module {}\n", pad(indent), m_name);
        ret += Block::to_string(indent);
        return ret;
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        m_scope = (scope) ? scope : make_typed<Scope>(scope);
        return execute_block(m_scope);
    }

    [[nodiscard]] Module const* module() const override;
    [[nodiscard]] Runtime& runtime() const override;

private:
    std::string m_name;
    Runtime& m_runtime;
};

class FunctionDef : public Block {
public:
    FunctionDef(SyntaxNode* parent, std::string name, std::vector<std::string> parameters, std::vector<std::shared_ptr<Statement>> const& statements)
        : Block(parent, statements)
        , m_name(move(name))
        , m_parameters(move(parameters))
    {
    }

    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::vector<std::string> const& parameters() const { return m_parameters; }

    std::string to_string(int indent) override
    {
        auto ret = to_string_arguments(indent);
        ret += '\n';
        ret += Block::to_string(indent);
        return ret;
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        auto bound_function = make_obj<BoundFunction>(scope, *this);
        scope->declare(name(), bound_function);
        return { ExecutionResultCode::None, bound_function };
    }

    std::string const& name() { return m_name; }

protected:
    std::string to_string_arguments(int indent)
    {
        auto ret = format("{}func {}(", pad(indent), m_name);
        bool first = true;
        for (auto& arg : m_parameters) {
            if (!first)
                ret += ", ";
            first = false;
            ret += arg;
        }
        ret += ')';
        return ret;
    }

    std::string m_name;
    std::vector<std::string> m_parameters;
};

class NativeFunctionDef : public FunctionDef {
public:
    NativeFunctionDef(SyntaxNode* parent, std::string name, std::vector<std::string> parameters, std::string native_function_name)
        : FunctionDef(parent, move(name), move(parameters), std::vector<std::shared_ptr<Statement>>())
        , m_native_function_name(move(native_function_name))
    {
    }

    std::string to_string(int indent) override
    {
        return format("{}{} -> \"{}\"", pad(indent), to_string_arguments(indent), m_native_function_name);
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
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

    virtual Obj evaluate(Ptr<Scope>& scope)
    {
        fatal("Not yet implemented");
    }

    virtual std::optional<Obj> assign(Ptr<Scope>& scope, Token const&, Obj const&)
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

    std::string to_string(int indent) override
    {
        return format("{}{}", pad(indent), m_expression->to_string(indent));
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        auto result = m_expression->evaluate(scope);
        return { (result->is_exception()) ? ExecutionResultCode::Error : ExecutionResultCode::None, result };
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

    std::string to_string(int indent) override
    {
        return m_literal->to_string();
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        return m_literal;
    }

private:
    Obj m_literal;
};

class Identifier : public Expression {
public:
    Identifier(SyntaxNode* parent, std::string identifier)
        : Expression(parent)
        , m_identifier(move(identifier))
    {
    }

    std::string to_string(int indent) override
    {
        return m_identifier;
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        return make_obj<String>(m_identifier);
    }

private:
    std::string m_identifier;
};

class ListLiteral : public Expression {
public:
    ListLiteral(SyntaxNode* parent, std::vector<std::shared_ptr<Expression>> elements)
        : Expression(parent)
        , m_elements(move(elements))
    {
    }

    std::string to_string(int indent) override
    {
        std::string ret = "[";
        bool first = true;
        for (auto& e : m_elements) {
            if (!first)
                ret += ", ";
            ret += e->to_string(indent);
            first = false;
        }
        ret += "]";
        return ret;
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        Ptr<List> list = make_typed<List>();
        for (auto& e : m_elements) {
            auto result = e->evaluate(scope);
            if (result->is_exception())
                return result;
            list->push_back(result);
        }
        return to_obj(list);
    }

private:
    std::vector<std::shared_ptr<Expression>> m_elements;
};

class ListComprehension : public Expression {
public:
    ListComprehension(std::shared_ptr<Expression> element,
        std::string rangevar,
        std::shared_ptr<Expression> generator,
        std::shared_ptr<Expression> condition = nullptr)
        : Expression(element->parent())
        , m_element(move(element))
        , m_rangevar(move(rangevar))
        , m_generator(move(generator))
        , m_condition(move(condition))
    {
    }

    std::string to_string(int indent) override
    {
        auto ret = format("[ {} for {} in {}", m_element->to_string(indent), m_rangevar, m_generator->to_string(indent));
        if (m_condition)
            ret += format(" where {}", m_condition->to_string(indent));
        return ret;
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        auto list = make_typed<List>();
        auto new_scope = make_typed<Scope>(scope);
        auto range = m_generator->evaluate(new_scope);
        if (range->is_exception())
            return range;
        new_scope->declare(m_rangevar, make_obj<Integer>(0));
        for (auto const& value : range) {
            new_scope->set(m_rangevar, value);
            auto elem = m_element->evaluate(new_scope);
            if (elem->is_exception())
                return elem;
            if (m_condition) {
                auto include_elem = m_condition->evaluate(new_scope);
                if (include_elem->is_exception())
                    return include_elem;
                auto include_maybe = include_elem->to_bool();
                if (!include_maybe.has_value())
                    return make_obj<Exception>(ErrorCode::ConversionError, "Cannot convert list comprehension condition result '{}' to boolean", include_elem->to_string());
                if (!include_maybe.value())
                    continue;
            }
            list->push_back(elem);
        }
        return to_obj(list);
    }

private:
    std::shared_ptr<Expression> m_element;
    std::string m_rangevar;
    std::shared_ptr<Expression> m_generator;
    std::shared_ptr<Expression> m_condition;
};

struct DictionaryLiteralEntry {
    std::string name;
    std::shared_ptr<Expression> value;
};

typedef std::vector<DictionaryLiteralEntry> DictionaryLiteralEntries;

class DictionaryLiteral : public Expression {
public:
    DictionaryLiteral(SyntaxNode* parent, DictionaryLiteralEntries entries)
        : Expression(parent)
        , m_entries(move(entries))
    {
    }

    std::string to_string(int indent) override
    {
        std::string ret = "{";
        bool first = true;
        for (auto& e : m_entries) {
            if (!first)
                ret += ", ";
            ret += format("{}: {}", e.name, e.value->to_string(indent));
            first = false;
        }
        ret += "}";
        return ret;
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        auto dictionary = make_typed<Dictionary>();
        for (auto& e : m_entries) {
            auto result = e.value->evaluate(scope);
            if (result->is_exception())
                return result;
            dictionary->put(e.name, result);
        }
        return to_obj(dictionary);
    }

private:
    DictionaryLiteralEntries m_entries;
};

class This : public Expression {
public:
    explicit This(SyntaxNode* parent)
        : Expression(parent)
    {
    }

    std::string to_string(int indent) override
    {
        return "this";
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        return to_obj(scope);
    }
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs)
        : Expression(lhs->parent())
        , m_lhs(std::move(lhs))
        , m_operator(std::move(op))
        , m_rhs(std::move(rhs))
    {
    }

    std::string to_string(int indent) override
    {
        return format("({}) {} ({})", m_lhs->to_string(indent), m_operator, m_rhs->to_string(indent));
    }

    std::optional<Obj> assign(Ptr<Scope>& scope, Token const& op, Obj const& value) override;
    Obj evaluate(Ptr<Scope>& scope) override;

private:
    std::shared_ptr<Expression> m_lhs;
    Token m_operator;
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

    std::string to_string(int indent) override
    {
        return format("{}({})", m_operator.value(), m_operand->to_string(indent));
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        Obj operand = m_operand->evaluate(scope);
        if (operand->is_exception())
            return operand;
        auto ret_maybe = operand->evaluate(m_operator.value(), make_typed<Arguments>());
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

    std::string to_string(int indent) override
    {
        auto ret = format("{}(", m_function->to_string(indent));
        bool first = true;
        for (auto& arg : m_arguments) {
            if (!first)
                ret += ", ";
            first = false;
            ret += arg->to_string(indent);
        }
        ret += ')';
        return ret;
    }

    Obj evaluate(Ptr<Scope>& scope) override
    {
        auto callable = m_function->evaluate(scope);
        if (callable->is_exception())
            return callable;
        auto args = make_typed<Arguments>();
        for (auto& arg : m_arguments) {
            auto evaluated_arg = arg->evaluate(scope);
            if (evaluated_arg->is_exception())
                return evaluated_arg;
            args->add(evaluated_arg);
        }
        return callable->call(args);
    }

private:
    std::shared_ptr<Expression> m_function;
    std::vector<std::shared_ptr<Expression>> m_arguments;
};

class VariableDeclaration : public Statement {
public:
    VariableDeclaration(SyntaxNode* parent, std::string variable, std::shared_ptr<Expression> expression = nullptr)
        : Statement(parent)
        , m_variable(move(variable))
        , m_expression(move(expression))
    {
    }

    std::string to_string(int indent) override
    {
        return format("{}var {}", pad(indent), m_variable);
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        if (scope->contains(m_variable))
            return { ExecutionResultCode::Error, make_obj<Exception>(ErrorCode::VariableAlreadyDeclared, m_variable) };
        Obj value;
        if (m_expression) {
            value = m_expression->evaluate(scope);
            if (value->is_exception())
                return { ExecutionResultCode::Error, value };
        } else {
            value = make_obj<Integer>(0);
        }
        scope->declare(m_variable, value);
        return { ExecutionResultCode::None, value };
    }

private:
    std::string m_variable;
    std::shared_ptr<Expression> m_expression;
};

class Return : public Statement {
public:
    explicit Return(SyntaxNode* parent, std::shared_ptr<Expression> expression)
        : Statement(parent)
        , m_expression(move(expression))
    {
    }

    std::string to_string(int indent) override
    {
        return format("{}return {}", pad(indent), m_expression->to_string(indent));
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        auto result = m_expression->evaluate(scope);
        return { (result->is_exception()) ? ExecutionResultCode::Error : ExecutionResultCode::Return, result };
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

    std::string to_string(int indent) override
    {
        return format("{}break", pad(indent));
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
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

    std::string to_string(int indent) override
    {
        return format("{}continue", pad(indent));
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        return { ExecutionResultCode::Continue, {} };
    }
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

    std::string to_string(int indent) override
    {
        auto ret = format("{}{} ", pad(indent), m_keyword);
        if (m_condition)
            ret += m_condition->to_string(indent);
        ret += '\n';
        ret += m_statement->to_string(indent + 2);
        return ret;
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        Obj condition_result;
        if (m_condition) {
            condition_result = m_condition->evaluate(scope);
            if (condition_result->is_exception())
                return { ExecutionResultCode::Error, condition_result };
        }
        if (!m_condition || condition_result) {
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

typedef std::vector<std::shared_ptr<ElifStatement>> ElifStatements;

class IfStatement : public Branch {
public:
    IfStatement(std::shared_ptr<Expression> const& condition,
        std::shared_ptr<Statement> if_stmt,
        ElifStatements elifs,
        std::shared_ptr<ElseStatement> else_stmt)
        : Branch("if", condition, move(if_stmt))
        , m_elifs(move(elifs))
        , m_else(move(else_stmt))
    {
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        if (auto result = Branch::execute(scope); result.code != ExecutionResultCode::Skipped)
            return result;
        for (auto& elif : m_elifs) {
            if (auto result = elif->execute(scope); result.code != ExecutionResultCode::Skipped)
                return result;
        }
        if (m_else)
            return m_else->execute(scope);
        return {};
    }

    std::string to_string(int indent) override
    {
        auto ret = Branch::to_string(indent);
        for (auto& elif : m_elifs) {
            ret += elif->to_string(indent);
        }
        if (m_else)
            ret += m_else->to_string(indent);
        return ret;
    }

private:
    ElifStatements m_elifs {};
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

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        ExecutionResult result { ExecutionResultCode::None, make_obj<Null>() };
        do {
            auto condition_value = m_condition->evaluate(scope);
            if (condition_value->is_exception())
                return { ExecutionResultCode::Error, condition_value };
            auto bool_value = condition_value->to_bool();
            if (!bool_value.has_value())
                return { ExecutionResultCode::Error, make_obj<Exception>(ErrorCode::ConversionError, bool_value, "bool") };
            if (!bool_value.value())
                break;
            result = m_stmt->execute(scope);
            if (result.code == ExecutionResultCode::Error)
                return result;
            if (result.code == ExecutionResultCode::Break)
                return {};
            if (result.code == ExecutionResultCode::Return)
                return result;
        } while (result.code == ExecutionResultCode::None);
        return result;
    }

    std::string to_string(int indent) override
    {
        return format("{}while {}\n{}", pad(indent), m_condition->to_string(indent), m_stmt->to_string(indent + 2));
    }

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

class ForStatement : public Statement {
public:
    ForStatement(SyntaxNode* parent, std::string variable, std::shared_ptr<Expression> range, std::shared_ptr<Statement> stmt)
        : Statement(parent)
        , m_variable(move(variable))
        , m_range(move(range))
        , m_stmt(move(stmt))
    {
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        ExecutionResult result;
        auto new_scope = make_typed<Scope>(scope);
        new_scope->declare(m_variable, make_obj<Integer>(0));
        auto range = m_range->evaluate(new_scope);
        if (range->is_exception())
            return { ExecutionResultCode::Error, range };
        for (auto& value : range) {
            new_scope->set(m_variable, value);
            result = m_stmt->execute(new_scope);
            if (result.code == ExecutionResultCode::Error)
                return result;
            if (result.code == ExecutionResultCode::Break)
                return {};
            if (result.code == ExecutionResultCode::Return)
                return result;
        }
        return result;
    }

    std::string to_string(int indent) override
    {
        return format("{}for {} in {}\n{}", pad(indent), m_variable, m_range->to_string(indent), m_stmt->to_string(indent + 2));
    }

private:
    std::string m_variable {};
    std::shared_ptr<Expression> m_range;
    std::shared_ptr<Statement> m_stmt;
};

class CaseStatement : public Branch {
public:
    explicit CaseStatement(std::shared_ptr<Expression> const& case_expression, std::shared_ptr<Statement> const& stmt)
        : Branch("case", case_expression, stmt)
    {
    }
};

typedef std::vector<std::shared_ptr<CaseStatement>> CaseStatements;

class DefaultCase : public Branch {
public:
    explicit DefaultCase(std::shared_ptr<Statement> const& stmt)
        : Branch("default", stmt)
    {
    }
};

class SwitchStatement : public Statement {
public:
    explicit SwitchStatement(SyntaxNode* parent, std::shared_ptr<Expression> switch_expression,
        CaseStatements case_statements, std::shared_ptr<DefaultCase> default_case)
        : Statement(parent)
        , m_switch_expression(move(switch_expression))
        , m_cases(move(case_statements))
        , m_default(move(default_case))
    {
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        for (auto& case_stmt : m_cases) {
            if (auto result = case_stmt->execute(scope); result.code != ExecutionResultCode::Skipped)
                return result;
        }
        if (m_default)
            return m_default->execute(scope);
        return {};
    }

    std::string to_string(int indent) override
    {
        auto ret = format("{}switch ({}) {\n", pad(indent), m_switch_expression->to_string(indent));
        for (auto& case_stmt : m_cases) {
            ret += case_stmt->to_string(indent);
        }
        if (m_default)
            ret += m_default->to_string(indent);
        ret += format("{}}", pad(indent));
        return ret;
    }

private:
    std::shared_ptr<Expression> m_switch_expression;
    std::vector<std::shared_ptr<CaseStatement>> m_cases {};
    std::shared_ptr<DefaultCase> m_default {};
};

}