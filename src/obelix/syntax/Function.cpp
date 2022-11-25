/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/Function.h>

namespace Obelix {

extern_logging_category(parser);

// -- FunctionDecl ----------------------------------------------------------

FunctionDecl::FunctionDecl(Token token, std::shared_ptr<Identifier> identifier, Identifiers parameters)
    : Statement(std::move(token))
    , m_identifier(std::move(identifier))
    , m_parameters(std::move(parameters))
{
}

std::shared_ptr<Identifier> const& FunctionDecl::identifier() const
{
    return m_identifier;
}

std::string const& FunctionDecl::name() const
{
    return identifier()->name();
}

std::shared_ptr<ExpressionType> FunctionDecl::type() const
{
    return identifier()->type();
}

std::string FunctionDecl::type_name() const
{
    return (type()) ? type()->type_name() : "[Unresolved]";
}

Identifiers const& FunctionDecl::parameters() const
{
    return m_parameters;
}

ExpressionTypes FunctionDecl::parameter_types() const
{
    ExpressionTypes ret;
    for (auto& parameter : m_parameters) {
        ret.push_back(parameter->type());
    }
    return ret;
}

std::string FunctionDecl::attributes() const
{
    return format(R"(name="{}" return_type="{}")", name(), type_name());
}

Nodes FunctionDecl::children() const
{
    Nodes ret;
    for (auto& param : m_parameters) {
        ret.push_back(param);
    }
    return ret;
}

std::string FunctionDecl::to_string() const
{
    return format("func {}({}): {}", name(), parameters_to_string(), type());
}

std::string FunctionDecl::parameters_to_string() const
{
    std::string ret;
    bool first = true;
    for (auto& param : m_parameters) {
        if (!first)
            ret += ", ";
        first = false;
        ret += param->name() + ": " + param->type()->to_string();
    }
    return ret;
}

// -- NativeFunctionDecl ----------------------------------------------------

NativeFunctionDecl::NativeFunctionDecl(Token token, std::shared_ptr<Identifier> identifier, Identifiers parameters, std::string native_function)
    : FunctionDecl(std::move(token), std::move(identifier), std::move(parameters))
    , m_native_function_name(std::move(native_function))
{
}

std::string const& NativeFunctionDecl::native_function_name() const
{
    return m_native_function_name;
}

std::string NativeFunctionDecl::attributes() const
{
    return format("{} native_function=\"{}\"", FunctionDecl::attributes(), native_function_name());
}

std::string NativeFunctionDecl::to_string() const
{
    return format("{} -> \"{}\"", FunctionDecl::to_string(), native_function_name());
}

// -- IntrinsicDecl ---------------------------------------------------------

IntrinsicDecl::IntrinsicDecl(Token token, std::shared_ptr<Identifier> identifier, Identifiers parameters)
    : FunctionDecl(std::move(token), std::move(identifier), std::move(parameters))
{
}

std::string IntrinsicDecl::to_string() const
{
    return format("intrinsic {}({}): {}", name(), parameters_to_string(), type());
}

// -- FunctionDef -----------------------------------------------------------

FunctionDef::FunctionDef(Token token, std::shared_ptr<FunctionDecl> func_decl, std::shared_ptr<Statement> statement)
    : Statement(std::move(token))
    , m_function_decl(std::move(func_decl))
    , m_statement(std::move(statement))
{
}

std::shared_ptr<FunctionDecl> const& FunctionDef::declaration() const
{
    return m_function_decl;
}

std::shared_ptr<Identifier> const& FunctionDef::identifier() const
{
    return m_function_decl->identifier();
}

std::string const& FunctionDef::name() const
{
    return identifier()->name();
}

std::shared_ptr<ExpressionType> FunctionDef::type() const
{
    return identifier()->type();
}

Identifiers const& FunctionDef::parameters() const
{
    return m_function_decl->parameters();
}

std::shared_ptr<Statement> const& FunctionDef::statement() const
{
    return m_statement;
}

Nodes FunctionDef::children() const
{
    Nodes ret;
    ret.push_back(m_function_decl);
    if (m_statement) {
        ret.push_back(m_statement);
    }
    return ret;
}

std::string FunctionDef::to_string() const
{
    auto ret = m_function_decl->to_string();
    if (m_statement) {
        ret += " ";
        ret += m_statement->to_string();
    }
    return ret;
}

}
