/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/arm64/MaterializedSyntaxNode.h>
#include <obelix/arm64/VariableAddress.h>

namespace Obelix {

// -- MaterializedFunctionParameter -----------------------------------------

MaterializedFunctionParameter::MaterializedFunctionParameter(std::shared_ptr<BoundIdentifier> const& param, std::shared_ptr<VariableAddress> address, ParameterPassingMethod method, int where)
    : BoundIdentifier(param->token(), param->name(), param->type())
    , MaterializedDeclaration()
    , m_address(std::move(address))
    , m_method(method)
    , m_where(where)
{
}

std::string MaterializedFunctionParameter::attributes() const
{
    return format(R"({} address="{}" where="{}")", BoundIdentifier::attributes(), address()->to_string(), where());
}

std::string MaterializedFunctionParameter::to_string() const
{
    return format("{} {} => {}", BoundIdentifier::to_string(), where(), address()->to_string());
}

std::shared_ptr<ObjectType> const& MaterializedFunctionParameter::declared_type() const
{
    return type();
}

std::shared_ptr<VariableAddress> MaterializedFunctionParameter::address() const
{
    return m_address;
}

MaterializedFunctionParameter::ParameterPassingMethod MaterializedFunctionParameter::method() const
{
    return m_method;
}

int MaterializedFunctionParameter::where() const
{
    return m_where;
}

// -- MaterializedFunctionDecl ----------------------------------------------

// -- MaterializedFunctionDecl --------------------------------------------------------

MaterializedFunctionDecl::MaterializedFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl, MaterializedFunctionParameters parameters, int nsaa, int stack_depth)
    : Statement(decl->token())
    , m_identifier(decl->identifier())
    , m_parameters(std::move(parameters))
    , m_nsaa(nsaa)
    , m_stack_depth(stack_depth)
{
    if (m_nsaa % 16)
        m_nsaa += 16 - (m_nsaa % 16);
}

std::shared_ptr<BoundIdentifier> const& MaterializedFunctionDecl::identifier() const
{
    return m_identifier;
}

std::string const& MaterializedFunctionDecl::name() const
{
    return identifier()->name();
}

std::shared_ptr<ObjectType> MaterializedFunctionDecl::type() const
{
    return identifier()->type();
}

MaterializedFunctionParameters const& MaterializedFunctionDecl::parameters() const
{
    return m_parameters;
}

int MaterializedFunctionDecl::nsaa() const
{
    return m_nsaa;
}

int MaterializedFunctionDecl::stack_depth() const
{
    return m_stack_depth;
}

std::string MaterializedFunctionDecl::attributes() const
{
    return format(R"(name="{}" return_type="{}" nsaa="{}" stack_depth="{}")", name(), type(), nsaa(), stack_depth());
}

Nodes MaterializedFunctionDecl::children() const
{
    Nodes ret;
    for (auto& param : m_parameters) {
        ret.push_back(param);
    }
    return ret;
}

std::string MaterializedFunctionDecl::to_string() const
{
    return format("func {}({}): {} [{}/{}]", name(), parameters_to_string(), type(), nsaa(), stack_depth());
}

std::string MaterializedFunctionDecl::label() const
{
    if (parameters().empty() || name() == "main")
        return name();
    size_t hash = 0u;
    for (auto const& param : parameters()) {
        hash ^= std::hash<ObjectType> {}(*param->type());
    }
    return format("{}_{}", name(), hash % 4096);
}

std::string MaterializedFunctionDecl::parameters_to_string() const
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

// -- MaterializedNativeFunctionDecl ----------------------------------------

MaterializedNativeFunctionDecl::MaterializedNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const& func_decl, MaterializedFunctionParameters parameters, int nsaa)
    : MaterializedFunctionDecl(func_decl, parameters, nsaa, 0)
    , m_native_function_name(func_decl->native_function_name())
{
}

std::string const& MaterializedNativeFunctionDecl::native_function_name() const
{
    return m_native_function_name;
}

