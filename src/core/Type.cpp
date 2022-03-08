/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Type.h>

namespace Obelix {

std::optional<ObelixType> ObelixType_by_name(std::string const& t)
{
    if (t == "int" || t == "s32")
        return ObelixType::TypeInt;
    if (t == "unsigned" || t == "u32")
        return ObelixType::TypeUnsigned;
    if (t == "byte" || t == "s8")
        return ObelixType::TypeByte;
    if (t == "char" || t == "u8")
        return ObelixType::TypeChar;
    if (t == "bool")
        return ObelixType::TypeBoolean;
    if (t == "string")
        return ObelixType::TypeString;
    if (t == "ptr")
        return ObelixType::TypePointer;
    return {};
}

MethodParameter::MethodParameter(char const* n, ObelixType t)
    : name(n)
    , type(ObjectType::get(t))
{
}

MethodParameter::MethodParameter(char const* n, std::shared_ptr<ObjectType> t)
    : name(n)
    , type(move(t))
{
}

MethodDescription::MethodDescription(char const* name, ObelixType type)
    : m_name(name)
    , m_is_operator(false)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
{
}

MethodDescription::MethodDescription(Operator op, ObelixType type)
    : m_operator(op)
    , m_is_operator(true)
    , m_return_type(ObjectType::get(type))
    , m_varargs(false)
{
}

std::unordered_map<ObelixType, std::shared_ptr<ObjectType>> ObjectType::s_types_by_id {};
std::unordered_map<std::string, std::shared_ptr<ObjectType>> ObjectType::s_types_by_name {};
std::vector<std::shared_ptr<ObjectType>> ObjectType::s_template_instantiations {};

[[maybe_unused]] auto s_self = ObjectType::register_type(TypeSelf);
[[maybe_unused]] auto s_argument = ObjectType::register_type(TypeArgument);
[[maybe_unused]] auto s_compatible = ObjectType::register_type(TypeCompatible);
[[maybe_unused]] auto s_unknown = ObjectType::register_type(TypeUnknown);

[[maybe_unused]] auto s_assignable = ObjectType::register_type(TypeAssignable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Assign, TypeArgument, MethodParameter { "other", TypeCompatible } });
    });

[[maybe_unused]] auto s_incrementable = ObjectType::register_type(TypeIncrementable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::UnaryIncrement, TypeArgument });
        type.add_method(MethodDescription { Operator::UnaryDecrement, TypeArgument });
        type.add_method(MethodDescription { Operator::BinaryIncrement, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::BinaryDecrement, TypeArgument, MethodParameter { "other", TypeCompatible } });
    });

[[maybe_unused]] auto s_any = ObjectType::register_type(TypeAny,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Equals, TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::NotEquals, TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::Dereference, TypeAny, MethodParameter { "attribute", TypeString } });
        type.add_method(MethodDescription { "typename", TypeString });
        type.add_method(MethodDescription { "length", TypeInt });
        type.add_method(MethodDescription { "empty", TypeBoolean });
    });

[[maybe_unused]] auto s_comparable = ObjectType::register_type(TypeComparable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Less, TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::LessEquals, TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::Greater, TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::GreaterEquals, TypeBoolean, MethodParameter { "other", TypeCompatible } });
    });

[[maybe_unused]] auto s_integer_number = ObjectType::register_type(TypeIntegerNumber,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Identity, TypeArgument });
        type.add_method(MethodDescription { Operator::BitwiseInvert, TypeArgument });
        type.add_method(MethodDescription { Operator::Add, TypeSelf, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::Subtract, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::Multiply, TypeSelf, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::Divide, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::BitwiseOr, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::BitwiseAnd, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::BitwiseXor, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::BitShiftLeft, TypeArgument, MethodParameter { "other", TypeUnsigned } });
        type.add_method(MethodDescription { Operator::BitShiftRight, TypeArgument, MethodParameter { "other", TypeUnsigned } });
        type.add_method(MethodDescription { Operator::Range, TypeRange, MethodParameter { "other", TypeCompatible } });
        type.will_be_a(TypeComparable);
        type.will_be_a(TypeAssignable);
        type.will_be_a(TypeIncrementable);
    });

[[maybe_unused]] auto s_signed_integer_number = ObjectType::register_type(TypeSignedIntegerNumber,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Negate, TypeArgument });
        type.will_be_a(TypeIntegerNumber);
    });

[[maybe_unused]] auto s_integer = ObjectType::register_type(TypeInt,
    [](ObjectType& type) {
        type.will_be_a(TypeSignedIntegerNumber);
        type.has_size(4);
    });

