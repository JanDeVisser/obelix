/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/boundsyntax/Function.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundFunctionDecl -----------------------------------------------------

BoundFunctionDecl::BoundFunctionDecl(std::shared_ptr<SyntaxNode> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : Statement(decl->token())
    , m_identifier(std::move(identifier))
    , m_parameters(std::move(parameters))
{
}

BoundFunctionDecl::BoundFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
    : Statement(decl->token())
    , m_identifier(decl->identifier())
    , m_parameters(decl->parameters())
{
}

BoundFunctionDecl::BoundFunctionDecl(std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : Statement(Token {})
    , m_identifier(std::move(identifier))
    , m_parameters(std::move(parameters))
{
}

std::shared_ptr<BoundIdentifier> const& BoundFunctionDecl::identifier() const
{
    return m_identifier;
}

std::string const& BoundFunctionDecl::name() const
{
    return identifier()->name();
}

std::shared_ptr<ObjectType> BoundFunctionDecl::type() const
{
    return identifier()->type();
}

std::string BoundFunctionDecl::type_name() const
{
    return type()->name();
}

BoundIdentifiers const& BoundFunctionDecl::parameters() const
{
    return m_parameters;
}

ObjectTypes BoundFunctionDecl::parameter_types() const
{
    ObjectTypes ret;
    for (auto& parameter : m_parameters) {
        ret.push_back(parameter->type());
    }
    return ret;
}

std::string BoundFunctionDecl::attributes() const
{
    return format(R"(name="{}" return_type="{}")", name(), type_name());
}

Nodes BoundFunctionDecl::children() const
{
    Nodes ret;
    for (auto& param : m_parameters) {
        ret.push_back(param);
    }
    return ret;
}

std::string BoundFunctionDecl::to_string() const
{
    return format("func {}({}): {}", name(), parameters_to_string(), type());
}

std::string BoundFunctionDecl::parameters_to_string() const
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

// -- BoundNativeFunctionDecl -----------------------------------------------

BoundNativeFunctionDecl::BoundNativeFunctionDecl(std::shared_ptr<NativeFunctionDecl> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(decl, std::move(identifier), std::move(parameters))
    , m_native_function_name(decl->native_function_name())
{
}

BoundNativeFunctionDecl::BoundNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(decl, std::move(identifier), std::move(parameters))
    , m_native_function_name(decl->native_function_name())
{
}

std::string const& BoundNativeFunctionDecl::native_function_name() const
{
    return m_native_function_name;
}

std::string BoundNativeFunctionDecl::attributes() const
{
    return format("{} native_function=\"{}\"", BoundFunctionDecl::attributes(), native_function_name());
}

std::string BoundNativeFunctionDecl::to_string() const
{
    return format("{} -> \"{}\"", BoundFunctionDecl::to_string(), native_function_name());
}

// -- BoundIntrinsicDecl ----------------------------------------------------

BoundIntrinsicDecl::BoundIntrinsicDecl(std::shared_ptr<SyntaxNode> const& decl, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(decl, std::move(identifier), std::move(parameters))
{
}

BoundIntrinsicDecl::BoundIntrinsicDecl(std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(std::move(identifier), std::move(parameters))
{
}

BoundIntrinsicDecl::BoundIntrinsicDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
    : BoundFunctionDecl(decl)
{
}

std::string BoundIntrinsicDecl::to_string() const
{
    return format("intrinsic {}({}): {}", name(), parameters_to_string(), type());
}

// -- BoundFunctionDef ------------------------------------------------------

BoundFunctionDef::BoundFunctionDef(std::shared_ptr<FunctionDef> const& orig_def, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement)
    : Statement(orig_def->token())
    , m_function_decl(std::move(func_decl))
    , m_statement(std::move(statement))
{
}

BoundFunctionDef::BoundFunctionDef(Token token, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement)
    : Statement(std::move(token))
    , m_function_decl(std::move(func_decl))
    , m_statement(std::move(statement))
{
}

BoundFunctionDef::BoundFunctionDef(std::shared_ptr<BoundFunctionDef> const& orig_def, std::shared_ptr<Statement> statement)
    : Statement(orig_def->token())
    , m_function_decl(orig_def->declaration())
    , m_statement(std::move(statement))
{
}

std::shared_ptr<BoundFunctionDecl> const& BoundFunctionDef::declaration() const
{
    return m_function_decl;
}

std::shared_ptr<BoundIdentifier> const& BoundFunctionDef::identifier() const
{
    return m_function_decl->identifier();
}

std::string const& BoundFunctionDef::name() const
{
    return identifier()->name();
}

std::shared_ptr<ObjectType> BoundFunctionDef::type() const
{
    return identifier()->type();
}

BoundIdentifiers const& BoundFunctionDef::parameters() const
{
    return m_function_decl->parameters();
}

std::shared_ptr<Statement> const& BoundFunctionDef::statement() const
{
    return m_statement;
}

Nodes BoundFunctionDef::children() const
{
    Nodes ret;
    ret.push_back(m_function_decl);
    if (m_statement) {
        ret.push_back(m_statement);
    }
    return ret;
}

std::string BoundFunctionDef::to_string() const
{
    auto ret = m_function_decl->to_string();
    if (m_statement) {
        ret += "\n";
        ret += m_statement->to_string();
    }
    return ret;
}

// -- BoundFunctionCall -----------------------------------------------------

BoundFunctionCall::BoundFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundFunctionDecl> const& decl)
    : BoundExpression(call)
    , m_name(call->name())
    , m_arguments(std::move(arguments))
    , m_declaration((decl) ? decl : call->declaration())
{
}

BoundFunctionCall::BoundFunctionCall(Token token, std::shared_ptr<BoundFunctionDecl> const& decl, BoundExpressions arguments)
    : BoundExpression(std::move(token), decl->type())
    , m_name(decl->name())
    , m_arguments(std::move(arguments))
    , m_declaration(decl)
{
}

std::string BoundFunctionCall::attributes() const
{
    return format(R"(name="{}" type="{}")", name(), type());
}

Nodes BoundFunctionCall::children() const
{
    Nodes ret;
    for (auto& arg : m_arguments) {
        ret.push_back(arg);
    }
    return ret;
}

std::string BoundFunctionCall::to_string() const
{
    Strings args;
    for (auto& arg : m_arguments) {
        args.push_back(arg->to_string());
    }
    return format("{}({}): {}", name(), join(args, ","), type());
}

std::string const& BoundFunctionCall::name() const
{
    return m_name;
}

BoundExpressions const& BoundFunctionCall::arguments() const
{
    return m_arguments;
}

std::shared_ptr<BoundFunctionDecl> const& BoundFunctionCall::declaration() const
{
    return m_declaration;
}

ObjectTypes BoundFunctionCall::argument_types() const
{
    ObjectTypes ret;
    for (auto& arg : arguments()) {
        ret.push_back(arg->type());
    }
    return ret;
}

// -- BoundNativeFunctionCall -----------------------------------------------

BoundNativeFunctionCall::BoundNativeFunctionCall(Token token, std::shared_ptr<BoundNativeFunctionDecl> const& declaration, BoundExpressions arguments)
    : BoundFunctionCall(std::move(token), declaration, std::move(arguments))
{
}

BoundNativeFunctionCall::BoundNativeFunctionCall(std::shared_ptr<BoundNativeFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundFunctionDecl> const& decl)
    : BoundFunctionCall(call, std::move(arguments), decl)
{
}

// -- BoundIntrinsicCall ----------------------------------------------------

BoundIntrinsicCall::BoundIntrinsicCall(Token token, std::shared_ptr<BoundIntrinsicDecl> const& declaration, BoundExpressions arguments, IntrinsicType intrinsic)
    : BoundFunctionCall(std::move(token), declaration, std::move(arguments))
    , m_intrinsic(intrinsic)
{
}

BoundIntrinsicCall::BoundIntrinsicCall(std::shared_ptr<BoundIntrinsicCall> const& call, BoundExpressions arguments, std::shared_ptr<BoundIntrinsicDecl> const& decl)
    : BoundFunctionCall(call, std::move(arguments), decl)
    , m_intrinsic(call->intrinsic())
{
}

IntrinsicType BoundIntrinsicCall::intrinsic() const
{
    return m_intrinsic;
}

// -- BoundFunction ---------------------------------------------------------

BoundFunction::BoundFunction(Token token, std::string name)
    : BoundExpression(std::move(token), PrimitiveType::Function)
    , m_name(std::move(name))
{
}

std::string const& BoundFunction::name() const
{
    return m_name;
}

std::string BoundFunction::attributes() const
{
    return format(R"(name="{}")", name());
}

std::string BoundFunction::to_string() const
{
    return format("func {}", name());
}

// -- BoundLocalFunction ----------------------------------------------------

BoundLocalFunction::BoundLocalFunction(Token token, std::string name)
    : BoundFunction(std::move(token), std::move(name))
{
}

// -- BoundImportedFunction -------------------------------------------------

BoundImportedFunction::BoundImportedFunction(Token token, std::shared_ptr<BoundModule> module, std::string name)
    : BoundFunction(std::move(token), std::move(name))
    , m_module(std::move(module))
{
}

std::shared_ptr<BoundModule> const& BoundImportedFunction::module() const
{
    return m_module;
}

}
