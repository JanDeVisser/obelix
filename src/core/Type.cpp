/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Type.h>

namespace Obelix {

std::unordered_map<ObelixType, std::shared_ptr<ObjectType>> ObjectType::s_types;

[[maybe_unused]] auto s_any = ObjectType::register_type(TypeAny,
    [](ObjectType& type) {
        type.add_method(MethodDescription {"==", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription {"!=", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription {".", TypeUnknown, MethodParameter { "attribute", TypeString } });
        type.add_method(MethodDescription {"typename", TypeString });
        type.add_method(MethodDescription {"length", TypeInt });
        type.add_method(MethodDescription {"empty", TypeBoolean });
    });

[[maybe_unused]] auto s_comparable = ObjectType::register_type(TypeComparable,
    [](ObjectType& type) {
        type.add_method(MethodDescription {"<", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription {"<=", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription {">", TypeBoolean, MethodParameter { "other", TypeCompatible } });
        type.add_method(MethodDescription {">=", TypeBoolean, MethodParameter { "other", TypeCompatible } });
    });

[[maybe_unused]] auto s_assignable = ObjectType::register_type(TypeAssignable,
    [](ObjectType& type) {
        type.add_method(MethodDescription {"=", TypeArgument, MethodParameter { "other", TypeAny } });
    });

[[maybe_unused]] auto s_integer = ObjectType::register_type(TypeInt,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "+", TypeInt });
        type.add_method(MethodDescription { "-", TypeInt });
        type.add_method(MethodDescription { "+", TypeInt, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { "-", TypeInt, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { "*", TypeInt, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { "/", TypeInt, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { "..", TypeRange, MethodParameter { "other", TypeInt } });
        type.will_be_a(TypeComparable);
    });

[[maybe_unused]] auto s_string = ObjectType::register_type(TypeString,
    [](ObjectType& type) {
        type.add_method(MethodDescription {"+", TypeString, MethodParameter { "other", TypeString } });
        type.add_method(MethodDescription {"*", TypeString, MethodParameter { "other", TypeInt } });
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
    for (auto& mth : m_methods) {
        if (mth.name() != method_name)
            continue;
        if (mth.parameters().size() != argument_types.size())
            continue;
        auto ix = 0;
        for (; ix < mth.parameters().size(); ++ix) {
            auto& param = mth.parameters()[ix];
            auto arg_type = argument_types[ix];
            if (param.type != arg_type)
                break;
        }
        if (ix != mth.parameters().size())
            break;
        switch (mth.return_type()) {
        case TypeSelf:
            return type();
        case TypeArgument:
            return argument_types[0];
        default:
            return mth.return_type();
        }
    }
    return TypeError;
}


std::optional<std::shared_ptr<ObjectType>> ObjectType::get(ObelixType type)
{
    if (!s_types.contains(type))
        return {};
    return s_types.at(type);
}

}
