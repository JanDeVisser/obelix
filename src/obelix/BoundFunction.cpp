/*
 * BoundFunction.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Arguments.h>
#include <obelix/BoundFunction.h>
#include <obelix/Syntax.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

BoundFunction::BoundFunction(Context<Obj>& enclosing_scope, FunctionDef definition)
    : Object(TypeBoundFunction)
    , m_enclosing_scope(enclosing_scope)
    , m_definition(std::move(definition))
{
}

Obj BoundFunction::call(Ptr<Arguments> args)
{
    return call(m_definition.name(), std::move(args));
}

Obj BoundFunction::call(std::string const& name, Ptr<Arguments> args)
{
    if (args->size() != m_definition.parameters().size()) {
        return make_obj<Exception>(ErrorCode::SyntaxError,
            format("Function {}: argument count mismatch: expected {}, got {}", m_definition.name(), m_definition.parameters().size(), args->size()));
    }
    Context<Obj> function_ctx = Context<Obj>(m_enclosing_scope);
    for (auto ix = 0u; ix < args->size(); ix++) {
        function_ctx.declare(m_definition.parameters()[ix].identifier(), args->at(ix));
    }
    auto result = TRY_OR_EXCEPTION(execute(m_definition.statement(),function_ctx));
    switch (result->node_type()) {
    case SyntaxNodeType::StatementExecutionResult: {
        auto execution_result = std::dynamic_pointer_cast<StatementExecutionResult>(result);
        switch (execution_result->flow_control()) {
        case FlowControl::None:
        case FlowControl::Return:
            return execution_result->result();
        default:
            return make_obj<Exception>(ErrorCode::SyntaxError, "Function call returning '{}'", FlowControl_name(execution_result->flow_control()));
        }
    }
    default: {
        auto value = std::dynamic_pointer_cast<Expression>(result)->to_object();
        if (value.is_error())
            return make_obj<Exception>(value.error());
        if (value.value().has_value())
            return value.value().value();
        return Obj::null();
    }
    }
}

std::string BoundFunction::to_string() const
{
    return m_definition.to_string(0);
}


}