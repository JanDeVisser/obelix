/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/ExpressionType.h>

namespace Obelix {

std::unordered_map<std::string, std::shared_ptr<ExpressionType>> ExpressionType::s_types_by_name {};
std::unordered_map<TypeID, std::shared_ptr<ExpressionType>> ExpressionType::s_types_by_id {};

std::shared_ptr<ExpressionType> ExpressionType::simple_type(std::string const& type)
{
    if (s_types_by_name.contains(type))
        return s_types_by_name.at(type);
    auto type_maybe = ObelixType_by_name(type);
    if (!type_maybe.has_value())
        return nullptr;
    if (s_types_by_id.contains(type_maybe.value())) {
        auto ret = s_types_by_id.at(type_maybe.value());
        s_types_by_name[type] = ret;
        return ret;
    }
    auto ret = std::make_shared<NativeExpressionType>(type_maybe.value());
    s_types_by_name[type] = ret;
    s_types_by_id[ret->type_id()] = ret;
    return ret;
}

std::shared_ptr<ExpressionType> ExpressionType::simple_type(TypeID type)
{
    if (s_types_by_id.contains(type))
        return s_types_by_id.at(type);
    if (type < ObelixType::TypeMaxNativeType) {
        auto t = static_cast<ObelixType>(type);
        auto ret = std::make_shared<NativeExpressionType>(t);
        s_types_by_name[ret->to_string()] = ret;
        s_types_by_id[ret->type_id()] = ret;
        return ret;
    }
    return nullptr;
}

std::shared_ptr<ExpressionType> ExpressionType::get_type(TypeID id)
{
    if (s_types_by_id.contains(id))
        return s_types_by_id[id];
    return nullptr;
}

std::shared_ptr<ExpressionType> ExpressionType::get_type(std::string const& type)
{
    if (s_types_by_name.contains(type))
        return s_types_by_name[type];
    return nullptr;
}

std::string type_name(std::shared_ptr<ExpressionType> const& type)
{
    return (type != nullptr) ? type->to_string() : ObelixType_name(ObelixType::TypeUnknown);
}

}