[[maybe_unused]] auto s_unsigned = ObjectType::register_type(TypeUnsigned,
    [](ObjectType& type) {
        type.will_be_a(TypeIntegerNumber);
        type.has_size(4);
    });

[[maybe_unused]] auto s_byte = ObjectType::register_type(TypeByte,
    [](ObjectType& type) {
        type.will_be_a(TypeSignedIntegerNumber);
        type.has_size(1);
    });

[[maybe_unused]] auto s_char = ObjectType::register_type(TypeChar,
    [](ObjectType& type) {
        type.will_be_a(TypeIntegerNumber);
        type.has_size(1);
    });

[[maybe_unused]] auto s_string = ObjectType::register_type(TypeString,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Add, TypeString, MethodParameter { "other", TypeString } });
        type.add_method(MethodDescription { Operator::Multiply, TypeString, MethodParameter { "other", TypeUnsigned } });
        type.will_be_a(TypeComparable);
        type.has_size(12);
    });

[[maybe_unused]] auto s_float = ObjectType::register_type(TypeFloat,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::Identity, TypeFloat });
        type.add_method(MethodDescription { Operator::Negate, TypeFloat });
        type.add_method(MethodDescription { Operator::Add, TypeFloat, MethodParameter { "other", TypeFloat } });
        type.add_method(MethodDescription { Operator::Subtract, TypeFloat, MethodParameter { "other", TypeFloat } });
        type.add_method(MethodDescription { Operator::Multiply, TypeFloat, MethodParameter { "other", TypeFloat } });
        type.add_method(MethodDescription { Operator::Divide, TypeFloat, MethodParameter { "other", TypeFloat } });
        type.will_be_a(TypeComparable);
        type.has_size(4);
    });

