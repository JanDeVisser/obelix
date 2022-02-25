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

std::unordered_map<ObelixType, std::shared_ptr<ObjectType>> ObjectType::s_types_by_id;
std::unordered_map<std::string, std::shared_ptr<ObjectType>> ObjectType::s_types_by_name;

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
        type.add_method(MethodDescription { "+", TypeString, MethodParameter { "other", TypeString } });
        type.add_method(MethodDescription { "*", TypeString, MethodParameter { "other", TypeInt } });
        type.will_be_a(TypeComparable);
        type.has_size(12);
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
        type.has_size(4);
    });

[[maybe_unused]] auto s_boolean = ObjectType::register_type(TypeBoolean,
    [](ObjectType& type) {
        type.add_method(MethodDescription { "!", TypeBoolean });
        type.add_method(MethodDescription { "||", TypeBoolean, MethodParameter { "other", TypeBoolean } });
        type.add_method(MethodDescription { "&&", TypeBoolean, MethodParameter { "other", TypeBoolean } });
        type.add_method(MethodDescription { "^", TypeBoolean, MethodParameter { "other", TypeBoolean } });
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
        type.add_method(MethodDescription { "++", TypeArgument });
        type.add_method(MethodDescription { "--", TypeArgument });
        type.add_method(MethodDescription { "+=", TypeArgument, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { "-=", TypeArgument, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { "+", TypeArgument, MethodParameter { "other", TypeInt } });
        type.add_method(MethodDescription { "-", TypeArgument, MethodParameter { "other", TypeInt } });
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
        auto type = s_types_by_id.at(t);
        for (auto is_a : type->m_is_a) {
            types.push_back(is_a);
        }
        if (auto ret = check_methods_of(*type); ret != ObelixType::TypeUnknown)
            return ret;
    }
    return TypeUnknown;
}

std::shared_ptr<ObjectType> ObjectType::get(ObelixType type)
{
    if (!s_types_by_id.contains(type)) {
        ObjectType::register_type(type,
            [](ObjectType& type) {
            });
    }
    return s_types_by_id.at(type);
}

std::shared_ptr<ObjectType> ObjectType::get(std::string const& type)
{
    if (s_types_by_name.contains(type))
        return s_types_by_name.at(type);
    auto type_maybe = ObelixType_by_name(type);
    if (type_maybe.has_value() && s_types_by_id.contains(type_maybe.value())) {
        auto ret = s_types_by_id.at(type_maybe.value());
        s_types_by_name[type] = ret;
        return ret;
    }
    return nullptr;
}

// std::shared_ptr<ObjectType> ObjectType::instantiate_template(std::shared_ptr<ObjectType> templ, std::vector<std::shared_ptr<ObjectType>> args)
//{
//
// }

}
