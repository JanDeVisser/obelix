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
#include <core/Type.h>
#include <lexer/Token.h>
#include <obelix/Runtime.h>
#include <obelix/Scope.h>
#include <obelix/Symbol.h>
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
    S(SwitchStatement)               \
    S(StatementExecutionResult)

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

#define TO_OBJECT(expr)                                     \
    ({                                                      \
        auto _to_obj_temp_maybe = TRY((expr)->to_object()); \
        if (!_to_obj_temp_maybe.has_value())                \
            return std::optional<Obj> {};                   \
        _to_obj_temp_maybe.value();                         \
    })

class SyntaxNode {
public:
    SyntaxNode() = default;
    virtual ~SyntaxNode() = default;

    [[nodiscard]] virtual std::string to_string(int) const = 0;
    [[nodiscard]] virtual SyntaxNodeType node_type() const = 0;
};

using Nodes = std::vector<std::shared_ptr<SyntaxNode>>;

class Statement : public SyntaxNode {
public:
    Statement() = default;
};

class Expression : public SyntaxNode {
public:
    Expression()
        : SyntaxNode()
    {
    }

    [[nodiscard]] virtual ErrorOr<std::optional<Obj>> to_object() const = 0;
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
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return m_expression->to_object(); }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }
    [[nodiscard]] ObelixType type() const { return m_type; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{} : {}", m_expression->to_string(indent), ObelixType_name(m_type));
    }

private:
    std::shared_ptr<Expression> m_expression;
    ObelixType m_type { TypeUnknown };
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
};

typedef std::vector<std::shared_ptr<Statement>> Statements;

class Block : public Statement {
public:
    explicit Block(Statements statements)
        : Statement()
        , m_statements(move(statements))
    {
    }

    explicit Block(std::shared_ptr<Block> const&, Statements statements)
        : Statement()
        , m_statements(move(statements))
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
    [[nodiscard]] Statements const& statements() const { return m_statements; }

protected:
    Statements m_statements {};
};

class Module : public Block {
public:
    Module(std::vector<std::shared_ptr<Statement>> const& statements, std::string name)
        : Block(statements)
        , m_name(move(name))
    {
    }

    Module(std::shared_ptr<Module> const& original, std::vector<std::shared_ptr<Statement>> const& statements)
        : Block(statements)
        , m_name(original->name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Module; }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        auto ret = format("{}module {}\n", pad(indent), m_name);
        ret += Block::to_string(indent);
        return ret;
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

    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }

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
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }

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
    [[nodiscard]] Obj const& literal() const { return m_literal; }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return literal(); }

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

    ErrorOr<std::optional<Obj>> to_object() const override
    {
        Ptr<List> list = make_typed<List>();
        for (auto& e : elements()) {
            auto obj = TO_OBJECT(e);
            if (obj->is_exception())
                return ptr_cast<Exception>(obj)->error();
            list->push_back(obj);
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

    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }
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

class DictionaryLiteral : public Expression {
public:
    struct Entry {
        std::string name;
        std::shared_ptr<Expression> value;

        [[nodiscard]] bool operator==(Entry const& other) const
        {
            return name == other.name && value == other.value;
        }
    };
    using Entries = std::vector<Entry>;

    explicit DictionaryLiteral(Entries entries)
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

    ErrorOr<std::optional<Obj>> to_object() const override
    {
        auto dictionary = make_typed<Dictionary>();
        for (auto& e : m_entries) {
            auto result = TO_OBJECT(e.value);
            if (result->is_exception())
                return result;
            dictionary->put(e.name, result);
        }
        return to_obj(dictionary);
    }
    [[nodiscard]] Entries const& entries() const { return m_entries; }

private:
    Entries m_entries;
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

    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }
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
    ErrorOr<std::optional<Obj>> to_object() const override;

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

    ErrorOr<std::optional<Obj>> to_object() const override
    {
        auto operand = TO_OBJECT(m_operand);
        if (operand->is_exception())
            return operand;
        auto ret_maybe = operand->evaluate(m_operator.value());
        if (!ret_maybe.has_value())
            return make_obj<Exception>(ErrorCode::OperatorUnresolved, "Could not resolve operator");
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

    ErrorOr<std::optional<Obj>> to_object() const override
    {
        return std::optional<Obj> {};
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::FunctionCall; }
    [[nodiscard]] std::shared_ptr<Expression> const& function() const { return m_function; }
    [[nodiscard]] Expressions const& arguments() const { return m_arguments; }

private:
    std::shared_ptr<Expression> m_function;
    Expressions m_arguments;
};

class VariableDeclaration : public Statement {
public:
    explicit VariableDeclaration(std::string variable, ObelixType type, std::shared_ptr<Expression> expr = nullptr, bool constant = false)
        : Statement()
        , m_variable(move(variable), type)
        , m_const(constant)
        , m_expression(move(expr))
    {
    }

    explicit VariableDeclaration(Symbol identifier, std::shared_ptr<Expression> expr = nullptr, bool constant = false)
        : Statement()
        , m_variable(std::move(identifier))
        , m_const(constant)
        , m_expression(move(expr))
    {
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}{} {} : {} = {}", pad(indent), (m_const) ? "const" : "var", m_variable.identifier(), ObelixType_name(m_variable.type()),
            m_expression->to_string(indent));
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::VariableDeclaration; }
    [[nodiscard]] Symbol const& variable() const { return m_variable; }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<Expression> const& expression() const { return m_expression; }

private:
    Symbol m_variable;
    bool m_const { false };
    std::shared_ptr<Expression> m_expression;
};

class Return : public Statement {
public:
    explicit Return(std::shared_ptr<Expression> expression)
        : Statement()
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] std::string to_string(int indent) const override
    {
        return format("{}return {}", pad(indent), m_expression->to_string(indent));
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::Return; }
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

    [[nodiscard]] std::shared_ptr<Statement> const& if_stmt() const { return statement(); }
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

ErrorOr<std::shared_ptr<SyntaxNode>> to_literal(std::shared_ptr<Expression> const& expr);

}