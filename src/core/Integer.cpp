/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Arguments.h>
#include <core/Object.h>

namespace Obelix {

Integer::Integer(int value)
    : Object(ObelixType::TypeInt)
    , m_value(value)
{
}

int Integer::compare(Obj const& other) const
{
    auto long_maybe = other->to_long();
    if (!long_maybe.has_value())
        return 1;
    return (int)(m_value - long_maybe.value());
}

std::optional<Obj> Integer::evaluate(std::string const& op, Ptr<Arguments> args) const
{
    if (op == "+" || op == UnaryOperator_name(UnaryOperator::Identity) || op == BinaryOperator_name(BinaryOperator::Add)) {
        long ret = m_value;
        for (auto const& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
            }
            ret += int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "!") || (op == UnaryOperator_name(UnaryOperator::LogicalInvert))) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' only takes a single operand", op));
        }
        return make_obj<Boolean>(!(to_bool().value()));
    }
    if ((op == "~") || (op == UnaryOperator_name(UnaryOperator::BitwiseInvert))) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' only takes a single operand", op));
        }
        return make_obj<Integer>(~m_value);
    }
    if (op == "-" || op == BinaryOperator_name(BinaryOperator::Subtract) || op == UnaryOperator_name(UnaryOperator::Negate)) {
        long ret = m_value;
        if (args->arguments()->empty()) {
            ret = -ret;
        } else {
            for (auto const& arg : args->arguments()) {
                auto int_maybe = arg->to_long();
                if (!int_maybe.has_value()) {
                    return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type_name());
                }
                ret -= int_maybe.value();
            }
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "*") || (op == BinaryOperator_name(BinaryOperator::Multiply))) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        for (auto const& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type_name());
            }
            ret *= int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "/") || op == BinaryOperator_name(BinaryOperator::Divide)) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        for (auto const& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
            }
            ret /= int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "%") || op == BinaryOperator_name(BinaryOperator::Modulo)) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
        }
        ret %= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "<<") || (op == BinaryOperator_name(BinaryOperator::BitShiftLeft))) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
        }
        ret <<= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == ">>") || (op == BinaryOperator_name(BinaryOperator::BitShiftRight))) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
        }
        ret >>= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "|") || (op == BinaryOperator_name(BinaryOperator::BitwiseOr))) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
        }
        ret |= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "&") || (op == BinaryOperator_name(BinaryOperator::BitwiseAnd))) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
        }
        ret &= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "^") || (op == BinaryOperator_name(BinaryOperator::BitwiseXor))) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
        }
        ret ^= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    return Object::evaluate(op, args);
}

}
