/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/Syntax.h>

namespace Obelix {

class BoundExpression : public SyntaxNode {
public:
    BoundExpression()
        : SyntaxNode()
    {
    }

    explicit BoundExpression(Token token, std::shared_ptr<ObjectType> type)
        : SyntaxNode(std::move(token))
        , m_type(std::move(type))
    {
    }

    explicit BoundExpression(std::shared_ptr<Expression> const& expr, std::shared_ptr<ObjectType> type)
        : SyntaxNode(expr->token())
        , m_type(move(type))
    {
    }

    explicit BoundExpression(std::shared_ptr<BoundExpression> const& expr)
        : SyntaxNode(expr->token())
        , m_type(expr->type())
    {
    }

    explicit BoundExpression(Token token, ObelixType type)
        : SyntaxNode(std::move(token))
        , m_type(ObjectType::get(type))
    {
    }

    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const { return m_type; }
    [[nodiscard]] std::string const& type_name() const { return m_type->name(); }
    [[nodiscard]] virtual ErrorOr<std::optional<Obj>> to_object() const = 0;
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{}")", type()->name()); }

private:
    std::shared_ptr<ObjectType> m_type { nullptr };
};

using BoundExpressions = std::vector<std::shared_ptr<BoundExpression>>;

class BoundIdentifier : public BoundExpression {
public:
    explicit BoundIdentifier(std::shared_ptr<Identifier> const& identifier, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(identifier, move(type))
        , m_identifier(identifier->name())
    {
    }

    BoundIdentifier(Token token, std::string identifier, std::shared_ptr<ObjectType> type)
        : BoundExpression(std::move(token), move(type))
        , m_identifier(move(identifier))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIdentifier; }
    [[nodiscard]] std::string const& name() const { return m_identifier; }
    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}" type="{}")", name(), type()); }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return std::optional<Obj> {}; }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", name(), type()); }

private:
    std::string m_identifier;
};

using BoundIdentifiers = std::vector<std::shared_ptr<BoundIdentifier>>;

class BoundFunctionDecl : public SyntaxNode {
public:
    explicit BoundFunctionDecl(std::shared_ptr<SyntaxNode> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : SyntaxNode(decl->token())
        , m_identifier(move(identifier))
        , m_parameters(move(parameters))
    {
    }

    explicit BoundFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
        : SyntaxNode(decl->token())
        , m_identifier(decl->identifier())
        , m_parameters(decl->parameters())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundFunctionDecl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return identifier()->type(); }
    [[nodiscard]] std::string type_name() const { return type()->name(); }
    [[nodiscard]] BoundIdentifiers const& parameters() const { return m_parameters; }
    [[nodiscard]] ObjectTypes parameter_types() const
    {
        ObjectTypes ret;
        for (auto& parameter : m_parameters) {
            ret.push_back(parameter->type());
        }
        return ret;
    }

    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}" return_type="{}")", name(), type_name()); }

    [[nodiscard]] std::string text_contents() const override
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += "\n";
            first = false;
            ret += format(R"(<Parameter name="{}" type="{}"/>)", param->name(), param->type_name());
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        return format("func {}({}): {}", name(), parameters_to_string(), type());
    }

protected:
    explicit BoundFunctionDecl(std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : SyntaxNode(Token {})
        , m_identifier(move(identifier))
        , m_parameters(move(parameters))
    {
    }

private:
    [[nodiscard]] std::string parameters_to_string() const
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += ", ";
            first = false;
            ret += param->name() + ": " + param->type_name();
        }
        return ret;
    }

    std::shared_ptr<BoundIdentifier> m_identifier;
    BoundIdentifiers m_parameters;
};

class BoundNativeFunctionDecl : public BoundFunctionDecl {
public:
    explicit BoundNativeFunctionDecl(std::shared_ptr<NativeFunctionDecl> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(decl, move(identifier), move(parameters))
        , m_native_function_name(decl->native_function_name())
    {
    }

    explicit BoundNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(decl, move(identifier), move(parameters))
        , m_native_function_name(decl->native_function_name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundNativeFunctionDecl; }
    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }
    [[nodiscard]] std::string attributes() const override { return format("{} native_function=\"{}\"", BoundFunctionDecl::attributes(), native_function_name()); }
    [[nodiscard]] std::string to_string() const override { return format("{} -> \"{}\"", BoundFunctionDecl::to_string(), native_function_name()); }

private:
    std::string m_native_function_name;
};

