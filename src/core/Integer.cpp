/*
 * Integer.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/Arguments.h>
#include <core/Object.h>

namespace Obelix {

Integer::Integer(int value)
    : Object("integer")
    , m_value(value)
{
}

int Integer::compare(Obj const& other) const
{
    auto long_maybe = other->to_long();
    if (!long_maybe.has_value())
        return 1;
    return (int)m_value - long_maybe.value();
}

std::optional<Obj> Integer::evaluate(std::string const& op, Ptr<Arguments> args)
{
    if ((op == "+") || (op == "add")) {
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
            }
            ret += int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "!") || (op == "negate")) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' only takes a single operand", op));
        }
        return make_obj<Boolean>(!(to_bool().value()));
    }
    if ((op == "~") || (op == "invert")) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' only takes a single operand", op));
        }
        return make_obj<Integer>(~m_value);
    }
    if ((op == "-") || (op == "sub")) {
        long ret = m_value;
        if (args->arguments()->empty()) {
            ret = -ret;
        } else {
            for (auto& arg : args->arguments()) {
                auto int_maybe = arg->to_long();
                if (!int_maybe.has_value()) {
                    return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type().c_str());
                }
                ret -= int_maybe.value();
            }
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "*") || (op == "mult")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op.c_str(), "int", arg->type().c_str());
            }
            ret *= int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "/") || (op == "div")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        for (auto& arg : args->arguments()) {
            auto int_maybe = arg->to_long();
            if (!int_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
            }
            ret /= int_maybe.value();
        }
        return make_obj<Integer>(ret);
    }
    if ((op == "%") || (op == "mod")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Arithmetical operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret %= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "<<") || (op == "shl")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret <<= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "<<") || (op == "shr")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires exactly 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret >>= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "|") || (op == "bitwise_or")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret |= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "&") || (op == "bitwise_and")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret &= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    if ((op == "^") || (op == "bitwise_xor")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Bitwise operation '{}' requires at least 2 operands", op));
        }
        long ret = m_value;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        ret ^= int_maybe.value();
        return make_obj<Integer>(ret);
    }
    return Object::evaluate(op, args);
}

}