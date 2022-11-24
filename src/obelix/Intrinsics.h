/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <array>
#include <functional>

#include <core/Logging.h>
#include <obelix/Architecture.h>
#include <obelix/Context.h>
#include <vector>

namespace Obelix {

#define INTRINSIC_TYPE_ENUM(S) \
    S(NotIntrinsic)            \
    S(allocate)                \
    S(ok)                      \
    S(error)                   \
    S(eputs)                   \
    S(fputs)                   \
    S(fsize)                   \
    S(putchar)                 \
    S(int_to_string)           \
    S(ptr_math)                \
    S(dereference)             \
    S(add_int_int)             \
    S(subtract_int_int)        \
    S(multiply_int_int)        \
    S(divide_int_int)          \
    S(bitwise_or_int_int)      \
    S(bitwise_and_int_int)     \
    S(bitwise_xor_int_int)     \
    S(shl_int)                 \
    S(shr_int)                 \
    S(equals_int_int)          \
    S(greater_int_int)         \
    S(less_int_int)            \
    S(negate_s64)              \
    S(negate_s32)              \
    S(negate_s16)              \
    S(negate_s8)               \
    S(invert_int)              \
    S(add_byte_byte)           \
    S(subtract_byte_byte)      \
    S(multiply_byte_byte)      \
    S(divide_byte_byte)        \
    S(equals_byte_byte)        \
    S(greater_byte_byte)       \
    S(less_byte_byte)          \
    S(negate_byte)             \
    S(invert_byte)             \
    S(add_str_str)             \
    S(multiply_str_int)        \
    S(equals_str_str)          \
    S(greater_str_str)         \
    S(less_str_str)            \
    S(and_bool_bool)           \
    S(or_bool_bool)            \
    S(xor_bool_bool)           \
    S(invert_bool)             \
    S(equals_bool_bool)        \
    S(enum_text_value)         \
    S(free_str)

enum IntrinsicType {
#undef INTRINSIC_TYPE
#define INTRINSIC_TYPE(intrinsic) intrinsic,
    INTRINSIC_TYPE_ENUM(INTRINSIC_TYPE)
#undef INTRINSIC_TYPE
        count
};

constexpr const char* IntrinsicType_name(IntrinsicType type)
{
    switch (type) {
#undef INTRINSIC_TYPE
#define INTRINSIC_TYPE(intrinsic)  \
    case IntrinsicType::intrinsic: \
        return #intrinsic;
        INTRINSIC_TYPE_ENUM(INTRINSIC_TYPE)
#undef INTRINSIC_TYPE
    default:
        fatal("Invalid IntrinsicType value {}", (int)type);
    }
}

inline IntrinsicType IntrinsicType_by_name(const char* type)
{
#undef INTRINSIC_TYPE
#define INTRINSIC_TYPE(intrinsic)  \
    if (!strcmp(type, #intrinsic)) \
        return IntrinsicType::intrinsic;
    INTRINSIC_TYPE_ENUM(INTRINSIC_TYPE)
#undef INTRINSIC_TYPE
    log_error("Invalid IntrinsicType {}", type);
    return IntrinsicType::NotIntrinsic;
}

inline IntrinsicType IntrinsicType_by_name(std::string const& type)
{
    return IntrinsicType_by_name(type.c_str());
}

template<>
struct Converter<IntrinsicType> {
    static std::string to_string(IntrinsicType val)
    {
        return IntrinsicType_name(val);
    }

    static double to_double(IntrinsicType val)
    {
        return static_cast<double>(val);
    }

    static unsigned long to_long(IntrinsicType val)
    {
        return static_cast<unsigned long>(val);
    }
};

}
