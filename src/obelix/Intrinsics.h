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

using ARM64Implementation = std::function<ErrorOr<void>(ARM64Context&)>;
class IntrinsicDecl;
class FunctionCall;

#define INTRINSIC_SIGNATURE_ENUM(S) \
    S(sig_allocate)                 \
    S(sig_eputs)                    \
    S(sig_fputs)                    \
    S(sig_fsize)                    \
    S(sig_putchar)                  \
    S(sig_puts)                     \
    S(sig_to_string)                \
    S(sig_add_int_int)              \
    S(sig_subtract_int_int)         \
    S(sig_multiply_int_int)         \
    S(sig_divide_int_int)           \
    S(sig_equals_int_int)           \
    S(sig_greater_int_int)          \
    S(sig_less_int_int)             \
    S(sig_negate_int)               \
    S(sig_invert_int)               \
    S(sig_add_byte_byte)            \
    S(sig_subtract_byte_byte)       \
    S(sig_multiply_byte_byte)       \
    S(sig_divide_byte_byte)         \
    S(sig_equals_byte_byte)         \
    S(sig_greater_byte_byte)        \
    S(sig_less_byte_byte)           \
    S(sig_negate_byte)              \
    S(sig_invert_byte)              \
    S(sig_add_str_str)              \
    S(sig_multiply_str_int)         \
    S(sig_equals_str_str)           \
    S(sig_greater_str_str)          \
    S(sig_less_str_str)             \
    S(sig_and_bool_bool)            \
    S(sig_or_bool_bool)             \
    S(sig_xor_bool_bool)            \
    S(sig_invert_bool)              \
    S(sig_equals_bool_bool)

enum InitrinsicSignature {
#undef INTRINSIC_SIGNATURE
#define INTRINSIC_SIGNATURE(sig) sig,
    INTRINSIC_SIGNATURE_ENUM(INTRINSIC_SIGNATURE)
#undef INTRINSIC_SIGNATURE
        sig_intrinsic_count
};

struct Signature {
    std::string name;
    std::shared_ptr<ObjectType> return_type;
    ObjectTypes parameter_types;

    [[nodiscard]] bool operator==(Signature const& other) const
    {
        return name == other.name && return_type == other.return_type && parameter_types == other.parameter_types;
    }
};

struct Intrinsic {
    Intrinsic() = default;

    explicit Intrinsic(Signature const& s, ARM64Implementation impl = nullptr)
        : signature(s)
        , arm64_impl(move(impl))
    {
        Symbols parameters;
        for (auto ix = 0; ix < s.parameter_types.size(); ++ix)
            parameters.emplace_back(format("param_{}", ix), s.parameter_types[ix]);
        declaration = make_node<IntrinsicDecl>(Symbol { signature.name, signature.return_type }, parameters);
    }

    Signature signature;
    std::shared_ptr<IntrinsicDecl> declaration { nullptr };
    ARM64Implementation arm64_impl { nullptr };
};

class Intrinsics {
public:
    [[nodiscard]] static bool is_intrinsic(Signature const&);
    [[nodiscard]] static bool is_intrinsic(std::shared_ptr<FunctionCall> const&);
    static Intrinsic const& set_arm64_implementation(InitrinsicSignature, ARM64Implementation const&);
    static ARM64Implementation& get_arm64_implementation(Signature const&);
    static Intrinsic& get_intrinsic(Signature const&);
    static Intrinsic& get_intrinsic(std::shared_ptr<FunctionCall> const&);

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
