/*
 * Syntax.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <core/List.h>
#include <core/Logging.h>
#include <core/NativeFunction.h>
#include <core/Range.h>
#include <lexer/Token.h>
#include <obelix/Runtime.h>
#include <obelix/Scope.h>
#include <obelix/Symbol.h>
#include <obelix/Type.h>
#include <string>

namespace Obelix {

extern_logging_category(parser);

#define ENUMERATE_SYNTAXNODETYPES(S) \
    S(SyntaxNode)                    \
    S(Statement)                     \
    S(Block)                         \
    S(Module)                        \
    S(Expression)                    \
    S(TypedExpression)               \
    S(Literal)                       \
    S(Identifier)                    \
    S(Symbol)                        \
    S(ListLiteral)                   \
    S(ListComprehension)             \
    S(DictionaryLiteral)             \
    S(This)                          \
    S(BinaryExpression)              \
    S(UnaryExpression)               \
    S(FunctionCall)                  \
    S(Import)                        \
    S(Pass)                          \
    S(FunctionDef)                   \
    S(NativeFunctionDef)             \
    S(ExpressionStatement)           \
    S(VariableDeclaration)           \
    S(Return)                        \
    S(Break)                         \
    S(Continue)                      \
    S(Branch)                        \
    S(ElseStatement)                 \
    S(ElifStatement)                 \
    S(IfStatement)                   \
    S(WhileStatement)                \
    S(ForStatement)                  \
    S(CaseStatement)                 \
    S(DefaultCase)                   \
    S(SwitchStatement)

enum class SyntaxNodeType {
#undef __SYNTAXNODETYPE
#define __SYNTAXNODETYPE(type) type,
    ENUMERATE_SYNTAXNODETYPES(__SYNTAXNODETYPE)
#undef __SYNTAXNODETYPE
};

constexpr char const* SyntaxNodeType_name(SyntaxNodeType type)
{
    switch (type) {
#undef __SYNTAXNODETYPE
#define __SYNTAXNODETYPE(type) \
    case SyntaxNodeType::type: \
        return #type;
        ENUMERATE_SYNTAXNODETYPES(__SYNTAXNODETYPE)
#undef __SYNTAXNODETYPE
    }
}

typedef std::vector<std::string> Strings;

class SyntaxNode;
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
    SyntaxNode() = default;
    virtual ~SyntaxNode() = default;

    [[nodiscard]] virtual std::string to_string(int) const = 0;
    [[nodiscard]] virtual SyntaxNodeType node_type() const = 0;
};

class Statement : public SyntaxNode {
public:
    Statement() = default;
    virtual ExecutionResult execute(Ptr<Scope>&) = 0;
};

class Expression : public SyntaxNode {
public:
    Expression()
        : SyntaxNode()
    {
    }

    virtual Obj evaluate(Ptr<Scope>& scope) = 0;
    virtual std::optional<Obj> assign(Ptr<Scope>& scope, Token const&, Obj const&) { return {}; }
};

class TypedExpression : public Expression {
public:
    TypedExpression(std::shared_ptr<Expression> expression, ObelixType type)
        : Expression()
        , m_expression(move(expression))
        , m_type(type)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::TypedExpression; }
    Obj evaluate(Ptr<Scope>& scope) override { return m_expression->evaluate(scope); }
    std::optional<Obj> assign(Ptr<Scope>& scope, Token const& op, Obj const& value) override { return m_expression->assign(scope, op, value); }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }
    [[nodiscard]] ObelixType type() const { return m_type; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{} : {}", m_expression->to_string(indent), ObelixType_name(m_type));
    }

private:
    std::shared_ptr<Expression> m_expression;
    ObelixType m_type { ObelixType::Unknown };
};

class Import : public Statement {
public:
    explicit Import(std::string name)
        : Statement()
        , m_name(move(name))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Import; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}import {}", pad(indent), m_name);
    }

    ExecutionResult execute(Ptr<Scope>&) override;

private:
    std::string m_name;
};

class Pass : public Statement {
public:
    Pass() = default;
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Pass; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{};", pad(indent));
    }

    ExecutionResult execute(Ptr<Scope>&) override
    {
        return {};
    }
};

typedef std::vector<std::shared_ptr<Statement>> Statements;

class Block : public Statement {
public:
    explicit Block(Statements statements)
        : Statement()
        , m_statements(move(statements))
        , m_scope(make_null<Scope>())
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Block; }

    [[nodiscard]] std::string to_string(int indent) const override
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
    [[nodiscard]] Statements const& statements() const { return m_statements; }

protected:
    Statements m_statements {};
    Ptr<Scope> m_scope;
};

class Module : public Block {
public:
    explicit Module(std::string name, std::vector<std::shared_ptr<Statement>> const& statements)
        : Block(statements)
        , m_name(move(name))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Module; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}module {}\n", pad(indent), m_name);
        ret += Block::to_string(indent);
        return ret;
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        Ptr<Scope> s = (scope) ? scope : make_typed<Scope>(scope);
        return execute_in(s);
    }

    ExecutionResult execute_in(Ptr<Scope>& scope)
    {
        m_scope = scope;
        return execute_block(m_scope);
    }

    [[nodiscard]] const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

class FunctionDef : public Statement {
public:
    FunctionDef(std::string name, ObelixType return_type, Symbols parameters, std::shared_ptr<Statement> statement)
        : Statement()
        , m_identifier(move(name), return_type)
        , m_parameters(move(parameters))
        , m_statement(move(statement))
    {
    }

    FunctionDef(Symbol identifier, Symbols parameters, std::shared_ptr<Statement> statement)
        : Statement()
        , m_identifier(std::move(identifier))
        , m_parameters(move(parameters))
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionDef; }
    [[nodiscard]] Symbol const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return m_identifier.identifier(); }
    [[nodiscard]] Symbols const& parameters() const { return m_parameters; }
    ExecutionResult execute(Ptr<Scope>& scope) override;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = to_string_arguments(indent);
        ret += '\n';
        ret += m_statement->to_string(indent + 2);
        return ret;
    }

protected:
    FunctionDef(Symbol identifier, Symbols parameters)
        : Statement()
        , m_identifier(std::move(identifier))
        , m_parameters(move(parameters))
        , m_statement(nullptr)
    {
    }

    [[nodiscard]] std::string to_string_arguments(int indent) const
    {
        auto ret = format("{}func {}(", pad(indent), identifier().identifier());
        bool first = true;
        for (auto& arg : m_parameters) {
            if (!first)
                ret += ", ";
            first = false;
            ret += arg.identifier();
        }
        ret += format(") : {}", ObelixType_name(identifier().type()));
        return ret;
    }

    Symbol m_identifier;
    Symbols m_parameters;
    std::shared_ptr<Statement> m_statement;
};

class NativeFunctionDef : public FunctionDef {
public:
    NativeFunctionDef(Symbol name, Symbols parameters, std::string native_function_name)
        : FunctionDef(std::move(name), move(parameters))
        , m_native_function_name(move(native_function_name))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::NativeFunctionDef; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}{} -> \"{}\"", to_string_arguments(indent), m_native_function_name);
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        auto native_function = Obelix::make_obj<Obelix::NativeFunction>(m_native_function_name);
        scope->declare(identifier(), native_function);
        return { ExecutionResultCode::None, native_function };
    }

private:
    std::string m_native_function_name;
};

class ExpressionStatement : public Statement {
public:
    explicit ExpressionStatement(std::shared_ptr<Expression> expression)
        : Statement()
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ExpressionStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}{}", pad(indent), m_expression->to_string(indent));
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        auto result = m_expression->evaluate(scope);
        return { (result->is_exception()) ? ExecutionResultCode::Error : ExecutionResultCode::None, result };
    }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<Expression> m_expression;
};

class Identifier : public Expression {
public:
    explicit Identifier(std::string name)
        : Expression()
        , m_name(move(name))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Identifier; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] std::string to_string(int indent) const override { return m_name; }
    Obj evaluate(Ptr<Scope>& scope) override { return make_obj<String>(name()); }

private:
    std::string m_name;
};

class Literal : public Expression {
public:
    explicit Literal(Token const& token)
        : Expression()
        , m_literal(token.to_object())
    {
    }

    explicit Literal(Obj value)
        : Expression()
        , m_literal(std::move(value))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Literal; }
    [[nodiscard]] std::string to_string(int indent) const override { return m_literal->to_string(); }
    Obj evaluate(Ptr<Scope>& scope) override { return literal(); }
    [[nodiscard]] Obj const& literal() const { return m_literal; }

private:
    Obj m_literal;
};

typedef std::vector<std::shared_ptr<Expression>> Expressions;

class ListLiteral : public Expression {
public:
    explicit ListLiteral(Expressions elements)
        : Expression()
        , m_elements(move(elements))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ListLiteral; }

    [[nodiscard]] std::string to_string(int indent) const override
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
    [[nodiscard]] Expressions const& elements() const { return m_elements; }

private:
    Expressions m_elements;
};

class ListComprehension : public Expression {
public:
    ListComprehension(std::shared_ptr<Expression> element,
        std::string rangevar,
        std::shared_ptr<Expression> generator,
        std::shared_ptr<Expression> condition = nullptr)
        : Expression()
        , m_element(move(element))
        , m_rangevar(move(rangevar))
        , m_generator(move(generator))
        , m_condition(move(condition))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ListComprehension; }

    [[nodiscard]] std::string to_string(int indent) const override
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
        auto iter_maybe = range->iterator();
        if (!iter_maybe.has_value())
            return make_obj<Exception>(ErrorCode::ObjectNotIterable, range);
        auto iter = iter_maybe.value();
        new_scope->declare(m_rangevar, make_obj<Integer>(0));
        for (auto value = iter->next(); value.has_value(); value = iter->next()) {
            assert(value.has_value());
            auto v = value.value();
            new_scope->assign(m_rangevar, v);
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

    [[nodiscard]] std::shared_ptr<Expression> const& element() const { return m_element; }
    [[nodiscard]] std::string const& rangevar() const { return m_rangevar; }
    [[nodiscard]] std::shared_ptr<Expression> const& generator() const { return m_generator; }
    [[nodiscard]] std::shared_ptr<Expression> const& condition() const { return m_condition; }

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
    explicit DictionaryLiteral(DictionaryLiteralEntries entries)
        : Expression()
        , m_entries(move(entries))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::DictionaryLiteral; }

    [[nodiscard]] std::string to_string(int indent) const override
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
    [[nodiscard]] DictionaryLiteralEntries const& entries() const { return m_entries; }

private:
    DictionaryLiteralEntries m_entries;
};

class This : public Expression {
public:
    explicit This()
        : Expression()
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::This; }

    [[nodiscard]] std::string to_string(int indent) const override
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
        : Expression()
        , m_lhs(std::move(lhs))
        , m_operator(std::move(op))
        , m_rhs(std::move(rhs))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BinaryExpression; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("({}) {} ({})", m_lhs->to_string(indent), m_operator.value(), m_rhs->to_string(indent));
    }

    std::optional<Obj> assign(Ptr<Scope>& scope, Token const& op, Obj const& value) override;
    Obj evaluate(Ptr<Scope>& scope) override;

    [[nodiscard]] std::shared_ptr<Expression> const& lhs() const { return m_lhs; }
    [[nodiscard]] std::shared_ptr<Expression> const& rhs() const { return m_rhs; }
    [[nodiscard]] Token op() const { return m_operator; }

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
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::UnaryExpression; }

    [[nodiscard]] std::string to_string(int indent) const override
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

    [[nodiscard]] Token op() const { return m_operator; }
    [[nodiscard]] std::shared_ptr<Expression> const& operand() const { return m_operand; }

private:
    Token m_operator;
    std::shared_ptr<Expression> m_operand;
};

class FunctionCall : public Expression {
public:
    FunctionCall(std::shared_ptr<Expression> function, Expressions arguments)
        : Expression()
        , m_function(move(function))
        , m_arguments(move(arguments))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionCall; }

    [[nodiscard]] std::string to_string(int indent) const override
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
    [[nodiscard]] std::shared_ptr<Expression> const& function() const { return m_function; }
    [[nodiscard]] Expressions const& arguments() const { return m_arguments; }

private:
    std::shared_ptr<Expression> m_function;
    Expressions m_arguments;
};

class VariableDeclaration : public Statement {
public:
    explicit VariableDeclaration(std::string variable, ObelixType type, std::shared_ptr<Expression> expr = nullptr)
        : Statement()
        , m_variable(move(variable), type)
        , m_expression(move(expr))
    {
    }

    explicit VariableDeclaration(Symbol identifier, std::shared_ptr<Expression> expr = nullptr)
        : Statement()
        , m_variable(std::move(identifier))
        , m_expression(move(expr))
    {
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}var {} = {}", pad(indent), m_variable.identifier(), m_expression->to_string(indent));
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        if (scope->contains(m_variable.identifier()))
            return { ExecutionResultCode::Error, make_obj<Exception>(ErrorCode::VariableAlreadyDeclared, m_variable) };
        Obj value;
        if (m_expression) {
            value = m_expression->evaluate(scope);
            if (value->is_exception())
                return { ExecutionResultCode::Error, value };
        } else {
            value = make_obj<Integer>(0);
        }
        scope->declare(m_variable.identifier(), value);
        return { ExecutionResultCode::None, value };
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::VariableDeclaration; }
    [[nodiscard]] Symbol const& variable() const { return m_variable; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    Symbol m_variable;
    std::shared_ptr<Expression> m_expression;
};

class Return : public Statement {
public:
    explicit Return(std::shared_ptr<Expression> expression)
        : Statement()
        , m_expression(move(expression))
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Return; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}return {}", pad(indent), m_expression->to_string(indent));
    }

    ExecutionResult execute(Ptr<Scope>& scope) override
    {
        auto result = m_expression->evaluate(scope);
        return { (result->is_exception()) ? ExecutionResultCode::Error : ExecutionResultCode::Return, result };
    }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<Expression> m_expression;
};

class Break : public Statement {
public:
    explicit Break()
        : Statement()
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Break; }

    [[nodiscard]] std::string to_string(int indent) const override
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
    explicit Continue()
        : Statement()
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Continue; }

    [[nodiscard]] std::string to_string(int indent) const override
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
    Branch(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
        : Statement()
        , m_keyword("branch")
        , m_condition(move(condition))
        , m_statement(move(statement))
    {
    }

    Branch(std::string keyword, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement)
        : Statement()
        , m_keyword(move(keyword))
        , m_condition(move(condition))
        , m_statement(move(statement))
    {
    }

    Branch(std::string keyword, std::shared_ptr<Statement> statement)
        : Statement()
        , m_keyword(move(keyword))
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Branch; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}{} ", pad(indent), keyword());
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
    [[nodiscard]] std::string const& keyword() const { return m_keyword; }
    [[nodiscard]] std::shared_ptr<Expression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }

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
    ElseStatement(std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const& stmt)
        : Branch("else", stmt)
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ElseStatement; }
};

class ElifStatement : public Branch {
public:
    ElifStatement(std::shared_ptr<Expression> const& condition, std::shared_ptr<Statement> const& stmt)
        : Branch("elif", stmt)
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ElifStatement; }
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
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::IfStatement; }

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

    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = Branch::to_string(indent);
        for (auto& elif : m_elifs) {
            ret += elif->to_string(indent);
        }
        if (m_else)
            ret += m_else->to_string(indent);
        return ret;
    }
    [[nodiscard]] ElifStatements const& elifs() const { return m_elifs; }
    [[nodiscard]] std::shared_ptr<Statement> const& else_stmt() const { return m_else; }

private:
    ElifStatements m_elifs {};
    std::shared_ptr<Statement> m_else {};
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> stmt)
        : Statement()
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

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::WhileStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}while {}\n{}", pad(indent), m_condition->to_string(indent), m_stmt->to_string(indent + 2));
    }
    [[nodiscard]] std::shared_ptr<Expression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

private:
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

class ForStatement : public Statement {
public:
    ForStatement(std::string variable, std::shared_ptr<Expression> range, std::shared_ptr<Statement> stmt)
        : Statement()
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
        auto iter_maybe = range->iterator();
        if (!iter_maybe.has_value())
            return { ExecutionResultCode::Error, make_obj<Exception>(ErrorCode::ObjectNotIterable, range) };
        auto iter = iter_maybe.value();
        for (auto value = iter->next(); value.has_value(); value = iter->next()) {
            assert(value.has_value());
            auto v = value.value();
            new_scope->assign(m_variable, v);
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

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::ForStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}for {} in {}\n{}", pad(indent), m_variable, m_range->to_string(indent), m_stmt->to_string(indent + 2));
    }
    [[nodiscard]] std::string const& variable() const { return m_variable; }
    [[nodiscard]] std::shared_ptr<Expression> const& range() const { return m_range; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

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
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::CaseStatement; }
};

typedef std::vector<std::shared_ptr<CaseStatement>> CaseStatements;

class DefaultCase : public Branch {
public:
    explicit DefaultCase(std::shared_ptr<Statement> const& stmt)
        : Branch("default", stmt)
    {
    }
    DefaultCase(std::shared_ptr<Expression> const&, std::shared_ptr<Statement> const& stmt)
        : Branch("default", stmt)
    {
    }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::DefaultCase; }
};

class SwitchStatement : public Statement {
public:
    explicit SwitchStatement(std::shared_ptr<Expression> switch_expression,
        CaseStatements case_statements, std::shared_ptr<DefaultCase> default_case)
        : Statement()
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

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::SwitchStatement; }
    [[nodiscard]] std::string to_string(int indent) const override
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
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_switch_expression; }
    [[nodiscard]] CaseStatements const& cases() const { return m_cases; }
    [[nodiscard]] std::shared_ptr<DefaultCase> const& default_case() const { return m_default; }

private:
    std::shared_ptr<Expression> m_switch_expression;
    CaseStatements m_cases {};
    std::shared_ptr<DefaultCase> m_default {};
};

}