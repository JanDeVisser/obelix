//
// Created by Jan de Visser on 2021-09-18.
//

#include <core/Logging.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

Runtime& SyntaxNode::runtime() const
{
    return module()->runtime();
}

Module const* SyntaxNode::module() const
{
    assert(parent());
    return parent()->module();
}

Module const* Module::module() const
{
    return this;
}

Runtime& Module::runtime() const
{
    return m_runtime;
}

ExecutionResult Import::execute(Ptr<Scope>& scope)
{
    auto module = runtime().import_module(m_name);
    scope->declare(m_name, to_obj(module->scope()));
    return {};
}

Obj BinaryExpression::evaluate(Ptr<Scope>& scope)
{
    Obj rhs = m_rhs->evaluate(scope);
    switch (m_operator.code()) {
    case TokenCode::Equals:
        if (auto ret_maybe = m_lhs->assign(scope, m_operator, rhs); ret_maybe.has_value())
            return ret_maybe.value();
        return make_obj<Exception>(ErrorCode::SyntaxError, "Could not assign to non-lvalue");
    default: {
        Obj lhs = m_lhs->evaluate(scope);
        auto ret_maybe = lhs->evaluate(m_operator.value(), make_typed<Arguments>(rhs));
        if (!ret_maybe.has_value())
            return make_obj<Exception>(ErrorCode::FunctionUndefined, m_operator);
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
    auto result = lhs->assign(rhs->to_string(), op.value(), value);
    if (!result.has_value())
        return make_obj<Exception>(ErrorCode::CannotAssignToObject, lhs->to_string());
    return result.value();
}

}
