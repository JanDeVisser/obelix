/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <optional>

#include <core/Format.h>
#include <core/Logging.h>

namespace Obelix {

/*
 * Precendeces according to https://en.cppreference.com/w/c/language/operator_precedence
 */
#define ENUMERATE_BINARY_OPERATORS(S) \
    S(Invalid, false, -1)             \
    S(Add, false, 11)                 \
    S(Subtract, false, 11)            \
    S(Multiply, false, 12)            \
    S(Divide, false, 12)              \
    S(Modulo, false, 12)              \
    S(Assign, true, 1)                \
    S(Equals, false, 8)               \
    S(NotEquals, false, 8)            \
    S(GreaterEquals, false, 9)        \
    S(LessEquals, false, 9)           \
    S(Greater, false, 9)              \
    S(Less, false, 9)                 \
    S(LogicalAnd, false, 4)           \
    S(LogicalOr, false, 3)            \
    S(BitwiseAnd, false, 7)           \
    S(BitwiseOr, false, 5)            \
    S(BitwiseXor, false, 6)           \
    S(BinaryIncrement, true, 1)       \
    S(BinaryDecrement, true, 1)       \
    S(MemberAccess, false, 14)        \
    S(BitShiftLeft, false, 10)        \
    S(BitShiftRight, false, 10)       \
    S(AssignShiftLeft, true, 1)       \
    S(AssignShiftRight, true, 1)      \
    S(AssignBitwiseAnd, true, 1)      \
    S(AssignBitwiseOr, true, 1)       \
    S(AssignBitwiseXor, true, 1)      \
    S(Range, false, 8)                \
    S(Subscript, false, 14)

#define ENUMERATE_UNARY_OPERATORS(S) \
    S(InvalidUnary)                  \
    S(Identity)                      \
    S(Negate)                        \
    S(UnaryIncrement)                \
    S(UnaryDecrement)                \
    S(LogicalInvert)                 \
    S(BitwiseInvert)                 \
    S(Dereference)                   \
    S(AddressOf)

enum class Operator {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) op,
    ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) op,
        ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
};

constexpr char const* Operator_name(Operator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) \
    case Operator::op:              \
        return #op;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) \
    case Operator::op:       \
        return #op;
        ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
    default:
        fatal("Unknowm Operator '{}'", (int) op);
    }
}

template<>
struct Converter<Operator> {
    static std::string to_string(Operator val)
    {
        return Operator_name(val);
    }

    static double to_double(Operator val)
    {
        return static_cast<double>(val);
    }

    static long to_long(Operator val)
    {
        return static_cast<long>(val);
    }
};

enum class BinaryOperator {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) op,
    ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
};

constexpr Operator to_operator(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) \
    case BinaryOperator::op:        \
        return Operator::op;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    }
    return Operator::Invalid;
}

#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) constexpr char const* Binary_##op = #op;
ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR

constexpr char const* BinaryOperator_name(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, p) \
    case BinaryOperator::op:        \
        return #op;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    default:
        fatal("Unknowm BinaryOperator '{}'", (int) op);
    }
}

constexpr bool BinaryOperator_is_assignment(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, assignment_op, p) \
    case BinaryOperator::op:                    \
        return assignment_op;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    default:
        fatal("Unknowm BinaryOperator '{}'", (int) op);
    }
}

constexpr BinaryOperator BinaryOperator_for_assignment_operator(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::BinaryIncrement:
        return BinaryOperator::Add;
    case BinaryOperator::BinaryDecrement:
        return BinaryOperator::Subtract;
    case BinaryOperator::AssignShiftLeft:
        return BinaryOperator::BitShiftLeft;
    case BinaryOperator::AssignShiftRight:
        return BinaryOperator::BitShiftRight;
    case BinaryOperator::AssignBitwiseAnd:
        return BinaryOperator::BitwiseAnd;
    case BinaryOperator::AssignBitwiseOr:
        return BinaryOperator::BitwiseOr;
    case BinaryOperator::AssignBitwiseXor:
        return BinaryOperator::BitwiseXor;
    default:
        return op;
    }
}

constexpr int BinaryOperator_precedence(BinaryOperator op)
{
    switch (op) {
#undef __BINARY_OPERATOR
#define __BINARY_OPERATOR(op, a, precedence) \
    case BinaryOperator::op:                 \
        return precedence;
        ENUMERATE_BINARY_OPERATORS(__BINARY_OPERATOR)
#undef __BINARY_OPERATOR
    default:
        fatal("Unknowm BinaryOperator '{}'", (int) op);
    }
}

template<>
struct Converter<BinaryOperator> {
    static std::string to_string(BinaryOperator val)
    {
        return BinaryOperator_name(val);
    }

    static double to_double(BinaryOperator val)
    {
        return static_cast<double>(val);
    }

    static long to_long(BinaryOperator val)
    {
        return static_cast<long>(val);
    }
};

enum class UnaryOperator {
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) op,
    ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
};

#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) constexpr char const* Unary_##op = #op;
ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR

constexpr Operator to_operator(UnaryOperator op)
{
    switch (op) {
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) \
    case UnaryOperator::op:  \
        return Operator::op;
        ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
    }
    return Operator::Invalid;
}

constexpr char const* UnaryOperator_name(UnaryOperator op)
{
    switch (op) {
#undef __UNARY_OPERATOR
#define __UNARY_OPERATOR(op) \
    case UnaryOperator::op:  \
        return #op;
        ENUMERATE_UNARY_OPERATORS(__UNARY_OPERATOR)
#undef __UNARY_OPERATOR
    default:
        fatal("Unknowm UnaryOperator '{}'", (int) op);
    }
}

template<>
struct Converter<UnaryOperator> {
    static std::string to_string(UnaryOperator val)
    {
        return UnaryOperator_name(val);
    }

    static double to_double(UnaryOperator val)
    {
        return static_cast<double>(val);
    }

    static long to_long(UnaryOperator val)
    {
        return static_cast<long>(val);
    }
};

}
