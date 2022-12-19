/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/boundsyntax/Function.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundFunctionDecl -----------------------------------------------------

BoundFunctionDecl::BoundFunctionDecl(std::shared_ptr<SyntaxNode> const& decl, std::string module, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundStatement(decl->token())
    , m_module(std::move(module))
    , m_identifier(std::move(identifier))
    , m_parameters(std::move(parameters))
{
}

BoundFunctionDecl::BoundFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl)
    : BoundStatement(decl->token())
    , m_module(decl->module())
    , m_identifier(decl->identifier())
    , m_parameters(decl->parameters())
{
}

BoundFunctionDecl::BoundFunctionDecl(std::string module, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundStatement(Token {})
    , m_module(std::move(module))
    , m_identifier(std::move(identifier))
    , m_parameters(std::move(parameters))
{
}

std::string const& BoundFunctionDecl::module() const
{
    return m_module;
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

void BoundFunctionDecl::add_parameter(pBoundIdentifier parameter)
{
    m_parameters.push_back(std::move(parameter));
}

// -- BoundNativeFunctionDecl -----------------------------------------------

BoundNativeFunctionDecl::BoundNativeFunctionDecl(std::shared_ptr<NativeFunctionDecl> const& decl, std::string module, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(decl, std::move(module), std::move(identifier), std::move(parameters))
    , m_native_function_name(decl->native_function_name())
{
}

BoundNativeFunctionDecl::BoundNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const& decl, std::string module, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(decl, std::move(module), std::move(identifier), std::move(parameters))
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

BoundIntrinsicDecl::BoundIntrinsicDecl(std::shared_ptr<SyntaxNode> const& decl, std::string module, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(decl, std::move(module), std::move(identifier), std::move(parameters))
{
}

BoundIntrinsicDecl::BoundIntrinsicDecl(std::string module, std::shared_ptr<BoundIdentifier> identifier, BoundIdentifiers parameters)
    : BoundFunctionDecl(std::move(module), std::move(identifier), std::move(parameters))
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

// -- BoundMethodDecl -------------------------------------------------------

BoundMethodDecl::BoundMethodDecl(pSyntaxNode const& decl, pMethodDescription method)
    : BoundFunctionDecl(decl, "/", std::make_shared<BoundIdentifier>(decl->token(), method->name(), method->return_type()), {})
    , m_method(std::move(method))
{
    for (auto const& param : m_method->parameters()) {
        add_parameter(std::make_shared<BoundIdentifier>(Token {}, param.name, param.type));
    }
}

pMethodDescription const& BoundMethodDecl::method() const
{
    return m_method;
}

// -- FunctionBlock ---------------------------------------------------------

FunctionBlock::FunctionBlock(Token token, Statements statements, pBoundFunctionDecl declaration)
    : Block(std::move(token), std::move(statements))
    , m_declaration(std::move(declaration))
{
}

FunctionBlock::FunctionBlock(Token token, std::shared_ptr<Statement> statement, pBoundFunctionDecl declaration)
    : Block(std::move(token), Statements { std::move(statement) })
    , m_declaration(std::move(declaration))
{
}

pBoundFunctionDecl const& FunctionBlock::declaration() const
{
    return m_declaration;
}


// -- BoundFunctionDef ------------------------------------------------------

BoundFunctionDef::BoundFunctionDef(std::shared_ptr<FunctionDef> const& orig_def, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement)
    : BoundStatement(orig_def->token())
    , m_function_decl(std::move(func_decl))
    , m_statement(std::move(statement))
{
}

BoundFunctionDef::BoundFunctionDef(Token token, std::shared_ptr<BoundFunctionDecl> func_decl, std::shared_ptr<Statement> statement)
    : BoundStatement(std::move(token))
    , m_function_decl(std::move(func_decl))
    , m_statement(std::move(statement))
{
}

BoundFunctionDef::BoundFunctionDef(std::shared_ptr<BoundFunctionDef> const& orig_def, std::shared_ptr<Statement> statement)
    : BoundStatement(orig_def->token())
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

bool BoundFunctionDef::is_fully_bound() const
{
    return (m_statement == nullptr) || m_statement->is_fully_bound();
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

BoundFunctionCall::BoundFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, pObjectType type)
    : BoundExpression(call, type)
    , m_name(call->name())
    , m_arguments(call->arguments())
    , m_declaration(call->declaration())
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

BoundNativeFunctionCall::BoundNativeFunctionCall(std::shared_ptr<BoundNativeFunctionCall> const& call, pObjectType type)
    : BoundFunctionCall(call, type)
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

BoundIntrinsicCall::BoundIntrinsicCall(std::shared_ptr<BoundIntrinsicCall> const& call, pObjectType type)
    : BoundFunctionCall(call, type)
    , m_intrinsic(call->intrinsic())
{
}

IntrinsicType BoundIntrinsicCall::intrinsic() const
{
    return m_intrinsic;
}

// -- BoundMethodCall -------------------------------------------------------

BoundMethodCall::BoundMethodCall(Token token, pBoundMethodDecl const& declaration, pBoundExpression self, BoundExpressions arguments)
    : BoundFunctionCall(std::move(token), declaration, std::move(arguments))
    , m_self(std::move(self))
{
}

pBoundExpression const& BoundMethodCall::self() const
{
    return m_self;
}

// -- BoundFunction ---------------------------------------------------------

BoundFunction::BoundFunction(Token token, std::shared_ptr<BoundFunctionDecl> declaration)
    : BoundExpression(std::move(token), PrimitiveType::Function)
    , m_declaration(std::move(declaration))
{
}

std::string const& BoundFunction::name() const
{
    return m_declaration->name();
}

std::shared_ptr<BoundFunctionDecl> const& BoundFunction::declaration() const
{
    return m_declaration;
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

BoundLocalFunction::BoundLocalFunction(Token token, std::shared_ptr<BoundFunctionDecl> declaration)
    : BoundFunction(std::move(token), std::move(declaration))
{
}

// -- BoundImportedFunction -------------------------------------------------

BoundImportedFunction::BoundImportedFunction(Token token, std::shared_ptr<BoundModule> module, std::shared_ptr<BoundFunctionDecl> declaration)
    : BoundFunction(std::move(token), std::move(declaration))
    , m_module(std::move(module))
{
}

std::shared_ptr<BoundModule> const& BoundImportedFunction::module() const
{
    return m_module;
}

}
