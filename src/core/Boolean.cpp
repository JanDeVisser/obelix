/*
 * Boolean.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

Boolean::Boolean(bool value)
    : Object(TypeBoolean)
    , m_value(value)
{
}

int Boolean::compare(Obj const& other) const
{
    auto long_maybe = other->to_long();
    if (!long_maybe.has_value())
        return 1;
    return (int)to_long().value() - long_maybe.value();
}

std::optional<Obj> Boolean::evaluate(std::string const& op, Ptr<Arguments> args)
{
    if ((op == "!") || (op == "negate")) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' only takes a single operand", op));
        }
        return make_obj<Boolean>(!m_value);
    }
    if ((op == "||") || (op == "or")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' requires at least 2 operands", op));
        }
        if (m_value)
            return to_obj(True());
        for (auto const& arg : args->arguments()) {
            auto bool_maybe = arg->to_bool();
            if (!bool_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "bool", arg->type_name());
            }
            if (bool_maybe.value())
                return to_obj(True());
        }
        return to_obj(False());
    }
    if ((op == "&&") || (op == "and")) {
        if (args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' requires at least 2 operands", op));
        }
        if (!m_value)
            return to_obj(False());
        for (auto const& arg : args->arguments()) {
            auto bool_maybe = arg->to_bool();
            if (!bool_maybe.has_value()) {
                return make_obj<Exception>(ErrorCode::TypeMismatch, op, "bool", arg->type_name());
            }
            if (!bool_maybe.value())
                return to_obj(False());
        }
        return to_obj(True());
    }
    return Object::evaluate(op, args);
}

Ptr<Boolean> const& Boolean::True()
{
    static Ptr<Boolean> s_true = make_typed<Boolean>(true);
    return s_true;
}

Ptr<Boolean> const& Boolean::False()
{
    static Ptr<Boolean> s_false = make_typed<Boolean>(false);
    return s_false;
}

}