class BoundIntrinsicDecl : public BoundFunctionDecl {
public:
    BoundIntrinsicDecl(std::shared_ptr<SyntaxNode> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(decl, move(identifier), move(parameters))
    {
    }

    BoundIntrinsicDecl(std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
        : BoundFunctionDecl(move(identifier), move(parameters))
    {
    }

    explicit BoundIntrinsicDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
        : BoundFunctionDecl(decl)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIntrinsicDecl; }
    [[nodiscard]] std::string to_string() const override { return format("{} intrinsic", BoundFunctionDecl::to_string()); }
};

class BoundFunctionDef : public Statement {
public:
    explicit BoundFunctionDef(std::shared_ptr<FunctionDef> const& orig_def, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement = nullptr)
        : Statement(orig_def->token())
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
    {
    }

    explicit BoundFunctionDef(Token token, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement = nullptr)
        : Statement(std::move(token))
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
    {
    }

    explicit BoundFunctionDef(std::shared_ptr<BoundFunctionDef> const& orig_def, std::shared_ptr<Statement> statement = nullptr)
        : Statement(orig_def->token())
        , m_function_decl(orig_def->declaration())
        , m_statement(move(statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundFunctionDef; }
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> const& declaration() const { return m_function_decl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_function_decl->identifier(); }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return identifier()->type(); }
    [[nodiscard]] BoundIdentifiers const& parameters() const { return m_function_decl->parameters(); }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }
    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        ret.push_back(m_function_decl);
        if (m_statement) {
            ret.push_back(m_statement);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = m_function_decl->to_string();
        if (m_statement) {
            ret += "\n";
            ret += m_statement->to_string();
        }
        return ret;
    }

protected:
    std::shared_ptr<BoundFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
};

class BoundLiteral : public BoundExpression {
public:
    explicit BoundLiteral(std::shared_ptr<Literal> const& literal, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(literal, move(type))
        , m_literal(literal->literal())
    {
    }

    explicit BoundLiteral(Token const& token)
        : BoundExpression(token, token.to_object()->type())
        , m_literal(token.to_object())
    {
    }

    explicit BoundLiteral(Token token, Obj value)
        : BoundExpression(std::move(token), value.type())
        , m_literal(std::move(value))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundLiteral; }
    [[nodiscard]] std::string text_contents() const override { return m_literal->to_string(); }
    [[nodiscard]] Obj const& literal() const { return m_literal; }
    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override { return literal(); }
    [[nodiscard]] std::string to_string() const override { return format("{}: {}", literal()->to_string(), type()); }

private:
    Obj m_literal;
};

class BoundBinaryExpression : public BoundExpression {
public:
    BoundBinaryExpression(std::shared_ptr<BinaryExpression> const& expr, std::shared_ptr<BoundExpression> lhs, BinaryOperator op, std::shared_ptr<BoundExpression> rhs, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(expr, move(type))
        , m_lhs(std::move(lhs))
        , m_operator(op)
        , m_rhs(std::move(rhs))
    {
    }

    BoundBinaryExpression(Token token, std::shared_ptr<BoundExpression> lhs, BinaryOperator op, std::shared_ptr<BoundExpression> rhs, std::shared_ptr<ObjectType> type)
        : BoundExpression(std::move(token), move(type))
        , m_lhs(std::move(lhs))
        , m_operator(op)
        , m_rhs(std::move(rhs))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundBinaryExpression; }
    [[nodiscard]] std::string attributes() const override { return format(R"(operator="{}" type="{}")", m_operator, type()); }
    [[nodiscard]] Nodes children() const override { return { m_lhs, m_rhs }; }
    ErrorOr<std::optional<Obj>> to_object() const override;
    [[nodiscard]] std::string to_string() const override { return format("{} {} {}", lhs(), op(), rhs()); }

