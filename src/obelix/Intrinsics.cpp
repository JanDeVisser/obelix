/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <functional>

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Intrinsics.h>
#include <obelix/ARM64Intrinsics.h>
#include <obelix/Syntax.h>

namespace Obelix {

Intrinsic::Intrinsic(Signature s, ARM64Implementation impl)
    : signature(std::move(s))
    , arm64_impl(move(impl))
{
    auto identifier = std::make_shared<BoundIdentifier>(Token {}, signature.name, signature.return_type);
    BoundIdentifiers parameters;
    int ix = 1;
    for (auto& param_type : signature.parameter_types) {
        auto parameter = std::make_shared<BoundIdentifier>(Token {}, format("param_{}", ix++), param_type);
        parameters.push_back(parameter);
    }
    declaration = std::make_shared<BoundIntrinsicDecl>(identifier, parameters);
}

std::vector<Intrinsic> Intrinsics::s_intrinsics {};

bool Intrinsics::is_intrinsic(Signature const& signature)
{
    if (s_intrinsics.empty())
        initialize();
    return std::any_of(s_intrinsics.begin(), s_intrinsics.end(), [&signature](auto& intrinsic) {
        return signature == intrinsic.signature;
    });
}

bool Intrinsics::is_intrinsic(std::string const& name, ObjectTypes const& parameter_types)
{
    if (s_intrinsics.empty())
        initialize();
    return std::any_of(s_intrinsics.begin(), s_intrinsics.end(), [&name, &parameter_types](auto& intrinsic) {
        return name == intrinsic.signature.name && parameter_types == intrinsic.signature.parameter_types;
    });
}

bool Intrinsics::is_intrinsic(std::shared_ptr<BoundFunctionDecl> const& decl)
{
    return is_intrinsic(decl->name(), decl->parameter_types());
}

Intrinsic const& Intrinsics::set_arm64_implementation(IntrinsicType signature, ARM64Implementation const& arm64_implementation)
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
    fatal("Intrinsic with signature {} not found", signature.to_string());
}

