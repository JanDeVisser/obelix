/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Type.h>

namespace Obelix {

std::unordered_map<ObelixType, std::shared_ptr<ObjectType>> ObjectType::s_types;

[[maybe_unused]] auto s_assignable = ObjectType::register_type(TypeAssignable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "=", TypeArgument, MethodParameter { "other", TypeCompatible } });
    });

[[maybe_unused]] auto s_incrementable = ObjectType::register_type(TypeIncrementable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "++", TypeArgument });
        type.add_method(MethodDescription { "--", TypeArgument });
        type.add_method(MethodDescription { "+=", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "-=", TypeArgument, MethodParameter { "other", TypeCompatible } });
    });

[[maybe_unused]] auto s_any = ObjectType::register_type(TypeAny,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "==", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "!=", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { ".", TypeAny, MethodParameter { "attribute", TypeString } });
        type.add_method(MethodDescription { "typename", TypeString });
        type.add_method(MethodDescription { "length", TypeInt });
        type.add_method(MethodDescription { "empty", TypeBoolean });
    });

[[maybe_unused]] auto s_comparable = ObjectType::register_type(TypeComparable,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "<", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "<=", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { ">", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { ">=", TypeBoolean, MethodParameter { "other", TypeCompatible } });
    });

[[maybe_unused]] auto s_integer_number = ObjectType::register_type(TypeIntegerNumber,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "+", TypeArgument });
        type.add_method(MethodDescription { "~", TypeArgument });
        type.add_method(MethodDescription { "+", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "-", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "*", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "/", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "|", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "&", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "^", TypeArgument, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription { "..", TypeRange, MethodParameter { "other", TypeCompatible } });
        type.will_be_a(TypeComparable);
        type.will_be_a(TypeAssignable);
        type.will_be_a(TypeIncrementable);
    });

[[maybe_unused]] auto s_signed_integer_number = ObjectType::register_type(TypeSignedIntegerNumber,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "-", TypeSelf });
        type.will_be_a(TypeIntegerNumber);
    });

[[maybe_unused]] auto s_integer = ObjectType::register_type(TypeInt,
    [](ObjectType& type) {
        type.will_be_a(TypeSignedIntegerNumber);
    });

[[maybe_unused]] auto s_unsigned = ObjectType::register_type(TypeUnsigned,
    [](ObjectType& type) {
        type.will_be_a(TypeIntegerNumber);
    });

[[maybe_unused]] auto s_byte = ObjectType::register_type(TypeByte,
    [](ObjectType& type) {
        type.will_be_a(TypeSignedIntegerNumber);
    });

[[maybe_unused]] auto s_char = ObjectType::register_type(TypeChar,
    [](ObjectType& type) {
        type.will_be_a(TypeIntegerNumber);
    });

[[maybe_unused]] auto s_string = ObjectType::register_type(TypeString,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "+", TypeString, MethodParameter { "other", TypeString } });
        type.add_method(MethodDescription { "*", TypeString, MethodParameter { "other", TypeInt } });
        type.will_be_a(TypeComparable);
    });

[[maybe_unused]] auto s_float = ObjectType::register_type(TypeFloat,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "+", TypeFloat });
        type.add_method(MethodDescription { "-", TypeFloat });
        type.add_method(MethodDescription { "+", TypeFloat, MethodParameter { "other", TypeFloat } });
        type.add_method(MethodDescription { "-", TypeFloat, MethodParameter { "other", TypeFloat } });
        type.add_method(MethodDescription { "*", TypeFloat, MethodParameter { "other", TypeFloat } });
        type.add_method(MethodDescription { "/", TypeFloat, MethodParameter { "other", TypeFloat } });
        type.will_be_a(TypeComparable);
    });

[[maybe_unused]] auto s_boolean = ObjectType::register_type(TypeBoolean,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "!", TypeBoolean });
        type.add_method(MethodDescription { "||", TypeBoolean, MethodParameter { "other", TypeBoolean } });
        type.add_method(MethodDescription { "&&", TypeBoolean, MethodParameter { "other", TypeBoolean } });
        type.add_method(MethodDescription { "^", TypeBoolean, MethodParameter { "other", TypeBoolean } });
    });

[[maybe_unused]] auto s_null = ObjectType::register_type(TypeNull,
    [](ObjectType& type) {
    });

[[maybe_unused]] auto s_list = ObjectType::register_type(TypeList,
    [](ObjectType& type) {
    });

[[maybe_unused]] auto s_dict = ObjectType::register_type(TypeObject,
    [](ObjectType& type) {
    });

ObelixType ObjectType::return_type_of(std::string_view method_name, ObelixTypes const& argument_types) const
{
    auto check_methods_of = [&method_name, &argument_types, this](ObjectType const& type) {
        for (auto& mth : type.m_methods) {
            if (mth.name() != method_name)
                continue;
            if (mth.parameters().size() != argument_types.size())
                continue;
            auto ix = 0;
            for (; ix < mth.parameters().size(); ++ix) {
                auto& param = mth.parameters()[ix];
                auto arg_type = argument_types[ix];
                if (param.type != arg_type && (param.type == ObelixType::TypeCompatible && arg_type != this->type()) && (param.type == ObelixType::TypeSelf && arg_type != this->type()))
                    break;
            }
            if (ix != mth.parameters().size())
                break;
            switch (mth.return_type()) {
            case TypeSelf:
                return type.type();
            case TypeArgument:
                return argument_types[0];
            default:
                return mth.return_type();
            }
        }
        return ObelixType::TypeUnknown;
    };

    std::vector<ObelixType> types { ObelixType::TypeAny, type() };
    while (!types.empty()) {
        ObelixType t = types.back();
        types.pop_back();
        auto type = s_types.at(t);
        for (auto is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (auto ret = check_methods_of(*type); ret != ObelixType::TypeUnknown)
            return ret;
    }
    return TypeUnknown;
}

std::optional<std::shared_ptr<ObjectType>> ObjectType::get(ObelixType type)
{
    if (!s_types.contains(type))
        return {};
    return s_types.at(type);
}

}
