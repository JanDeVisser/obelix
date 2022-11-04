/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/bind/BindContext.h>

namespace Obelix {

FunctionCallPair make_functioncall(std::shared_ptr<BoundFunction> function, ObjectTypes types)
{
    return std::make_pair<std::shared_ptr<BoundFunction>, ObjectTypes>(std::move(function), std::move(types));
}

// -- ContextImpl -----------------------------------------------------------

ContextImpl::ContextImpl(BindContextType type, BindContext& owner)
    : m_type(type)
    , m_parent_impl((owner.parent() != nullptr) ? dynamic_cast<BindContext*>(owner.parent())->impl() : nullptr)
{
    if (m_parent_impl != nullptr)
        m_parent_impl->m_child_impls.push_back(this);
}

ContextImpl::~ContextImpl()
{
}

BindContextType ContextImpl::type() const
{
    return m_type;
}

std::shared_ptr<ContextImpl> ContextImpl::parent_impl()
{
    return m_parent_impl;
}

ContextImpls ContextImpl::children() const
{
    ContextImpls ret;
    for (auto child : m_child_impls) {
        ret.push_back(child->shared_from_this());
    }
    return ret;
}

std::shared_ptr<ModuleContext> ContextImpl::module_impl()
{
    switch (type()) {
    case BindContextType::SubContext:
        assert(parent_impl() != nullptr);
        return parent_impl()->module_impl();
    case BindContextType::ModuleContext:
        return std::dynamic_pointer_cast<ModuleContext>(shared_from_this());
    case BindContextType::RootContext:
        return nullptr;
    }
}

std::shared_ptr<RootContext> ContextImpl::root_impl()
{
    switch (type()) {
    case BindContextType::SubContext:
    case BindContextType::ModuleContext:
        assert(parent_impl() != nullptr);
        return parent_impl()->root_impl();
    case BindContextType::RootContext:
        return std::dynamic_pointer_cast<RootContext>(shared_from_this());
    }
}

// -- SubContext ------------------------------------------------------------

SubContext::SubContext(BindContext& owner)
    : ContextImpl(BindContextType::SubContext, owner)
{
}

// -- ExportsFunctions ------------------------------------------------------

ExportsFunctions::ExportsFunctions(BindContextType type, BindContext& owner)
    : ContextImpl(type, owner)
{
}

void ExportsFunctions::add_declared_function(std::string const& name, std::shared_ptr<BoundFunctionDecl> const& func)
{
    m_declared_functions.insert({ name, func });
}

FunctionRegistry const& ExportsFunctions::declared_functions() const
{
    return m_declared_functions;
}

void ExportsFunctions::add_exported_function(std::shared_ptr<BoundFunctionDecl> const& func)
{
    m_exported_functions.push_back(func);
}

BoundFunctionDecls const& ExportsFunctions::exported_functions() const
{
    return m_exported_functions;
}

std::shared_ptr<BoundFunctionDecl> ExportsFunctions::match(std::string const& name, ObjectTypes arg_types) const
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

// -- ModuleContext ---------------------------------------------------------

ModuleContext::ModuleContext(BindContext& owner, std::string name)
    : ExportsFunctions(BindContextType::ModuleContext, owner)
    , m_name(std::move(name))
{
}

std::string const& ModuleContext::name() const
{
    return m_name;
}

void ModuleContext::add_imported_function(std::shared_ptr<BoundFunctionDecl> const& func)
{
    m_imported_functions.push_back(func);
}

BoundFunctionDecls const& ModuleContext::imported_functions() const
{
    return m_imported_functions;
}

// -- RootContext -----------------------------------------------------------

RootContext::RootContext(BindContext& owner)
    : ExportsFunctions(BindContextType::RootContext, owner)
{
}

void RootContext::add_custom_type(std::shared_ptr<ObjectType> type)
{
    if (!type->is_custom())
        return;
    for (auto const& t : m_custom_types) {
        if (*t == *type)
            return;
    }
    m_custom_types.push_back(std::move(type));
}

ObjectTypes const& RootContext::custom_types() const
{
    return m_custom_types;
}

void RootContext::add_unresolved_function(FunctionCallPair func_call)
{
    m_unresolved_functions.push_back(std::move(func_call));
}

FunctionCallPairs const& RootContext::unresolved_functions() const
{
    return m_unresolved_functions;
}

void RootContext::clear_unresolved_functions()
{
    m_unresolved_functions.clear();
}

void RootContext::add_module(std::shared_ptr<BoundModule> const& module)
{
    m_modules.insert({ module->name(), module });
}

std::shared_ptr<BoundModule> RootContext::module(std::string const& name) const
{
    if (m_modules.contains(name))
        return m_modules.at(name);
    return nullptr;
}

std::shared_ptr<ModuleContext> RootContext::module_context(const std::string& name)
{
    for (auto& ctx : m_module_contexts) {
        if (ctx->name() == name)
            return ctx;
    }
    return nullptr;
}

void RootContext::add_module_context(std::shared_ptr<ModuleContext> ctx)
{
    m_module_contexts.push_back(std::move(ctx));
}

// -- BindContext -----------------------------------------------------------

BindContext::BindContext()
    : Context()
    , m_impl(std::make_shared<RootContext>(*this))
{
}

BindContext::BindContext(BindContext& parent, BindContextType type)
    : Context(parent)
{
    switch (type) {
    case BindContextType::SubContext:
        m_impl = std::make_shared<SubContext>(*this);
        break;
    case BindContextType::ModuleContext:
        fatal("Use BindContext(BindContext& parent, std::string name) to create a ModuleContext");
    case BindContextType::RootContext:
        m_impl = std::make_shared<RootContext>(*this);
        break;
    }
    return_type = parent.return_type;
}

BindContext::BindContext(Obelix::BindContext& parent, std::string name)
    : Context(parent)
    , m_impl(std::make_shared<ModuleContext>(*this, std::move(name)))
{
    m_impl->root_impl()->add_module_context(std::dynamic_pointer_cast<ModuleContext>(m_impl));
}

void BindContext::add_custom_type(std::shared_ptr<ObjectType> type)
{
    m_impl->root_impl()->add_custom_type(std::move(type));
}

ObjectTypes const& BindContext::custom_types() const
{
    return m_impl->root_impl()->custom_types();
}

void BindContext::add_unresolved_function(FunctionCallPair func_call)
{
    m_impl->root_impl()->add_unresolved_function(std::move(func_call));
}

FunctionCallPairs const& BindContext::unresolved_functions() const
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

FunctionRegistry const& BindContext::declared_functions() const
{
    return m_impl->module_impl()->declared_functions();
}

void BindContext::add_imported_function(std::shared_ptr<BoundFunctionDecl> const& func)
{
    m_impl->module_impl()->add_imported_function(func);
}

BoundFunctionDecls const& BindContext::imported_functions() const
{
    return m_impl->module_impl()->imported_functions();
}

void BindContext::add_exported_function(std::shared_ptr<BoundFunctionDecl> const& func)
{
    m_impl->module_impl()->add_exported_function(func);
}

BoundFunctionDecls const& BindContext::exported_functions() const
{
    return m_impl->module_impl()->exported_functions();
}

void BindContext::add_module(std::shared_ptr<BoundModule> const& module)
{
    m_impl->root_impl()->add_module(module);
}

std::shared_ptr<BoundModule> BindContext::module(std::string const& name) const
{
    return m_impl->root_impl()->module(name);
}

std::shared_ptr<BoundFunctionDecl> BindContext::match(std::string const& name, ObjectTypes arg_types, bool also_check_root) const
{
    if (auto ret = m_impl->module_impl()->match(name, arg_types); ret != nullptr)
        return ret;
    if (!also_check_root)
        return nullptr;
    auto root_ctx = m_impl->root_impl();
    for (auto const& ctx : root_ctx->children()) {
        auto module_impl = std::dynamic_pointer_cast<ModuleContext>(ctx);
        if (module_impl != nullptr) {
            debug(parser, "---------> *{}*", module_impl->name());
            if (module_impl->name() == "")
                return module_impl->match(name, arg_types);
        }
    }
    return nullptr;
}

}
