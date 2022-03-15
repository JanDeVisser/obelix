/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Arguments.h>
#include <core/Object.h>

namespace Obelix {

String::String(std::string value)
    : Object(TypeString)
    , m_value(move(value))
{
}

int String::compare(Obj const& other) const
{
    return to_string().compare(other->to_string());
}

std::optional<Obj> String::evaluate(std::string const& op, Ptr<Arguments> args) const
{
    if (op == "+" || op == "Add") {
        auto ret = m_value;
        for (auto const& arg : args->arguments()) {
            ret += arg->to_string();
        }
        return make_obj<String>(ret);
    }
    if (op == "*" || op == "repeat" || op == "Multiply") {
        if (args->size() != 1) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("String operation '{}' requires exactly 2 operands", op));
        }
        std::string ret;
        auto& arg = args->at(0);
        auto int_maybe = arg->to_long();
        if (!int_maybe.has_value()) {
            return make_obj<Exception>(ErrorCode::TypeMismatch, op, "int", arg->type_name());
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
