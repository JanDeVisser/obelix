//
// Created by Jan de Visser on 2021-09-18.
//

#include <core/Logging.h>
#include <obelix/Syntax.h>

namespace Obelix {

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

ExecutionResult Import::execute(Ptr<Scope> scope)
{
    auto module = runtime().import_module(m_name);
    scope->declare(m_name, to_obj(module->scope()));
    return {};
}

}