std::string MaterializedNativeFunctionDecl::to_string() const
{
    return format("{} -> \"{}\"", MaterializedFunctionDecl::to_string(), m_native_function_name);
}

// -- MaterializedIntrinsicDecl ---------------------------------------------

MaterializedIntrinsicDecl::MaterializedIntrinsicDecl(std::shared_ptr<BoundIntrinsicDecl> const& decl, MaterializedFunctionParameters parameters, int nsaa)
    : MaterializedFunctionDecl(decl, parameters, nsaa, 0)
{
}

std::string MaterializedIntrinsicDecl::to_string() const
{
    return format("intrinsic {}({}): {}", name(), parameters_to_string(), type());
}

// -- MaterializedFunctionDef --------------------------------------------------------

MaterializedFunctionDef::MaterializedFunctionDef(std::shared_ptr<BoundFunctionDef> const& bound_def, std::shared_ptr<MaterializedFunctionDecl> func_decl, std::shared_ptr<Statement> statement, int stack_depth)
    : Statement(bound_def->token())
    , m_function_decl(std::move(func_decl))
    , m_statement(std::move(statement))
    , m_stack_depth(stack_depth)
{
    if (m_stack_depth % 16)
        m_stack_depth += 16 - (m_stack_depth % 16);
}

std::shared_ptr<MaterializedFunctionDecl> const& MaterializedFunctionDef::declaration() const
{
    return m_function_decl;
}

std::shared_ptr<BoundIdentifier> const& MaterializedFunctionDef::identifier() const
{
    return m_function_decl->identifier();
}

std::string const& MaterializedFunctionDef::name() const
{
    return identifier()->name();
}

std::shared_ptr<ObjectType> const& MaterializedFunctionDef::type() const
{
    return identifier()->type();
}

MaterializedFunctionParameters const& MaterializedFunctionDef::parameters() const
{
    return m_function_decl->parameters();
}

std::shared_ptr<Statement> const& MaterializedFunctionDef::statement() const
{
    return m_statement;
}

int MaterializedFunctionDef::stack_depth() const
{
    return m_stack_depth;
}

std::string MaterializedFunctionDef::to_string() const
{
    if (m_statement != nullptr)
        return format("{}\n{}", m_function_decl, m_statement);
    return m_function_decl->to_string();
}

Nodes MaterializedFunctionDef::children() const
{
    Nodes ret = { m_function_decl };
    if (m_statement) {
        ret.push_back(m_statement);
    }
    return ret;
}

std::string MaterializedFunctionDef::label() const
{
    return declaration()->label();
}

// -- MaterializedFunctionCall --------------------------------------------------------

MaterializedFunctionCall::MaterializedFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<MaterializedFunctionDecl> decl)
    : BoundExpression(call)
    , m_name(call->name())
    , m_arguments(std::move(arguments))
    , m_declaration(std::move(decl))
{
}

std::string MaterializedFunctionCall::attributes() const
{
    return format(R"(name="{}" type="{}")", name(), type());
}

Nodes MaterializedFunctionCall::children() const
{
    Nodes ret;
    for (auto& arg : m_arguments) {
        ret.push_back(arg);
    }
    return ret;
}

std::string MaterializedFunctionCall::to_string() const
{
    Strings args;
    for (auto& arg : m_arguments) {
        args.push_back(arg->to_string());
    }
    return format("{}({}): {}", name(), join(args, ","), type());
}

std::string const& MaterializedFunctionCall::name() const
{
    return m_name;
}

BoundExpressions const& MaterializedFunctionCall::arguments() const
{
    return m_arguments;
}

std::shared_ptr<MaterializedFunctionDecl> const& MaterializedFunctionCall::declaration() const
{
    return m_declaration;
}

ObjectTypes MaterializedFunctionCall::argument_types() const
{
    ObjectTypes ret;
    for (auto& arg : arguments()) {
        ret.push_back(arg->type());
    }
    return ret;
}

// -- MaterializedNativeFunctionCall --------------------------------------------------------