ARM64Implementation& Intrinsics::get_arm64_implementation(std::shared_ptr<BoundFunctionDecl> const& decl)
{
    if (is_intrinsic(decl)) {
        return get_intrinsic(decl).arm64_impl;
    }
    fatal("Intrinsic with signature {} not found", decl);
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

Intrinsic& Intrinsics::get_intrinsic(std::string const& name, ObjectTypes const& parameter_types)
{
    if (s_intrinsics.empty())
        initialize();
    for (auto& intrinsic : s_intrinsics) {
        if (name == intrinsic.signature.name && parameter_types == intrinsic.signature.parameter_types) {
            return intrinsic;
        }
    }
    fatal("Not found");
}

Intrinsic& Intrinsics::get_intrinsic(std::shared_ptr<BoundFunctionDecl> const& decl)
{
    return get_intrinsic(decl->name(), decl->parameter_types());
}

Intrinsic& Intrinsics::get_intrinsic(IntrinsicType sig)
{
    if (s_intrinsics.empty())
        initialize();
    if (sig >= intrinsic_count)
        fatal("Invalid signature '{}'", (int)sig);
    return s_intrinsics[sig];
}

void Intrinsics::initialize()
{
    if (s_intrinsics.empty()) {
        s_intrinsics.resize(intrinsic_count);
        s_intrinsics[intrinsic_allocate] = Intrinsic(Signature { "allocate", ObjectType::get(TypePointer), { ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_eputs] = Intrinsic(Signature { "eputs", ObjectType::get(TypeInt), { ObjectType::get(TypeString) } });
        s_intrinsics[intrinsic_fputs] = Intrinsic(Signature { "fputs", ObjectType::get(TypeInt), { ObjectType::get(TypeInt), ObjectType::get(TypeString) } });
        s_intrinsics[intrinsic_fsize] = Intrinsic(Signature { "fsize", ObjectType::get(TypeInt), { ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_putchar] = Intrinsic(Signature { "putchar", ObjectType::get(TypeInt), { ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_puts] = Intrinsic(Signature { "puts", ObjectType::get(TypeInt), { ObjectType::get(TypeString) } });

        s_intrinsics[intrinsic_to_string] = Intrinsic(Signature { "to_string", ObjectType::get(TypeString), { ObjectType::get(TypeInt) } });

        s_intrinsics[intrinsic_ptr_math] = Intrinsic(Signature { "ptr_math", ObjectType::get(TypePointer), { ObjectType::get(TypePointer), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_dereference] = Intrinsic(Signature { UnaryOperator_name(UnaryOperator::Dereference), ObjectType::get(TypeAny), { ObjectType::get(TypePointer) } });

        s_intrinsics[intrinsic_add_int_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Add), ObjectType::get(TypeInt), { ObjectType::get(TypeInt), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_subtract_int_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Subtract), ObjectType::get(TypeInt), { ObjectType::get(TypeInt), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_multiply_int_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Multiply), ObjectType::get(TypeInt), { ObjectType::get(TypeInt), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_divide_int_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Divide), ObjectType::get(TypeInt), { ObjectType::get(TypeInt), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_equals_int_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Equals), ObjectType::get(TypeBoolean), { ObjectType::get(TypeInt), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_greater_int_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Greater), ObjectType::get(TypeBoolean), { ObjectType::get(TypeInt), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_less_int_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Less), ObjectType::get(TypeBoolean), { ObjectType::get(TypeInt), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_negate_int] = Intrinsic(Signature { UnaryOperator_name(UnaryOperator::Negate), ObjectType::get(TypeInt), { ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_invert_int] = Intrinsic(Signature { UnaryOperator_name(UnaryOperator::BitwiseInvert), ObjectType::get(TypeInt), { ObjectType::get(TypeInt) } });

        s_intrinsics[intrinsic_add_byte_byte] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Add), ObjectType::get(TypeByte), { ObjectType::get(TypeByte), ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_subtract_byte_byte] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Subtract), ObjectType::get(TypeByte), { ObjectType::get(TypeByte), ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_multiply_byte_byte] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Multiply), ObjectType::get(TypeByte), { ObjectType::get(TypeByte), ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_divide_byte_byte] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Divide), ObjectType::get(TypeByte), { ObjectType::get(TypeByte), ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_equals_byte_byte] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Equals), ObjectType::get(TypeBoolean), { ObjectType::get(TypeByte), ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_greater_byte_byte] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Greater), ObjectType::get(TypeBoolean), { ObjectType::get(TypeByte), ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_less_byte_byte] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Less), ObjectType::get(TypeBoolean), { ObjectType::get(TypeByte), ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_negate_byte] = Intrinsic(Signature { UnaryOperator_name(UnaryOperator::Negate), ObjectType::get(TypeByte), { ObjectType::get(TypeByte) } });
        s_intrinsics[intrinsic_invert_byte] = Intrinsic(Signature { UnaryOperator_name(UnaryOperator::BitwiseInvert), ObjectType::get(TypeByte), { ObjectType::get(TypeByte) } });

        s_intrinsics[intrinsic_add_str_str] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Add), ObjectType::get(TypeString), { ObjectType::get(TypeString), ObjectType::get(TypeString) } });
        s_intrinsics[intrinsic_multiply_str_int] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Multiply), ObjectType::get(TypeString), { ObjectType::get(TypeString), ObjectType::get(TypeInt) } });
        s_intrinsics[intrinsic_equals_str_str] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Equals), ObjectType::get(TypeBoolean), { ObjectType::get(TypeString), ObjectType::get(TypeString) } });
        s_intrinsics[intrinsic_greater_str_str] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Greater), ObjectType::get(TypeBoolean), { ObjectType::get(TypeString), ObjectType::get(TypeString) } });
        s_intrinsics[intrinsic_less_str_str] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Less), ObjectType::get(TypeBoolean), { ObjectType::get(TypeString), ObjectType::get(TypeString) } });

        s_intrinsics[intrinsic_and_bool_bool] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::LogicalAnd), ObjectType::get(TypeBoolean), { ObjectType::get(TypeBoolean), ObjectType::get(TypeBoolean) } });
        s_intrinsics[intrinsic_or_bool_bool] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::LogicalOr), ObjectType::get(TypeBoolean), { ObjectType::get(TypeBoolean), ObjectType::get(TypeBoolean) } });
        s_intrinsics[intrinsic_xor_bool_bool] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::BitwiseXor), ObjectType::get(TypeBoolean), { ObjectType::get(TypeBoolean), ObjectType::get(TypeBoolean) } });
        s_intrinsics[intrinsic_invert_bool] = Intrinsic(Signature { UnaryOperator_name(UnaryOperator::LogicalInvert), ObjectType::get(TypeBoolean), { ObjectType::get(TypeBoolean) } });
        s_intrinsics[intrinsic_equals_bool_bool] = Intrinsic(Signature { BinaryOperator_name(BinaryOperator::Equals), ObjectType::get(TypeBoolean), { ObjectType::get(TypeBoolean), ObjectType::get(TypeBoolean) } });

#undef INTRINSIC_TYPE
#define INTRINSIC_TYPE(intrinsic) set_arm64_implementation(intrinsic, arm64_##intrinsic);
        INTRINSIC_TYPE_ENUM(INTRINSIC_TYPE)
#undef INTRINSIC_TYPE
    }
}

}