    [[nodiscard]] std::shared_ptr<BoundExpression> const& lhs() const { return m_lhs; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& rhs() const { return m_rhs; }
    [[nodiscard]] BinaryOperator op() const { return m_operator; }

private:
    std::shared_ptr<BoundExpression> m_lhs;
    BinaryOperator m_operator;
    std::shared_ptr<BoundExpression> m_rhs;
};

class BoundUnaryExpression : public BoundExpression {
public:
    BoundUnaryExpression(std::shared_ptr<UnaryExpression> const& expr, std::shared_ptr<BoundExpression> operand, UnaryOperator op, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(expr, move(type))
        , m_operator(op)
        , m_operand(std::move(operand))
    {
    }

    BoundUnaryExpression(Token token, std::shared_ptr<BoundExpression> operand, UnaryOperator op, std::shared_ptr<ObjectType> type = nullptr)
        : BoundExpression(std::move(token), move(type))
        , m_operator(op)
        , m_operand(std::move(operand))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundUnaryExpression; }
    [[nodiscard]] std::string attributes() const override { return format(R"(operator="{}" type="{}")", m_operator, type()); }
    [[nodiscard]] Nodes children() const override { return { m_operand }; }
    [[nodiscard]] std::string to_string() const override { return format("{} {}", op(), operand()->to_string()); }
    [[nodiscard]] UnaryOperator op() const { return m_operator; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& operand() const { return m_operand; }

    ErrorOr<std::optional<Obj>> to_object() const override;

private:
    UnaryOperator m_operator;
    std::shared_ptr<BoundExpression> m_operand;
};

class BoundFunctionCall : public BoundExpression {
public:
    BoundFunctionCall(std::shared_ptr<FunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundFunctionDecl> decl)
        : BoundExpression(call, decl->type())
        , m_name(call->name())
        , m_arguments(move(arguments))
        , m_declaration(move(decl))
    {
    }

    BoundFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundFunctionDecl> const& decl = nullptr)
        : BoundExpression(call)
        , m_name(call->name())
        , m_arguments(move(arguments))
        , m_declaration((decl) ? decl : call->declaration())
    {
    }

    BoundFunctionCall(Token token, std::shared_ptr<BoundFunctionDecl> const& decl, BoundExpressions arguments)
        : BoundExpression(std::move(token), decl->type())
        , m_name(decl->name())
        , m_arguments(move(arguments))
        , m_declaration(decl)
    {
    }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" type="{}")", name(), type());
    }

    [[nodiscard]] std::string text_contents() const override
    {
        std::string ret = "<Arguments";
        if (m_arguments.empty())
            return ret + "/>";
        ret += ">";
        for (auto& arg : m_arguments) {
            ret += '\n';
            ret += arg->to_xml(2);
        }
        return ret + "\n</Arguments>";
    }

    [[nodiscard]] std::string to_string() const override
    {
        Strings args;
        for (auto& arg : m_arguments) {
            args.push_back(arg->to_string());
        }
        return format("{}({}): {}", name(), join(args, ","), type());
    }

    [[nodiscard]] ErrorOr<std::optional<Obj>> to_object() const override
    {
        return std::optional<Obj> {};
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundFunctionCall; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] BoundExpressions const& arguments() const { return m_arguments; }
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> const& declaration() const { return m_declaration; }

    [[nodiscard]] ObjectTypes argument_types() const
    {
        ObjectTypes ret;
        for (auto& arg : arguments()) {
            ret.push_back(arg->type());
        }
        return ret;
    }

private:
    std::string m_name;
    BoundExpressions m_arguments;
    std::shared_ptr<BoundFunctionDecl> m_declaration;
};

class BoundNativeFunctionCall : public BoundFunctionCall {
public:
    explicit BoundNativeFunctionCall(std::shared_ptr<FunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundNativeFunctionDecl> decl)
        : BoundFunctionCall(call, move(arguments), move(decl))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundNativeFunctionCall; }
};

class BoundIntrinsicCall : public BoundFunctionCall {
public:
    explicit BoundIntrinsicCall(std::shared_ptr<FunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundIntrinsicDecl> decl)
        : BoundFunctionCall(call, move(arguments), move(decl))
    {
    }

    explicit BoundIntrinsicCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments)
        : BoundFunctionCall(call, move(arguments), std::make_shared<BoundIntrinsicDecl>(call->declaration()))
    {
    }

    explicit BoundIntrinsicCall(Token token, std::shared_ptr<BoundIntrinsicDecl> const& decl, BoundExpressions arguments)
        : BoundFunctionCall(std::move(token), decl, move(arguments))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIntrinsicCall; }
};

class BoundExpressionStatement : public Statement {
public:
    BoundExpressionStatement(std::shared_ptr<ExpressionStatement> const& stmt, std::shared_ptr<BoundExpression> expression)
        : Statement(stmt->token())
        , m_expression(move(expression))
    {
    }

    BoundExpressionStatement(Token token, std::shared_ptr<BoundExpression> expression)
        : Statement(std::move(token))
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundExpressionStatement; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }
    [[nodiscard]] Nodes children() const override { return { m_expression }; }
    [[nodiscard]] std::string to_string() const override { return m_expression->to_string(); }