MaterializedNativeFunctionCall::MaterializedNativeFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<MaterializedNativeFunctionDecl> decl)
    : MaterializedFunctionCall(call, std::move(arguments), std::move(decl))
{
}

// -- MaterializedIntrinsicCall --------------------------------------------------------

MaterializedIntrinsicCall::MaterializedIntrinsicCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<MaterializedIntrinsicDecl> decl, IntrinsicType intrinsic)
    : MaterializedFunctionCall(call, std::move(arguments), std::move(decl))
    , m_intrinsic(intrinsic)
{
}

IntrinsicType MaterializedIntrinsicCall::intrinsic() const
{
    return m_intrinsic;
}

// -- MaterializedVariableDecl ----------------------------------------------

MaterializedVariableDecl::MaterializedVariableDecl(std::shared_ptr<BoundVariableDeclaration> const& var_decl, size_t offset, std::shared_ptr<BoundExpression> expression)
    : Statement(var_decl->token())
    , MaterializedDeclaration()
    , m_variable(var_decl->variable())
    , m_const(var_decl->is_const())
    , m_expression(std::move(expression))
    , m_offset(offset)
{
}

MaterializedVariableDecl::MaterializedVariableDecl(std::shared_ptr<BoundVariableDeclaration> const& var_decl, std::shared_ptr<BoundExpression> expression)
    : Statement(var_decl->token())
    , m_variable(var_decl->variable())
    , m_const(var_decl->is_const())
    , m_expression(std::move(expression))
{
}

std::string MaterializedVariableDecl::attributes() const
{
    return format(R"(name="{}" type="{}" is_const="{}" offset="{}")", name(), type(), is_const(), offset());
}

Nodes MaterializedVariableDecl::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string MaterializedVariableDecl::to_string() const
{
    auto keyword = (m_const) ? "const" : "var";
    if (m_expression)
        return format("{} {}: {} {}", keyword, m_variable, m_expression, offset());
    return format("{} {} {}", keyword, m_variable, offset());
}

std::shared_ptr<VariableAddress> MaterializedVariableDecl::address() const
{
    return std::make_shared<StackVariableAddress>(offset());
}

std::shared_ptr<BoundIdentifier> const& MaterializedVariableDecl::variable() const
{
    return m_variable;
}

std::string const& MaterializedVariableDecl::name() const
{
    return variable()->name();
}

std::shared_ptr<ObjectType> const& MaterializedVariableDecl::type() const
{
    return variable()->type();
}

bool MaterializedVariableDecl::is_const() const
{
    return m_const;
}

size_t MaterializedVariableDecl::offset() const
{
    return m_offset;
}

std::shared_ptr<BoundExpression> const& MaterializedVariableDecl::expression() const
{
    return m_expression;
}

std::shared_ptr<ObjectType> const& MaterializedVariableDecl::declared_type() const
{
    return type();
}

// -- MaterializedStaticVariableDecl --------------------------------------------------------

MaterializedStaticVariableDecl::MaterializedStaticVariableDecl(std::shared_ptr<BoundVariableDeclaration> const& var_decl, std::shared_ptr<BoundExpression> expression)
    : MaterializedVariableDecl(var_decl, std::move(expression))
{
}

std::string MaterializedStaticVariableDecl::label() const
{
    return format("_{}", name());
}

std::shared_ptr<VariableAddress> MaterializedStaticVariableDecl::address() const
{
    return std::make_shared<StaticVariableAddress>(label());
}

// -- MaterializedLocalVariableDecl --------------------------------------------------------

MaterializedLocalVariableDecl::MaterializedLocalVariableDecl(std::shared_ptr<BoundVariableDeclaration> const& var_decl, std::shared_ptr<BoundExpression> expression)
    : MaterializedVariableDecl(var_decl, std::move(expression))
{
}

// -- MaterializedGlobalVariableDecl --------------------------------------------------------

MaterializedGlobalVariableDecl::MaterializedGlobalVariableDecl(std::shared_ptr<BoundVariableDeclaration> const& var_decl, std::shared_ptr<BoundExpression> expression)
    : MaterializedVariableDecl(var_decl, std::move(expression))
{
}

