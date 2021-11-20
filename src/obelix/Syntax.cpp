/*
 * Syntax.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/Logging.h>
#include <obelix/BoundFunction.h>
#include <obelix/Parser.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

ExecutionResult Import::execute(Ptr<Scope>& scope)
{
    auto module_scope = scope->import_module(m_name);
    scope->declare(m_name, to_obj(module_scope));
    return {};
}

ExecutionResult FunctionDef::execute(Ptr<Scope>& scope)
{
    auto bound_function = make_obj<BoundFunction>(scope, *this);
    scope->declare(name(), bound_function);
    return { ExecutionResultCode::None, bound_function };
}

Obj BinaryExpression::evaluate(Ptr<Scope>& scope)
{
    Obj rhs = m_rhs->evaluate(scope);
    switch (m_operator.code()) {
    case TokenCode::Equals:
    case Parser::KeywordIncEquals:
    case Parser::KeywordDecEquals:
        if (auto ret_maybe = m_lhs->assign(scope, m_operator, rhs); ret_maybe.has_value())
            return ret_maybe.value();
        return make_obj<Exception>(ErrorCode::SyntaxError, "Could not assign to non-lvalue");
    default: {
        Obj lhs = m_lhs->evaluate(scope);
        auto ret_maybe = lhs->evaluate(m_operator.value(), make_typed<Arguments>(rhs));
        if (!ret_maybe.has_value())
            return make_obj<Exception>(ErrorCode::OperatorUnresolved, m_operator.value(), lhs);
        return ret_maybe.value();
    }
    }
}

std::optional<Obj> BinaryExpression::assign(Ptr<Scope>& scope, Token const& op, Obj const& value)
{
    Obj lhs = m_lhs->evaluate(scope);
    if (lhs->is_exception())
        return lhs;
    Obj rhs = m_rhs->evaluate(scope);
    if (rhs->is_exception())
        return rhs;
    auto result = lhs->evaluate(op.value(), make_typed<Arguments>(rhs->to_string(), value));
    if (!result.has_value())
        return make_obj<Exception>(ErrorCode::CannotAssignToObject, lhs->to_string());
    return result.value();
}

}
