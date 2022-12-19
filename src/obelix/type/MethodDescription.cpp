/*
* Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
*
* SPDX-License-Identifier: MIT
*/

#include <obelix/BoundSyntaxNode.h>
#include <obelix/type/Type.h>

namespace Obelix {

MethodParameter::MethodParameter(char const* n, PrimitiveType t)
    : name(n)
    , type(ObjectType::get(t))
{
}

MethodParameter::MethodParameter(char const* n, pObjectType t)
    : name(n)
    , type(std::move(t))
{
}

MethodParameter::MethodParameter(std::string n, pObjectType t)
    : name(std::move(n))
    , type(std::move(t))
{
}

MethodDescription::MethodDescription(MethodDescription const& other, pObjectType owner)
    : MethodDescription(other)
{
    m_method_of = std::move(owner);
}

MethodDescription::MethodDescription(char const* name, PrimitiveType type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_name(name)
    , m_is_operator(false)
    , m_is_pure(pure)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
    , m_parameters(std::move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(char const* name, pObjectType type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_name(name)
    , m_is_operator(false)
    , m_is_pure(pure)
    , m_return_type(std::move(type))
    , m_varargs(false)
    , m_parameters(std::move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(std::string name, pObjectType type, MethodParameters parameters)
    : m_name(std::move(name))
    , m_is_operator(false)
    , m_return_type(std::move(type))
    , m_varargs(false)
    , m_parameters(std::move(parameters))
{
}

MethodDescription::MethodDescription(Operator op, PrimitiveType type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_operator(op)
    , m_is_operator(true)
    , m_is_pure(pure)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
    , m_parameters(std::move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

MethodDescription::MethodDescription(Operator op, pObjectType type, IntrinsicType intrinsic, MethodParameters parameters, bool pure)
    : m_operator(op)
    , m_is_operator(true)
    , m_is_pure(pure)
    , m_return_type(std::move(type))
    , m_varargs(false)
    , m_parameters(std::move(parameters))
{
    if (intrinsic != IntrinsicType::NotIntrinsic)
        set_default_implementation(intrinsic);
}

void MethodDescription::set_default_implementation(std::string const& native_function)
{
    m_default_implementation.is_intrinsic = false;
    m_default_implementation.native_function = native_function;
    m_default_implementation.intrinsic = IntrinsicType::NotIntrinsic;
}

void MethodDescription::set_default_implementation(IntrinsicType intrinsic)
{
    assert(intrinsic > IntrinsicType::NotIntrinsic && intrinsic < IntrinsicType::count);
    m_default_implementation.is_intrinsic = true;
    m_default_implementation.intrinsic = intrinsic;
    m_default_implementation.native_function = "";
}

void MethodDescription::set_implementation(Architecture arch, IntrinsicType intrinsic)
{
    MethodImpl impl { .is_intrinsic = false, .intrinsic = intrinsic };
    m_implementations[arch] = impl;
}

void MethodDescription::set_implementation(Architecture arch, std::string const& native_function)
{
    MethodImpl impl { .is_intrinsic = false, .native_function = native_function };
    m_implementations[arch] = impl;
}

Operator MethodDescription::op() const
{
    return m_operator;
}

pObjectType const& MethodDescription::return_type() const {
    return m_return_type;
}

void MethodDescription::set_return_type(pObjectType const& ret_type) {
    m_return_type = ret_type;
}

bool MethodDescription::varargs() const
{
    return m_varargs;
}

MethodParameters const& MethodDescription::parameters() const
{
    return m_parameters;
}

bool MethodDescription::is_operator() const
{
    return m_is_operator;
}

bool MethodDescription::is_pure() const
{
    return m_is_pure;
}

pObjectType const& MethodDescription::method_of() const
{
    return m_method_of;
}

std::shared_ptr<BoundFunctionDecl> MethodDescription::declaration() const
{
    auto ident = std::make_shared<BoundIdentifier>(Token {}, method_of()->name() + '$' + name(), return_type());
    BoundIdentifiers params;
    params.push_back(make_node<BoundIdentifier>(Token {}, "this", method_of()));
    for (auto const& p : parameters()) {
        params.push_back(make_node<BoundIdentifier>(Token {}, p.name, p.type));
    }
    if (implementation().is_intrinsic)
        return std::make_shared<BoundIntrinsicDecl>("/", ident, params);
    return std::make_shared<BoundFunctionDecl>("/", ident, params);
}


MethodImpl const& MethodDescription::implementation(Architecture arch) const
{
    if (m_implementations.contains(arch))
        return m_implementations.at(arch);
    return m_default_implementation;
}

MethodImpl const& MethodDescription::implementation() const
{
    return m_default_implementation;
}

std::string const& MethodDescription::name() const
{
    if (m_is_operator)
        return Operator_name_as_string(m_operator);
    return m_name;
}

void MethodDescription::set_method_of(pObjectType method_of)
{
    m_method_of = std::move(method_of);
}

bool MethodDescription::is_compatible(ObjectTypes const& argument_types) const
{
    if (parameters().size() != argument_types.size())
        return false;
    auto ix = 0u;
    for (; ix < parameters().size(); ++ix) {
        auto const& param = parameters()[ix];
        auto const& arg_type = argument_types[ix];
        switch (param.type->type()) {
            case PrimitiveType::Self:
                if (*arg_type != *method_of())
                    return false;
                break;
            case PrimitiveType::Compatible:
                if (auto smallest = arg_type->smallest_compatible_type(); !smallest.has_value() || !param.type->is_compatible_with(smallest.value()))
                    return false;
                break;
            case PrimitiveType::AssignableTo:
                if (!param.type->is_assignable_to(arg_type))
                    return false;
                break;
            default:
                if (auto smallest = arg_type->smallest_compatible_type(); !smallest.has_value() || !param.type->is_compatible_with(smallest.value()))
                    return false;
                break;
        }
    }
    return true;
}


}
