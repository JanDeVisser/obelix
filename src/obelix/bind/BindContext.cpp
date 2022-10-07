/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/bind/BindContext.h>

namespace Obelix {

FunctionCall make_functioncall(std::shared_ptr<BoundFunction> function, ObjectTypes types)
{
    return std::make_pair<std::shared_ptr<BoundFunction>, ObjectTypes>(std::move(function), std::move(types));
}

// -- ContextImpl -----------------------------------------------------------

ContextImpl::ContextImpl(BindContextType type, BindContext* owner)
    : m_type(type)
    , m_owner(owner)
{
}

BindContextType ContextImpl::type() const
{
    return m_type;
}

BindContext const* ContextImpl::owner() const
{
    return m_owner;
}

BindContext* ContextImpl::owner()
{
    return m_owner;
}

ContextImpl const* ContextImpl::parent_impl() const
{
    auto parent = dynamic_cast<BindContext const*>(owner()->parent());
    return parent->impl();
}

ContextImpl* ContextImpl::parent_impl()
{
    auto parent = dynamic_cast<BindContext*>(owner()->parent());
    return parent->impl();
}

ModuleContext const* ContextImpl::module_impl() const
{
    switch (type()) {
    case BindContextType::SubContext:
        assert(owner()->parent() != nullptr);
        return dynamic_cast<BindContext const*>(owner()->parent())->impl()->module_impl();
    case BindContextType::ModuleContext:
        return (ModuleContext const*)this;
    case BindContextType::RootContext:
        return nullptr;
    }
}

ModuleContext* ContextImpl::module_impl()
{
    switch (type()) {
    case BindContextType::SubContext:
        assert(owner()->parent() != nullptr);
        return dynamic_cast<BindContext*>(owner()->parent())->impl()->module_impl();
    case BindContextType::ModuleContext:
        return (ModuleContext*)this;
    case BindContextType::RootContext:
        return nullptr;
    }
}

RootContext const* ContextImpl::root_impl() const
{
    switch (type()) {
    case BindContextType::SubContext:
    case BindContextType::ModuleContext:
        assert(owner()->parent() != nullptr);
        return dynamic_cast<BindContext const*>(owner()->parent())->impl()->root_impl();
    case BindContextType::RootContext:
        return (RootContext const*)(this);
    }
}

RootContext* ContextImpl::root_impl()
{
    switch (type()) {
    case BindContextType::SubContext:
    case BindContextType::ModuleContext:
        assert(owner()->parent() != nullptr);
        return dynamic_cast<BindContext*>(owner()->parent())->impl()->root_impl();
    case BindContextType::RootContext:
        return (RootContext*)(this);
    }
}

// -- SubContext ------------------------------------------------------------

SubContext::SubContext(BindContext* owner)
    : ContextImpl(BindContextType::SubContext, owner)
{
}

// -- ModuleContext ---------------------------------------------------------

ModuleContext::ModuleContext(BindContext* owner)
    : ContextImpl(BindContextType::ModuleContext, owner)
{
}

void ModuleContext::add_declared_function(std::string const& name, std::shared_ptr<BoundFunctionDecl> const& func)
{
    m_declared_functions.insert({ name, func });
}

FunctionRegistry const& ModuleContext::declared_functions() const
{
    return m_declared_functions;
}

void ModuleContext::clear_declared_functions()
{
    m_declared_functions.clear();
}

std::shared_ptr<BoundFunctionDecl> ModuleContext::match(std::string const& name, ObjectTypes arg_types) const
{
    debug(parser, "matching function {}({})", name, arg_types);
    std::shared_ptr<BoundFunctionDecl> func_decl = nullptr;
    auto range = m_declared_functions.equal_range(name);
    for (auto iter = range.first; iter != range.second; ++iter) {
        debug(parser, "checking {}({})", iter->first, iter->second->parameters());
        auto candidate = iter->second;
        if (arg_types.size() != candidate->parameters().size())
            continue;

        bool all_matched = true;
        for (auto ix = 0u; ix < arg_types.size(); ix++) {
            auto& arg_type = arg_types.at(ix);
            auto& param = candidate->parameters().at(ix);
            if (!arg_type->is_assignable_to(param->type())) {
                all_matched = false;
                break;
            }
        }
        if (all_matched) {
            func_decl = candidate;
            break;
        }
    }
    if (func_decl != nullptr)
        debug(parser, "match() returns {}", *func_decl);
    else
        debug(parser, "No matching function found");
    return func_decl;
}

// -- RootContext -----------------------------------------------------------

RootContext::RootContext(BindContext* owner)
    : ContextImpl(BindContextType::RootContext, owner)
{
}

void RootContext::add_unresolved_function(FunctionCall func_call)
{
    m_unresolved_functions.push_back(std::move(func_call));
}

FunctionCalls const& RootContext::unresolved_functions() const
{
    return m_unresolved_functions;
}

void RootContext::clear_unresolved_functions()
{
    m_unresolved_functions.clear();
}

void RootContext::add_module(std::shared_ptr<BoundModule> const& module)
{
    std::cout << "Adding module " << module->name() << "\n";
    m_modules.insert({ module->name(), module });
}

std::shared_ptr<BoundModule> RootContext::module(std::string const& name) const
{
    if (m_modules.contains(name))
        return m_modules.at(name);
    return nullptr;
}

// -- BindContext -----------------------------------------------------------

BindContext::BindContext(BindContext& parent, BindContextType type)
    : Context(parent)
{
    switch (type) {
    case BindContextType::SubContext:
        m_impl = std::make_unique<SubContext>(this);
        break;
    case BindContextType::ModuleContext:
        m_impl = std::make_unique<ModuleContext>(this);
        break;
    case BindContextType::RootContext:
        m_impl = std::make_unique<RootContext>(this);
        break;
    }
    return_type = parent.return_type;
}

BindContext::BindContext()
    : Context()
    , m_impl(std::make_unique<RootContext>(this))
{
}

void BindContext::add_unresolved_function(FunctionCall func_call)
{
    m_impl->root_impl()->add_unresolved_function(std::move(func_call));
}

FunctionCalls const& BindContext::unresolved_functions() const
{
    return m_impl->root_impl()->unresolved_functions();
}

void BindContext::clear_unresolved_functions()
{
    m_impl->root_impl()->clear_unresolved_functions();
}

void BindContext::add_declared_function(std::string const& name, std::shared_ptr<BoundFunctionDecl> const& func)
{
    m_impl->module_impl()->add_declared_function(name, func);
}

std::multimap<std::string, std::shared_ptr<BoundFunctionDecl>> const& BindContext::declared_functions() const
{
    return m_impl->module_impl()->declared_functions();
}

void BindContext::add_module(std::shared_ptr<BoundModule> const& module)
{
    m_impl->root_impl()->add_module(module);
}

std::shared_ptr<BoundModule> BindContext::module(std::string const& name) const
{
    return m_impl->root_impl()->module(name);
}

std::shared_ptr<BoundFunctionDecl> BindContext::match(std::string const& name, ObjectTypes arg_types) const
{
    return m_impl->module_impl()->match(name, arg_types);
}

void BindContext::clear_declared_functions()
{
    m_impl->module_impl()->clear_declared_functions();
}

}
