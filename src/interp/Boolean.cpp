/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
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

std::optional<Obj> Boolean::evaluate(std::string const& op, Ptr<Arguments> args) const
{
    if ((op == "!") || (op == "LogicalInvert")) {
        if (!args->empty()) {
            return make_obj<Exception>(ErrorCode::SyntaxError, format("Logical operation '{}' only takes a single operand", op));
        }
        return make_obj<Boolean>(!m_value);
    }
    if ((op == "||") || (op == "or") || op == "LogicalOr") {
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
    if ((op == "&&") || (op == "and") || op == "LogicalAnd") {
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
