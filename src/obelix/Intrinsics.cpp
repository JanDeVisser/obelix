/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <functional>

#include <obelix/Intrinsics.h>
#include <obelix/ARM64Intrinsics.h>
#include <obelix/Syntax.h>

namespace Obelix {

std::vector<Intrinsic> Intrinsics::s_intrinsics {};

bool Intrinsics::is_intrinsic(Signature const& signature)
{
    if (s_intrinsics.empty())
        initialize();
    return std::any_of(s_intrinsics.begin(), s_intrinsics.end(), [&signature](auto& intrinsic) {
        return signature == intrinsic.signature;
    });
}

bool Intrinsics::is_intrinsic(std::shared_ptr<FunctionCall> const& call)
{
    return is_intrinsic(Signature { call->name(), call->type(), call->argument_types() });
}

Intrinsic const& Intrinsics::set_arm64_implementation(InitrinsicSignature signature, ARM64Implementation const& arm64_implementation)
{
    if (s_intrinsics.empty())
        initialize();
    assert(s_intrinsics.size() > signature);
    s_intrinsics[signature].arm64_impl = arm64_implementation;
    return s_intrinsics[signature];
}

ARM64Implementation& Intrinsics::get_arm64_implementation(Signature const& signature)
{
    if (s_intrinsics.empty())
        initialize();
    for (auto& intrinsic : s_intrinsics) {
        if (signature == intrinsic.signature) {
            return intrinsic.arm64_impl;
        }
    }
    fatal("Unreachable");
}

Intrinsic& Intrinsics::get_intrinsic(Signature const& signature)
{
    if (s_intrinsics.empty())
        initialize();
    for (auto& intrinsic : s_intrinsics) {
        if (signature == intrinsic.signature) {
            return intrinsic;
        }
    }
    fatal("Not found");
}

Intrinsic& Intrinsics::get_intrinsic(std::shared_ptr<FunctionCall> const& call)
{
    return get_intrinsic(Signature { call->name(), call->type(), call->argument_types() });
}

void Intrinsics::initialize()
{
    if (s_intrinsics.empty()) {
        s_intrinsics.resize(sig_intrinsic_count);
        s_intrinsics[sig_allocate] = Intrinsic(
            Signature {
                "allocate",
                ExpressionType::simple_type(TypePointer),
                { ExpressionType::simple_type(TypeInt) } });

        s_intrinsics[sig_eputs] = Intrinsic(Signature { "eputs", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeString) } });
        s_intrinsics[sig_fputs] = Intrinsic(Signature { "fputs", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeString) } });
        s_intrinsics[sig_fsize] = Intrinsic(Signature { "fsize", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_putchar] = Intrinsic(Signature { "putchar", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_puts] = Intrinsic(Signature { "puts", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeString) } });

        s_intrinsics[sig_to_string] = Intrinsic(Signature { "to_string", ExpressionType::simple_type(TypeString), { ExpressionType::simple_type(TypeInt) } });

        s_intrinsics[sig_add_int_int] = Intrinsic(Signature { "+", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_subtract_int_int] = Intrinsic(Signature { "-", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_multiply_int_int] = Intrinsic(Signature { "*", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_divide_int_int] = Intrinsic(Signature { "/", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_equals_int_int] = Intrinsic(Signature { "==", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_greater_int_int] = Intrinsic(Signature { ">", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_less_int_int] = Intrinsic(Signature { "<", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeInt), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_negate_int] = Intrinsic(Signature { "-", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_invert_int] = Intrinsic(Signature { "~", ExpressionType::simple_type(TypeInt), { ExpressionType::simple_type(TypeInt) } });

        s_intrinsics[sig_add_byte_byte] = Intrinsic(Signature { "+", ExpressionType::simple_type(TypeByte), { ExpressionType::simple_type(TypeByte), ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_subtract_byte_byte] = Intrinsic(Signature { "-", ExpressionType::simple_type(TypeByte), { ExpressionType::simple_type(TypeByte), ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_multiply_byte_byte] = Intrinsic(Signature { "*", ExpressionType::simple_type(TypeByte), { ExpressionType::simple_type(TypeByte), ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_divide_byte_byte] = Intrinsic(Signature { "/", ExpressionType::simple_type(TypeByte), { ExpressionType::simple_type(TypeByte), ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_equals_byte_byte] = Intrinsic(Signature { "==", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeByte), ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_greater_byte_byte] = Intrinsic(Signature { ">", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeByte), ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_less_byte_byte] = Intrinsic(Signature { "<", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeByte), ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_negate_byte] = Intrinsic(Signature { "-", ExpressionType::simple_type(TypeByte), { ExpressionType::simple_type(TypeByte) } });
        s_intrinsics[sig_invert_byte] = Intrinsic(Signature { "~", ExpressionType::simple_type(TypeByte), { ExpressionType::simple_type(TypeByte) } });

        s_intrinsics[sig_add_str_str] = Intrinsic(Signature { "+", ExpressionType::simple_type(TypeString), { ExpressionType::simple_type(TypeString), ExpressionType::simple_type(TypeString) } });
        s_intrinsics[sig_multiply_str_int] = Intrinsic(Signature { "*", ExpressionType::simple_type(TypeString), { ExpressionType::simple_type(TypeString), ExpressionType::simple_type(TypeInt) } });
        s_intrinsics[sig_equals_str_str] = Intrinsic(Signature { "==", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeString), ExpressionType::simple_type(TypeString) } });
        s_intrinsics[sig_greater_str_str] = Intrinsic(Signature { ">", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeString), ExpressionType::simple_type(TypeString) } });
        s_intrinsics[sig_less_str_str] = Intrinsic(Signature { "<", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeString), ExpressionType::simple_type(TypeString) } });

        s_intrinsics[sig_and_bool_bool] = Intrinsic(Signature { "&", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeBoolean), ExpressionType::simple_type(TypeBoolean) } });
        s_intrinsics[sig_or_bool_bool] = Intrinsic(Signature { "|", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeBoolean), ExpressionType::simple_type(TypeBoolean) } });
        s_intrinsics[sig_xor_bool_bool] = Intrinsic(Signature { "^", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeBoolean), ExpressionType::simple_type(TypeBoolean) } });
        s_intrinsics[sig_invert_bool] = Intrinsic(Signature { "!", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeBoolean) } });
        s_intrinsics[sig_equals_bool_bool] = Intrinsic(Signature { "==", ExpressionType::simple_type(TypeBoolean), { ExpressionType::simple_type(TypeBoolean), ExpressionType::simple_type(TypeBoolean) } });

#undef INTRINSIC_SIGNATURE
#define INTRINSIC_SIGNATURE(sig) set_arm64_implementation(sig, arm64_##sig);
        INTRINSIC_SIGNATURE_ENUM(INTRINSIC_SIGNATURE)
#undef INTRINSIC_SIGNATURE
    }
}

}
