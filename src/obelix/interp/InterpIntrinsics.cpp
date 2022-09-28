/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/interp/InterpIntrinsics.h>
#include <obelix/Intrinsics.h>

namespace Obelix {

logging_category(interp);

static std::array<InterpImplementation, IntrinsicType::count> s_intrinsics = {};

bool register_interp_intrinsic(IntrinsicType type, InterpImplementation intrinsic)
{
    s_intrinsics[type] = move(intrinsic);
    return true;
}

InterpImplementation const& get_interp_intrinsic(IntrinsicType type)
{
    assert(type > IntrinsicType::NotIntrinsic && type < IntrinsicType::count);
    return s_intrinsics[type];
}

#define INTRINSIC(intrinsic)                                                                                              \
    ErrorOrNode interp_intrinsic_##intrinsic(InterpContext& ctx, BoundLiterals);                                          \
    auto s_interp_##intrinsic##_decl = register_interp_intrinsic(IntrinsicType::intrinsic, interp_intrinsic_##intrinsic); \
    ErrorOrNode interp_intrinsic_##intrinsic(InterpContext& ctx, BoundLiterals params)

#define INTRINSIC_ALIAS(intrinsic, alias) \
    auto s_##intrinsic##_decl = register_interp_intrinsic(IntrinsicType::intrinsic, interp_intrinsic_##alias); \

INTRINSIC(int_to_string)
{
    auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    if (int_literal != nullptr)
        return std::make_shared<BoundStringLiteral>(int_literal->token(), int_literal->string_value());
    return params[0];
}

INTRINSIC(add_int_int)
{
    auto int_literal_1 = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    auto int_literal_2 = std::dynamic_pointer_cast<BoundIntLiteral>(params[1]);
    if (int_literal_1 != nullptr && int_literal_1 != nullptr)
        return std::make_shared<BoundIntLiteral>(int_literal_1->token(), int_literal_1->int_value() + int_literal_2->int_value());
    return params[0];
}

INTRINSIC(subtract_int_int)
{
    auto int_literal_1 = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    auto int_literal_2 = std::dynamic_pointer_cast<BoundIntLiteral>(params[1]);
    if (int_literal_1 != nullptr && int_literal_1 != nullptr)
        return std::make_shared<BoundIntLiteral>(int_literal_1->token(), int_literal_1->int_value() - int_literal_2->int_value());
    return params[0];
}

INTRINSIC(multiply_int_int)
{
    auto int_literal_1 = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    auto int_literal_2 = std::dynamic_pointer_cast<BoundIntLiteral>(params[1]);
    if (int_literal_1 != nullptr && int_literal_1 != nullptr)
        return std::make_shared<BoundIntLiteral>(int_literal_1->token(), int_literal_1->int_value() * int_literal_2->int_value());
    return params[0];
}

INTRINSIC(divide_int_int)
{
    auto int_literal_1 = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    auto int_literal_2 = std::dynamic_pointer_cast<BoundIntLiteral>(params[1]);
    if (int_literal_1 != nullptr && int_literal_1 != nullptr)
        return std::make_shared<BoundIntLiteral>(int_literal_1->token(), int_literal_1->int_value() / int_literal_2->int_value());
    return params[0];
}

INTRINSIC(equals_int_int)
{
    auto int_literal_1 = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    auto int_literal_2 = std::dynamic_pointer_cast<BoundIntLiteral>(params[1]);
    if (int_literal_1 != nullptr && int_literal_1 != nullptr)
        return std::make_shared<BoundBooleanLiteral>(int_literal_1->token(), int_literal_1->int_value() == int_literal_2->int_value());
    return params[0];
}

INTRINSIC(greater_int_int)
{
    auto int_literal_1 = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    auto int_literal_2 = std::dynamic_pointer_cast<BoundIntLiteral>(params[1]);
    if (int_literal_1 != nullptr && int_literal_1 != nullptr)
        return std::make_shared<BoundBooleanLiteral>(int_literal_1->token(), int_literal_1->int_value() > int_literal_2->int_value());
    return params[0];
}

INTRINSIC(less_int_int)
{
    auto int_literal_1 = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    auto int_literal_2 = std::dynamic_pointer_cast<BoundIntLiteral>(params[1]);
    if (int_literal_1 != nullptr && int_literal_1 != nullptr)
        return std::make_shared<BoundBooleanLiteral>(int_literal_1->token(), int_literal_1->int_value() < int_literal_2->int_value());
    return params[0];
}

INTRINSIC(negate_s64)
{
    auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    if (int_literal != nullptr)
        return std::make_shared<BoundIntLiteral>(int_literal->token(), 0 - int_literal->int_value(), int_literal->type());
    return params[0];
}

INTRINSIC_ALIAS(negate_s32, negate_s64)
INTRINSIC_ALIAS(negate_s16, negate_s64)
INTRINSIC_ALIAS(negate_s8, negate_s64)

INTRINSIC(invert_int)
{
    auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(params[0]);
    if (int_literal != nullptr)
        return std::make_shared<BoundIntLiteral>(int_literal->token(), ~(int_literal->int_value()));
    return params[0];
}

INTRINSIC(invert_bool)
{
    auto bool_literal = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[0]);
    if (bool_literal != nullptr)
        return std::make_shared<BoundBooleanLiteral>(bool_literal->token(), !(bool_literal->bool_value()));
    return params[0];
}

INTRINSIC(and_bool_bool)
{
    auto bool_literal_1 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[0]);
    auto bool_literal_2 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[1]);
    if (bool_literal_1 != nullptr && bool_literal_1 != nullptr)
        return std::make_shared<BoundBooleanLiteral>(bool_literal_1->token(), bool_literal_1->bool_value() && bool_literal_2->bool_value());
    return params[0];
}

INTRINSIC(or_bool_bool)
{
    auto bool_literal_1 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[0]);
    auto bool_literal_2 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[1]);
    if (bool_literal_1 != nullptr && bool_literal_1 != nullptr)
        return std::make_shared<BoundBooleanLiteral>(bool_literal_1->token(), bool_literal_1->bool_value() || bool_literal_2->bool_value());
    return params[0];
}

INTRINSIC(xor_bool_bool)
{
    auto bool_literal_1 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[0]);
    auto bool_literal_2 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[1]);
    if (bool_literal_1 != nullptr && bool_literal_1 != nullptr)
        return std::make_shared<BoundBooleanLiteral>(bool_literal_1->token(), bool_literal_1->bool_value() ^ bool_literal_2->bool_value());
    return params[0];
}

INTRINSIC(equals_bool_bool)
{
    auto bool_literal_1 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[0]);
    auto bool_literal_2 = std::dynamic_pointer_cast<BoundBooleanLiteral>(params[1]);
    if (bool_literal_1 != nullptr && bool_literal_1 != nullptr)
        return std::make_shared<BoundBooleanLiteral>(bool_literal_1->token(), !(bool_literal_1->bool_value() ^ bool_literal_2->bool_value()));
    return params[0];
}

INTRINSIC(add_str_str)
{
    auto literal_1 = std::dynamic_pointer_cast<BoundStringLiteral>(params[0]);
    auto literal_2 = std::dynamic_pointer_cast<BoundStringLiteral>(params[1]);
    if (literal_1 != nullptr && literal_1 != nullptr)
        return std::make_shared<BoundStringLiteral>(literal_1->token(), literal_1->string_value() + literal_2->string_value());
    return params[0];
}

INTRINSIC(greater_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(less_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(equals_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(multiply_str_int)
{
    fatal("Not implemented");
}

}