private:
    std::shared_ptr<BoundExpression> m_expression;
};

class BoundVariableDeclaration : public Statement {
public:
    explicit BoundVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
        : Statement(decl->token())
        , m_variable(move(variable))
        , m_const(decl->is_const())
        , m_expression(move(expr))
    {
    }

    explicit BoundVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
        : Statement(std::move(token))
        , m_variable(move(variable))
        , m_const(is_const)
        , m_expression(move(expr))
    {
    }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" type="{}" is_const="{}")", name(), type(), is_const());
    }

    [[nodiscard]] Nodes children() const override
    {
        if (m_expression)
            return { m_expression };
        return {};
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = format("{} {}: {}", (is_const()) ? "const" : "var", name(), expression()->type());
        if (m_expression)
            ret += format(" = {}", m_expression->to_string());
        return ret;
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundVariableDeclaration; }
    [[nodiscard]] std::string const& name() const { return m_variable->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return m_variable->type(); }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& variable() const { return m_variable; }

private:
    std::shared_ptr<BoundIdentifier> m_variable;
    bool m_const { false };
    std::shared_ptr<BoundExpression> m_expression;
};

class BoundReturn : public Statement {
public:
    BoundReturn(std::shared_ptr<SyntaxNode> const& ret, std::shared_ptr<BoundExpression> expression)
        : Statement(ret->token())
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] Nodes children() const override
    {
        if (m_expression)
            return { m_expression };
        return {};
    }

    [[nodiscard]] std::string to_string() const override
    {
        std::string ret = "return";
        if (m_expression)
            ret = format("{} {}", ret, m_expression->to_string());
        return ret;
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundReturn; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }

private:
    std::shared_ptr<BoundExpression> m_expression;
};

class BoundBranch : public Statement {
public:
    BoundBranch(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> bound_statement)
        : Statement(node->token())
        , m_condition(move(condition))
        , m_statement(move(bound_statement))
    {
    }

    BoundBranch(Token token, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> bound_statement)
        : Statement(std::move(token))
        , m_condition(move(condition))
        , m_statement(move(bound_statement))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundBranch; }
    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        if (m_condition)
            ret.push_back(m_condition);
        ret.push_back(m_statement);
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        if (m_condition)
            return format("if ({})\n{}", m_condition->to_string(), m_statement->to_string());
        return format("else\n{}", m_statement->to_string());
    }

    [[nodiscard]] std::shared_ptr<BoundExpression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }

private:
    std::shared_ptr<BoundExpression> m_condition { nullptr };
    std::shared_ptr<Statement> m_statement;
};

typedef std::vector<std::shared_ptr<BoundBranch>> BoundBranches;

class BoundIfStatement : public Statement {
public:
    BoundIfStatement(std::shared_ptr<IfStatement> const& if_stmt, BoundBranches branches, std::shared_ptr<Statement> else_stmt)
        : Statement(if_stmt->token())
        , m_branches(move(branches))
        , m_else(move(else_stmt))
    {
    }

    BoundIfStatement(Token token, BoundBranches branches, std::shared_ptr<Statement> else_stmt = nullptr)
        : Statement(std::move(token))
        , m_branches(move(branches))
        , m_else(move(else_stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundIfStatement; }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& branch : m_branches) {
            ret.push_back(branch);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        std::string ret;
        bool first;
        for (auto& branch : m_branches) {
            if (first)
                ret = branch->to_string();
            else
                ret += "el" + branch->to_string();
            first = false;
        }
        if (m_else)
            ret += "else\n" + m_else->to_string();
        return ret;
    }

    [[nodiscard]] BoundBranches const& branches() const { return m_branches; }

private:
    BoundBranches m_branches {};
    std::shared_ptr<Statement> m_else {};
};

