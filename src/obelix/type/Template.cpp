/*
* Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
*
* SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <obelix/type/Type.h>

namespace Obelix {

const char* TemplateParameterType_name(TemplateParameterType t)
{
    switch (t) {
#undef ENUM_TEMPLATE_PARAMETER_TYPE
#define ENUM_TEMPLATE_PARAMETER_TYPE(t) \
    case TemplateParameterType::t:      \
        return #t;
        ENUMERATE_TEMPLATE_PARAMETER_TYPES(ENUM_TEMPLATE_PARAMETER_TYPE)
#undef ENUM_TEMPLATE_PARAMETER_TYPE
    default:
        fatal("Unknown TemplateParameterType '{}'", (int)t);
    }
}

const char* TemplateParameterMultiplicity_name(TemplateParameterMultiplicity t)
{
    switch (t) {
#undef ENUM_TEMPLATE_PARAMETER_MULTIPLICITY
#define ENUM_TEMPLATE_PARAMETER_MULTIPLICITY(t) \
    case TemplateParameterMultiplicity::t:      \
        return #t;
        ENUMERATE_TEMPLATE_PARAMETER_MULTIPLICITIES(ENUM_TEMPLATE_PARAMETER_MULTIPLICITY)
#undef ENUM_TEMPLATE_PARAMETER_MULTIPLICITY
    default:
        fatal("Unknown TemplateParameterMultiplicity '{}'", (int)t);
    }
}

std::string TemplateParameter::to_string() const
{
    return format("{} {} {}", multiplicity, type, name);
}

size_t hash(TemplateArgumentValue const& arg)
{
    if (std::holds_alternative<long>(arg))
        return std::hash<long> {}(std::get<long>(arg));
    if (std::holds_alternative<std::string>(arg))
        return std::hash<std::string> {}(std::get<std::string>(arg));
    if (std::holds_alternative<std::shared_ptr<ObjectType>>(arg))
        return std::hash<std::shared_ptr<ObjectType>> {}(std::get<std::shared_ptr<ObjectType>>(arg));
    if (std::holds_alternative<bool>(arg))
        return std::hash<bool> {}(std::get<bool>(arg));
    assert(std::holds_alternative<NVP>(arg));
    auto nvp = std::get<NVP>(arg);
    return std::hash<std::string> {}(nvp.first) ^ std::hash<long> {}(nvp.second);
}

std::string to_string(TemplateArgumentValue const& arg)
{
    if (std::holds_alternative<long>(arg))
        return Obelix::to_string(std::get<long>(arg));
    if (std::holds_alternative<std::string>(arg))
        return std::get<std::string>(arg);
    if (std::holds_alternative<std::shared_ptr<ObjectType>>(arg))
        return std::get<std::shared_ptr<ObjectType>>(arg)->to_string();
    if (std::holds_alternative<bool>(arg))
        return Obelix::to_string(std::get<bool>(arg));
    assert(std::holds_alternative<NVP>(arg));
    auto nvp = std::get<NVP>(arg);
    return format("{}={}", nvp.first, nvp.second);
}

bool compare(TemplateArgumentValue const& arg1, TemplateArgumentValue const& arg2, bool match_any)
{
    if (arg1.index() != arg2.index())
        return false;
    if (arg1.valueless_by_exception() != arg2.valueless_by_exception())
        return false;
    if (std::holds_alternative<std::shared_ptr<ObjectType>>(arg1)) {
        auto type1 = std::get<std::shared_ptr<ObjectType>>(arg1);
        auto type2 = std::get<std::shared_ptr<ObjectType>>(arg2);
        if (match_any && (type2->type() == PrimitiveType::Any || type1->type() == PrimitiveType::Any))
            return true;
        return *type1 == *type2;
    }
    return arg1 == arg2;
}

TemplateArgument::TemplateArgument(TemplateParameterType type)
    : parameter_type(type)
    , multiplicity(TemplateParameterMultiplicity::Optional)
{
}

TemplateArgument::TemplateArgument(std::shared_ptr<ObjectType> type)
    : parameter_type(TemplateParameterType::Type)
{
    value.push_back(type);
}

TemplateArgument::TemplateArgument(long integer)
    : parameter_type(TemplateParameterType::Integer)
{
    value.push_back(integer);
}

TemplateArgument::TemplateArgument(int integer)
    : parameter_type(TemplateParameterType::Integer)
{
    value.push_back(integer);
}

TemplateArgument::TemplateArgument(std::string string)
    : parameter_type(TemplateParameterType::String)
{
    value.push_back(std::move(string));
}

TemplateArgument::TemplateArgument(bool boolean)
    : parameter_type(TemplateParameterType::Boolean)
{
    value.push_back(boolean);
}

TemplateArgument::TemplateArgument(std::string name, long nvp_value)
    : parameter_type(TemplateParameterType::NameValue)
{
    value.push_back(std::make_pair(std::move(name), nvp_value));
}

TemplateArgument::TemplateArgument(NVP nvp)
    : parameter_type(TemplateParameterType::NameValue)
{
    value.push_back(std::move(nvp));
}

TemplateArgument::TemplateArgument(TemplateParameterType type, TemplateArgumentValues arguments)
    : parameter_type(type)
    , multiplicity(TemplateParameterMultiplicity::Multiple)
    , value(move(arguments))
{
    assert(std::all_of(value.begin(), value.end(), [type](auto const& arg) {
        switch (type) {
        case TemplateParameterType::String:
            return std::holds_alternative<std::string>(arg);
        case TemplateParameterType::Integer:
            return std::holds_alternative<long>(arg);
        case TemplateParameterType::Boolean:
            return std::holds_alternative<bool>(arg);
        case TemplateParameterType::Type:
            return std::holds_alternative<std::shared_ptr<ObjectType>>(arg);
        case TemplateParameterType::NameValue:
            return std::holds_alternative<NVP>(arg);
        default:
            fatal("Unknown TemplateParameterType {}", type);
        }
    }));
    assert(value.size() <= 1 || multiplicity == TemplateParameterMultiplicity::Multiple);
    assert(!value.empty() || multiplicity == TemplateParameterMultiplicity::Optional);
}

std::shared_ptr<ObjectType> TemplateArgument::as_type() const
{
    assert(!value.empty());
    assert(std::holds_alternative<std::shared_ptr<ObjectType>>(value[0]));
    assert(parameter_type == TemplateParameterType::Type);
    return std::get<std::shared_ptr<ObjectType>>(value[0]);
}

long TemplateArgument::as_integer() const
{
    assert(!value.empty());
    assert(parameter_type == TemplateParameterType::Integer);
    assert(std::holds_alternative<long>(value[0]));
    return std::get<long>(value[0]);
}

std::string const& TemplateArgument::as_string() const
{
    assert(!value.empty());
    assert(parameter_type == TemplateParameterType::String);
    assert(std::holds_alternative<std::string>(value[0]));
    return std::get<std::string>(value[0]);
}

bool TemplateArgument::as_bool() const
{
    assert(!value.empty());
    assert(parameter_type == TemplateParameterType::Boolean);
    assert(std::holds_alternative<bool>(value[0]));
    return std::get<bool>(value[0]);
}

NVP const& TemplateArgument::as_nvp() const
{
    assert(!value.empty());
    assert(parameter_type == TemplateParameterType::NameValue);
    assert(std::holds_alternative<NVP>(value[0]));
    return std::get<NVP>(value[0]);
}

TemplateArgumentValues const& TemplateArgument::as_values() const
{
    return value;
}

size_t TemplateArgument::hash() const
{
    size_t ret = std::hash<int> {}(static_cast<int>(parameter_type));
    for (auto const& arg : value) {
        ret ^= Obelix::hash(arg);
    }
    return ret;
}

std::string TemplateArgument::to_string() const
{
    if (multiplicity == TemplateParameterMultiplicity::Multiple) {
        return format("[ {} ]", join(value, ' ', [](TemplateArgumentValue const& v) { return Obelix::to_string(v); }));
    }
    if (!value.empty())
        return Obelix::to_string(value[0]);
    return "";
}

bool TemplateArgument::operator==(TemplateArgument const& other) const
{
    if (parameter_type != other.parameter_type || multiplicity != other.multiplicity)
        return false;
    if (value.size() != other.value.size())
        return false;
    for (auto ix = 0u; ix < value.size(); ++ix) {
        if (!compare(value[ix], other.value[ix]))
            return false;
    }
    return true;
}

}
