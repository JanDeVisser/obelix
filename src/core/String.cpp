/*
 * String.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

String::String(std::string value)
    : Object("string")
    , m_value(move(value))
{
}

int String::compare(Obj const& other) const
{
    return to_string().compare(other->to_string());
}

std::optional<Obj> String::evaluate(std::string const& op, Ptr<Arguments> args)
{
    if (op == "+") {
        auto ret = m_value;
        for (auto const& arg : args->arguments()) {
            ret += arg->to_string();
        }
        return make_obj<String>(ret);
    }
    if ((op == "*") || (op == "repeat")) {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("String operation '{}' requires exactly 2 operands", op));
        }
        std::string ret;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type());
        }
        if (int_maybe.value() < 0) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Repeat count of string operation '{}' cannot be negative", op));
        }
        for (auto ix = 0u; ix < int_maybe.value(); ix++) {
            ret += m_value;
        }
        return make_obj<String>(ret);
    }
    return Object::evaluate(op, args);
}

}