class BoundWhileStatement : public Statement {
public:
    BoundWhileStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> stmt)
        : Statement(node->token())
        , m_condition(move(condition))
        , m_stmt(move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundWhileStatement; }
    [[nodiscard]] Nodes children() const override { return { m_condition, m_stmt }; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& condition() const { return m_condition; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

    [[nodiscard]] std::string to_string() const override
    {
        return format("while ({})\n{}", m_condition->to_string(), m_stmt->to_string());
    }

private:
    std::shared_ptr<BoundExpression> m_condition;
    std::shared_ptr<Statement> m_stmt;
};

class BoundForStatement : public Statement {
public:
    BoundForStatement(std::shared_ptr<ForStatement> const& orig_for_stmt, std::shared_ptr<BoundExpression> range, std::shared_ptr<Statement> stmt)
        : Statement(orig_for_stmt->token())
        , m_variable(orig_for_stmt->variable())
        , m_range(move(range))
        , m_stmt(move(stmt))
    {
    }

    BoundForStatement(std::shared_ptr<BoundForStatement> const& orig_for_stmt, std::shared_ptr<BoundExpression> range, std::shared_ptr<Statement> stmt)
        : Statement(orig_for_stmt->token())
        , m_variable(orig_for_stmt->variable())
        , m_range(move(range))
        , m_stmt(move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundForStatement; }
    [[nodiscard]] std::string attributes() const override { return format(R"(variable="{}")", m_variable); }
    [[nodiscard]] Nodes children() const override { return { m_range, m_stmt }; }
    [[nodiscard]] std::string const& variable() const { return m_variable; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& range() const { return m_range; }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_stmt; }

    [[nodiscard]] std::string to_string() const override
    {
        return format("for ({} in {})\n{}", m_variable, m_range->to_string(), m_stmt->to_string());
    }

private:
    std::string m_variable {};
    std::shared_ptr<BoundExpression> m_range;
    std::shared_ptr<Statement> m_stmt;
};

class BoundCaseStatement : public BoundBranch {
public:
    BoundCaseStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> case_expression, std::shared_ptr<Statement> stmt)
        : BoundBranch(node, move(case_expression), move(stmt))
    {
    }

    BoundCaseStatement(Token token, std::shared_ptr<BoundExpression> case_expression, std::shared_ptr<Statement> stmt)
        : BoundBranch(std::move(token), move(case_expression), move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundCaseStatement; }
};

typedef std::vector<std::shared_ptr<BoundCaseStatement>> BoundCaseStatements;

class BoundDefaultCase : public BoundBranch {
public:
    BoundDefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<Statement> const& stmt)
        : BoundBranch(node, nullptr, move(stmt))
    {
    }

    BoundDefaultCase(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> const&, std::shared_ptr<Statement> const& stmt)
        : BoundBranch(node, nullptr, move(stmt))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundDefaultCase; }
};

class BoundSwitchStatement : public Statement {
public:
    BoundSwitchStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> switch_expr, BoundCaseStatements cases, std::shared_ptr<BoundDefaultCase> default_case)
        : Statement(node->token())
        , m_switch_expression(move(switch_expr))
        , m_cases(move(cases))
        , m_default(move(default_case))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::SwitchStatement; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_switch_expression; }
    [[nodiscard]] BoundCaseStatements const& cases() const { return m_cases; }
    [[nodiscard]] std::shared_ptr<BoundDefaultCase> const& default_case() const { return m_default; }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        ret.push_back(m_switch_expression);
        for (auto& case_stmt : m_cases) {
            ret.push_back(case_stmt);
        }
        if (m_default)
            ret.push_back(m_default);
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto ret = format("switch ({}) {{\n", expression()->to_string());
        for (auto& case_stmt : m_cases) {
            ret += '\n';
            ret += case_stmt->to_string();
        }
        if (m_default) {
            ret += '\n';
            ret += m_default->to_string();
        }
        return ret + "}";
    }

private:
    std::shared_ptr<BoundExpression> m_switch_expression;
    BoundCaseStatements m_cases {};
    std::shared_ptr<BoundDefaultCase> m_default {};
};

class BoundAssignment : public BoundExpression {
public:
    BoundAssignment(Token token, std::shared_ptr<BoundIdentifier> identifier, std::shared_ptr<BoundExpression> expression)
        : BoundExpression(std::move(token), identifier->type())
        , m_identifier(move(identifier))
        , m_expression(move(expression))
    {
        assert(m_identifier->type()->type() == m_expression->type()->type());
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::BoundAssignment; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    ErrorOr<std::optional<Obj>> to_object() const override { return identifier()->to_object(); }
    [[nodiscard]] Nodes children() const override { return { identifier(), expression() }; }
    [[nodiscard]] std::string to_string() const override { return format("{} = {}", identifier()->to_string(), expression()->to_string()); }

private:
    std::shared_ptr<BoundIdentifier> m_identifier;
    std::shared_ptr<BoundExpression> m_expression;
};

}
