/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <memory>

#include <core/Type.h>
#include <obelix/ARM64Context.h>

namespace Obelix {

class BoundFunctionDecl;
class BoundIntrinsicDecl;

using ARM64Implementation = std::function<ErrorOr<void>(ARM64Context&)>;
class IntrinsicDecl;
class FunctionCall;

#define INTRINSIC_TYPE_ENUM(S)      \
    S(intrinsic_allocate)           \
    S(intrinsic_eputs)              \
    S(intrinsic_fputs)              \
    S(intrinsic_fsize)              \
    S(intrinsic_putchar)            \
    S(intrinsic_puts)               \
    S(intrinsic_to_string)          \
    S(intrinsic_ptr_math)           \
    S(intrinsic_dereference)        \
    S(intrinsic_add_int_int)        \
    S(intrinsic_subtract_int_int)   \
    S(intrinsic_multiply_int_int)   \
    S(intrinsic_divide_int_int)     \
    S(intrinsic_equals_int_int)     \
    S(intrinsic_greater_int_int)    \
    S(intrinsic_less_int_int)       \
    S(intrinsic_negate_int)         \
    S(intrinsic_invert_int)         \
    S(intrinsic_add_byte_byte)      \
    S(intrinsic_subtract_byte_byte) \
    S(intrinsic_multiply_byte_byte) \
    S(intrinsic_divide_byte_byte)   \
    S(intrinsic_equals_byte_byte)   \
    S(intrinsic_greater_byte_byte)  \
    S(intrinsic_less_byte_byte)     \
    S(intrinsic_negate_byte)        \
    S(intrinsic_invert_byte)        \
    S(intrinsic_add_str_str)        \
    S(intrinsic_multiply_str_int)   \
    S(intrinsic_equals_str_str)     \
    S(intrinsic_greater_str_str)    \
    S(intrinsic_less_str_str)       \
    S(intrinsic_and_bool_bool)      \
    S(intrinsic_or_bool_bool)       \
    S(intrinsic_xor_bool_bool)      \
    S(intrinsic_invert_bool)        \
    S(intrinsic_equals_bool_bool)

enum IntrinsicType {
#undef INTRINSIC_TYPE
#define INTRINSIC_TYPE(intrinsic) intrinsic,
    INTRINSIC_TYPE_ENUM(INTRINSIC_TYPE)
#undef INTRINSIC_TYPE
        intrinsic_count
};

template<>
struct Converter<IntrinsicType> {
    static std::string to_string(IntrinsicType val)
    {
        switch (val) {
#undef INTRINSIC_TYPE
#define INTRINSIC_TYPE(intrinsic)  \
    case IntrinsicType::intrinsic: \
        return #intrinsic;
            INTRINSIC_TYPE_ENUM(INTRINSIC_TYPE)
#undef INTRINSIC_TYPE
        default:
            fatal("Invalid IntrinsicType value {}", (int)val);
        }
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

struct Signature {
    std::string name;
    std::shared_ptr<ObjectType> return_type;
    ObjectTypes parameter_types;

    [[nodiscard]] bool operator==(Signature const& other) const
    {
        return name == other.name && return_type == other.return_type && parameter_types == other.parameter_types;
    }

    [[nodiscard]] std::string to_string() const
    {
        std::string ret = name + "(";
        std::string glue;
        for (auto const& param : parameter_types) {
            ret += glue;
            ret += param->to_string();
            glue = ", ";
        }
        ret += "): ";
        return ret + return_type->to_string();
    }
};

struct Intrinsic {
    Intrinsic() = default;

    explicit Intrinsic(Signature, ARM64Implementation impl = nullptr);

    Signature signature;
    std::shared_ptr<BoundIntrinsicDecl> declaration { nullptr };
    ARM64Implementation arm64_impl { nullptr };
};

class Intrinsics {
public:
    [[nodiscard]] static bool is_intrinsic(Signature const&);
    [[nodiscard]] static bool is_intrinsic(std::string const&, ObjectTypes const& param_type);
    [[nodiscard]] static bool is_intrinsic(std::shared_ptr<BoundFunctionDecl> const&);
    static Intrinsic const& set_arm64_implementation(IntrinsicType, ARM64Implementation const&);
    static ARM64Implementation& get_arm64_implementation(Signature const&);
    static ARM64Implementation& get_arm64_implementation(std::shared_ptr<BoundFunctionDecl> const&);
    static Intrinsic& get_intrinsic(Signature const&);
    static Intrinsic& get_intrinsic(std::string const&, ObjectTypes const& param_type);
    static Intrinsic& get_intrinsic(IntrinsicType);
    static Intrinsic& get_intrinsic(std::shared_ptr<BoundFunctionDecl> const&);

private:
    static void initialize();
    static std::vector<Intrinsic> s_intrinsics;
};

}

template<>
struct std::hash<Obelix::Signature> {
    static size_t type_hash(Obelix::ObelixType t)
    {
        return std::hash<int> {}(t);
    }

    auto operator()(Obelix::Signature const& signature) const noexcept
    {
        auto h = std::hash<std::string> {}(signature.name);
        h ^= type_hash(signature.return_type->type()) << 1;
        for (auto& parameter_type : signature.parameter_types) {
            h ^= type_hash(parameter_type->type()) << 1;
        }
        return h;
    }
};
