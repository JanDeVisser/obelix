/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Syntax.h>
#include <obelix/SyntaxNodeType.h>

namespace Obelix {

class MaterializedFunctionParameter : public BoundIdentifier {
public:
    MaterializedFunctionParameter(std::shared_ptr<BoundIdentifier> const& param, int offset)
        : BoundIdentifier(param->token(), param->name(), param->type())
        , m_offset(offset)
    {
    }

    [[nodiscard]] int offset() const { return m_offset; }
    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionParameter; }
    [[nodiscard]] std::string attributes() const override
    {
        return format(R"({} offset="{}")", BoundIdentifier::attributes(), offset());
    }
    [[nodiscard]] std::string to_string() const override
    {
        if (m_offset > 0)
            return format("{} [{}]", BoundIdentifier::to_string(), m_offset);
        return BoundIdentifier::to_string();
    }

private:
    int m_offset;
};

using MaterializedFunctionParameters = std::vector<std::shared_ptr<MaterializedFunctionParameter>>;

class MaterializedFunctionDecl : public SyntaxNode {
public:
    explicit MaterializedFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl, MaterializedFunctionParameters parameters)
        : SyntaxNode(decl->token())
        , m_identifier(decl->identifier())
        , m_parameters(move(parameters))
    {
    }

    explicit MaterializedFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
        : SyntaxNode(decl->token())
        , m_identifier(decl->identifier())
    {
        for (auto& param : decl->parameters()) {
            auto materialized_param = std::make_shared<MaterializedFunctionParameter>(param, 0);
            m_parameters.push_back(materialized_param);
        }
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionDecl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return identifier()->type(); }
    [[nodiscard]] MaterializedFunctionParameters const& parameters() const { return m_parameters; }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" return_type="{}")", name(), type());
    }

    [[nodiscard]] std::string text_contents() const override
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += "\n";
            first = false;
            ret += format(R"(<Parameter {}/>)", param->attributes());
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        return format("func {}({}): {}", name(), parameters_to_string(), type());
    }

protected:
    [[nodiscard]] std::string parameters_to_string() const
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += ", ";
            first = false;
            ret += param->to_string();
        }
        return ret;
    }

private:
    std::shared_ptr<BoundIdentifier> m_identifier;
    MaterializedFunctionParameters m_parameters;
};

class MaterializedNativeFunctionDecl : public MaterializedFunctionDecl {
public:
    explicit MaterializedNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const& func_decl)
        : MaterializedFunctionDecl(func_decl)
        , m_native_function_name(func_decl->native_function_name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedNativeFunctionDecl; }
    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("{} -> \"{}\"", MaterializedFunctionDecl::to_string(), m_native_function_name);
    }

private:
    std::string m_native_function_name;
};

class MaterializedIntrinsicDecl : public MaterializedFunctionDecl {
public:
    explicit MaterializedIntrinsicDecl(std::shared_ptr<BoundIntrinsicDecl> const& decl)
        : MaterializedFunctionDecl(decl)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedIntrinsicDecl; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("intrinsic {}({}): {}", name(), parameters_to_string(), type());
    }
};

class MaterializedFunctionDef : public Statement {
public:
    MaterializedFunctionDef(std::shared_ptr<BoundFunctionDef> const& bound_def, std::shared_ptr<MaterializedFunctionDecl> func_decl, std::shared_ptr<Statement> statement, int stack_depth)
        : Statement(bound_def->token())
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
        , m_stack_depth(stack_depth)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionDef; }
    [[nodiscard]] std::shared_ptr<MaterializedFunctionDecl> const& declaration() const { return m_function_decl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_function_decl->identifier(); }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const { return identifier()->type(); }
    [[nodiscard]] MaterializedFunctionParameters const& parameters() const { return m_function_decl->parameters(); }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }
    [[nodiscard]] int stack_depth() const { return m_stack_depth; }
    [[nodiscard]] std::string to_string() const override
    {
        if (m_statement != nullptr)
            return format("{} [{}]\n{}", m_function_decl, m_stack_depth, m_statement);
        return m_function_decl->to_string();
    }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret = { m_function_decl };
        if (m_statement) {
            ret.push_back(m_statement);
        }
        return ret;
    }

protected:
    std::shared_ptr<MaterializedFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
    int m_stack_depth;
};

class MaterializedVariableDecl : public Statement {
public:
    explicit MaterializedVariableDecl(std::shared_ptr<BoundVariableDeclaration> const& var_decl, int offset, std::shared_ptr<BoundExpression> expression)
        : Statement(var_decl->token())
        , m_variable(var_decl->variable())
        , m_const(var_decl->is_const())
        , m_expression(move(expression))
        , m_offset(offset)
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
        std::string keyword = (m_const) ? "const" : "var";
        if (m_expression)
            return format("{} {}: {} [{}]", keyword, m_variable, m_expression, m_offset);
        return format("{} {} [{}]", keyword, m_variable, m_offset);
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedVariableDecl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& variable() const { return m_variable; }
    [[nodiscard]] std::string const& name() const { return variable()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const { return variable()->type(); }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }
    [[nodiscard]] int offset() const { return m_offset; }

private:
    std::shared_ptr<BoundIdentifier> m_variable;
    bool m_const { false };
    std::shared_ptr<BoundExpression> m_expression;
    int m_offset;
};

}