// -- MaterializedVariableAccess --------------------------------------------------------

MaterializedVariableAccess::MaterializedVariableAccess(std::shared_ptr<BoundExpression> const& expr, std::shared_ptr<VariableAddress> address)
    : BoundVariableAccess(expr->token(), expr->type())
    , m_address(std::move(address))
{
}

std::shared_ptr<VariableAddress> const& MaterializedVariableAccess::address() const
{
    return m_address;
}

// -- MaterializedIdentifier --------------------------------------------------------

MaterializedIdentifier::MaterializedIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
    : MaterializedVariableAccess(identifier, std::move(address))
    , m_identifier(identifier->name())
{
}

std::string const& MaterializedIdentifier::name() const
{
    return m_identifier;
}

std::string MaterializedIdentifier::attributes() const
{
    return format(R"(name="{}" type="{}" address="{}")", name(), type(), address()->to_string());
}

std::string MaterializedIdentifier::to_string() const
{
    return format("{}: {} [{}]", name(), type()->to_string(), address()->to_string());
}

// -- MaterializedIntIdentifier --------------------------------------------------------

MaterializedIntIdentifier::MaterializedIntIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
    : MaterializedIdentifier(identifier, std::move(address))
{
}

// -- MaterializedStructIdentifier --------------------------------------------------------

MaterializedStructIdentifier::MaterializedStructIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
    : MaterializedIdentifier(identifier, std::move(address))
{
}

// -- MaterializedArrayIdentifier --------------------------------------------------------

MaterializedArrayIdentifier::MaterializedArrayIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
    : MaterializedIdentifier(identifier, std::move(address))
{
}

// -- MaterializedMemberAccess --------------------------------------------------------

MaterializedMemberAccess::MaterializedMemberAccess(std::shared_ptr<BoundMemberAccess> const& member_access, std::shared_ptr<MaterializedVariableAccess> strukt, std::shared_ptr<MaterializedIdentifier> member)
    : MaterializedVariableAccess(member_access, member->address())
    , m_struct(std::move(strukt))
    , m_member(std::move(member))
{
}

std::shared_ptr<MaterializedVariableAccess> const& MaterializedMemberAccess::structure() const
{
    return m_struct;
}

std::shared_ptr<MaterializedIdentifier> const& MaterializedMemberAccess::member() const
{
    return m_member;
}

std::string MaterializedMemberAccess::attributes() const
{
    return format(R"(type="{} address={}")", type(), address()->to_string());
}

Nodes MaterializedMemberAccess::children() const
{
    return { m_struct, m_member };
}

std::string MaterializedMemberAccess::to_string() const
{
    return format("{}.{}: {} {}", structure(), member(), type()->to_string(), address()->to_string());
}

// -- MaterializedArrayAccess --------------------------------------------------------

MaterializedArrayAccess::MaterializedArrayAccess(std::shared_ptr<BoundArrayAccess> const& array_access, std::shared_ptr<MaterializedVariableAccess> array, std::shared_ptr<BoundExpression> index, int element_size)
    : MaterializedVariableAccess(array_access, std::make_shared<ArrayElementAddress>(array->address(), element_size))
    , m_array(std::move(array))
    , m_element_size(element_size)
    , m_index(std::move(index))
{
}

std::shared_ptr<MaterializedVariableAccess> const& MaterializedArrayAccess::array() const
{
    return m_array;
}

std::shared_ptr<BoundExpression> const& MaterializedArrayAccess::index() const
{
    return m_index;
}

std::string MaterializedArrayAccess::attributes() const
{
    return format(R"(type="{}" element_size="{}")", type(), element_size());
}

Nodes MaterializedArrayAccess::children() const
{
    return { m_array, m_index };
}

std::string MaterializedArrayAccess::to_string() const
{
    return format("{}[{}]: {} {}", array(), index(), type()->to_string(), address()->to_string());
}

int MaterializedArrayAccess::element_size() const
{
    return m_element_size;
}

}