[[maybe_unused]] auto s_boolean = ObjectType::register_type(TypeBoolean,
    [](ObjectType& type) {
        type.add_method(MethodDescription { Operator::LogicalInvert, TypeBoolean });
        type.add_method(MethodDescription { Operator::LogicalAnd, TypeBoolean, MethodParameter { "other", TypeBoolean } });
        type.add_method(MethodDescription { Operator::LogicalOr, TypeBoolean, MethodParameter { "other", TypeBoolean } });
        type.add_method(MethodDescription { Operator::BitwiseOr, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::BitwiseAnd, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { Operator::BitwiseXor, TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.has_size(1);
    });

[[maybe_unused]] auto s_null = ObjectType::register_type(TypeNull,
    [](ObjectType& type) {
    });

[[maybe_unused]] auto s_list = ObjectType::register_type(TypeList,
    [](ObjectType& type) {
    });

[[maybe_unused]] auto s_pointer = ObjectType::register_type(TypePointer,
    [](ObjectType& type) {
        type.has_template_parameter("target");
        type.has_size(8);
        type.add_method(MethodDescription { Operator::UnaryIncrement, TypeArgument });
        type.add_method(MethodDescription { Operator::UnaryDecrement, TypeArgument });
        type.add_method(MethodDescription { Operator::BinaryIncrement, TypeArgument, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { Operator::BinaryDecrement, TypeArgument, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { Operator::Add, TypeArgument, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { Operator::Subtract, TypeArgument, MethodParameter { "other", TypeInt } });
    });

[[maybe_unused]] auto s_dict = ObjectType::register_type(TypeObject,
    [](ObjectType& type) {
    });

std::string ObjectType::to_string() const
{
    std::string ret = name();
    auto glue = '<';
    for (auto& parameter : template_arguments()) {
        ret += glue;
        ret += parameter->to_string();
        glue = ',';
    }
    if (glue == ',')
        ret += '>';
    return ret;
}

bool ObjectType::operator==(ObjectType const& other) const
{
    if (name() != other.name())
        return false;
    if (template_arguments().size() != other.template_arguments().size())
        return false;
    for (auto ix = 0; ix < template_arguments().size(); ++ix) {
        if (*template_arguments()[ix] != *other.template_arguments()[ix])
            return false;
    }
    return true;
}

bool ObjectType::is_a(ObjectType const* other) const
{
    if ((*other == *this) || (other->type() == TypeAny))
        return true;
    return std::any_of(m_is_a.begin(), m_is_a.end(), [&other](auto& super_type) { return super_type->is_a(other); });
}

bool ObjectType::is_compatible(MethodDescription const& mth, ObjectTypes const& argument_types) const
{
    if (mth.parameters().size() != argument_types.size())
        return false;
    auto ix = 0;
    for (; ix < mth.parameters().size(); ++ix) {
        auto& param = mth.parameters()[ix];
        auto arg_type = argument_types[ix];
        if (!arg_type->is_a(param.type) && (*param.type == *s_compatible && !arg_type->is_a(this)) && (*(param.type) == *s_self && *arg_type != *this))
            break;
    }
    return (ix == mth.parameters().size());
}

std::optional<std::shared_ptr<ObjectType>> ObjectType::return_type_of(std::string_view method_name, ObjectTypes const& argument_types) const
{
    auto check_methods_of = [&method_name, &argument_types, this](ObjectType const* type) -> std::shared_ptr<ObjectType> const& {
        for (auto& mth : type->m_methods) {
            if (mth.is_operator())
                continue;
            if (mth.name() != method_name)
                continue;
            if (!is_compatible(mth, argument_types))
                continue;
            if (*(mth.return_type()) == *s_self)
                return ObjectType::get(this);
            if (*(mth.return_type()) == *s_argument)
                return argument_types[0];
            return mth.return_type();
        }
        return s_unknown;
    };

    auto self = get(this);
    ObjectTypes types { s_any, self };
    while (!types.empty()) {
        auto type = types.back();
        types.pop_back();
        for (auto& is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (auto ret = check_methods_of(type.get()); *ret != *s_unknown)
            return ret;
    }
    return {};
}

std::optional<std::shared_ptr<ObjectType>> ObjectType::return_type_of(Operator op, ObjectTypes const& argument_types) const
{
    auto check_operators_of = [&op, &argument_types, this](ObjectType const* type) -> std::shared_ptr<ObjectType> const& {
        for (auto& mth : type->m_methods) {
            if (!mth.is_operator())
                continue;
            if (mth.op() != op)
                continue;
            if (!is_compatible(mth, argument_types))
                continue;
            if (*(mth.return_type()) == *s_self)
                return ObjectType::get(this);
            if (*(mth.return_type()) == *s_argument)
                return argument_types[0];
            return mth.return_type();
        }
        return s_unknown;
    };

    auto self = get(this);
    ObjectTypes types { s_any, self };
    while (!types.empty()) {
        auto type = types.back();
        types.pop_back();
        for (auto& is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (auto ret = check_operators_of(type.get()); *ret != *s_unknown)
            return ret;
    }
    return {};
}

std::shared_ptr<ObjectType> const& ObjectType::get(ObelixType type)
{
    if (!s_types_by_id.contains(type)) {
        ObjectType::register_type(type,
            [](ObjectType& type) {
            });
    }
    return s_types_by_id.at(type);
}

std::shared_ptr<ObjectType> const& ObjectType::get(std::string const& type)
{
    if (s_types_by_name.contains(type))
        return s_types_by_name.at(type);
    auto type_maybe = ObelixType_by_name(type);
    if (type_maybe.has_value() && s_types_by_id.contains(type_maybe.value())) {
        auto ret = s_types_by_id.at(type_maybe.value());
        s_types_by_name[type] = ret;
        return s_types_by_name[type];
    }
    return s_unknown;
}

std::shared_ptr<ObjectType> const& ObjectType::get(ObjectType const* type)
{
    if (!type->is_parameterized())
        return get(type->name());

    for (auto& instantiation : s_template_instantiations) {
        if (*type == *instantiation)
            return instantiation;
    }
    return s_unknown;
}

ErrorOr<std::shared_ptr<ObjectType>> ObjectType::resolve(std::string const& type_name, ObjectTypes const& template_args)
{
    auto base_type = ObjectType::get(type_name);
    if (base_type == nullptr || *base_type == *s_unknown)
        return Error { ErrorCode::NoSuchType, type_name };
    if (base_type->is_parameterized() && (template_args.size() != base_type->template_parameters().size()))
        return Error { ErrorCode::TemplateParameterMismatch, type_name, base_type->template_parameters().size(), template_args.size() };
    if (!base_type->is_parameterized() && !template_args.empty())
        return Error { ErrorCode::TypeNotParameterized, type_name };
    if (!base_type->is_parameterized())
        return base_type;
    for (auto& template_instantiation : s_template_instantiations) {
        if (template_instantiation->instatiates_template() == base_type) {
            size_t ix;
            for (ix = 0; ix < template_args.size(); ++ix) {
                if (template_instantiation->template_arguments()[ix] != template_args[ix])
                    break;
            }
            if (ix >= template_args.size())
                return template_instantiation;
        }
    }
    auto instantiation = std::make_shared<ObjectType>(base_type->type(), [&template_args, &base_type](ObjectType& new_type) {
        new_type.m_instantiates_template = base_type;
        new_type.m_template_arguments = template_args;
    });
    s_template_instantiations.push_back(instantiation);
    return instantiation;
}

}
