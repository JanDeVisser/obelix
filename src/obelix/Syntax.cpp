//
// Created by Jan de Visser on 2021-09-18.
//

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
            return make_obj<Exception>(ErrorCode::OperatorUnresolved, lhs, m_operator.value());
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
