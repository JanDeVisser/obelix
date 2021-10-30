/*
 * BoundFunction.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <obelix/BoundFunction.h>
#include <obelix/Syntax.h>

namespace Obelix {

BoundFunction::BoundFunction(Ptr<Scope> scope, FunctionDef const& definition)
    : Object("boundfunction")
    , m_scope(make_typed<Scope>(scope))
    , m_definition(definition)
{
}

Obj BoundFunction::call(Ptr<Arguments> args)
{
    return call(m_definition.name(), std::move(args));
}

Obj BoundFunction::call(std::string const& name, Ptr<Arguments> args)
{
    assert(args->size() == m_definition.parameters().size());
    Ptr<Scope> function_scope = m_scope->clone();
    for (auto ix = 0u; ix < args->size(); ix++) {
        function_scope->declare(m_definition.parameters()[ix], args->at(ix));
    }
    auto result = m_definition.execute_block(function_scope);
    Obj return_value;
    switch (result.code) {
    case ExecutionResultCode::None:
        // All good
        break;
    case ExecutionResultCode::Return:
        return_value = result.return_value;
        break;
    case ExecutionResultCode::Continue:
    case ExecutionResultCode::Break:
        return_value = make_obj<Exception>(ErrorCode::SyntaxError, "Encountered 'break' or 'continue' without enclosing loop");
        break;
    case ExecutionResultCode::Skipped:
        return_value = make_obj<Exception>(ErrorCode::SyntaxError, "Unhandled 'Skipped' execution result");
    case ExecutionResultCode::Error:
        return_value = make_obj<Exception>(ErrorCode::SyntaxError, result.return_value.to_string());
        break;
    }
    return return_value;
}

}