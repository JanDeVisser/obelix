/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/bind/BindContext.h>

namespace Obelix {

extern_logging_category(bind);

FunctionCallPair make_functioncall(pSyntaxNode function, ObjectTypes types)
{
    return std::make_pair<pSyntaxNode, ObjectTypes>(std::move(function), std::move(types));
}

// -- ContextImpl -----------------------------------------------------------

ContextImpl::ContextImpl(BindContextType type, pContextImpl parent_impl)
    : m_type(type)
    , m_parent_impl(std::move(parent_impl))
{
}

ContextImpl::~ContextImpl()
{
}

BindContextType ContextImpl::type() const
{
    return m_type;
}

pContextImpl ContextImpl::parent_impl()
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

pModuleContext ContextImpl::module_impl()
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

pRootContext ContextImpl::root_impl()
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

pModuleContext ContextImpl::module_impl(std::string const& module)
{
    switch (type()) {
    case BindContextType::SubContext:
        assert(parent_impl() != nullptr);
        return root_impl()->module_impl(module);
    case BindContextType::ModuleContext: {
        auto module_ctx = dynamic_cast<ModuleContext*>(this);
        if (module_ctx->name() == module)
            return std::dynamic_pointer_cast<ModuleContext>(shared_from_this());
        return root_impl()->module_impl(module);
    }
    case BindContextType::RootContext:
        return root_impl()->module_context(module);
    }
}

std::optional<SyntaxError> ContextImpl::declare(std::string const& name, pBoundVariableDeclaration const& decl)
{
    if (m_variables.contains(name))
        return SyntaxError(ErrorCode::VariableAlreadyDeclared, name);
    m_variables[name] = decl;
    return {};
}

std::optional<pBoundVariableDeclaration> ContextImpl::get(std::string const& name) const
{
    if (m_variables.contains(name)) {
        auto value = m_variables.at(name);
        return value;
    }
    if (m_parent_impl)
        return m_parent_impl->get(name);
    return {};
}


// -- SubContext ------------------------------------------------------------

SubContext::SubContext(pContextImpl parent_impl)
    : ContextImpl(BindContextType::SubContext, std::move(parent_impl))
{
}

// -- ExportsFunctions ------------------------------------------------------

ExportsFunctions::ExportsFunctions(BindContextType type, pContextImpl parent_impl)
    : ContextImpl(type, std::move(parent_impl))
{
}

void ExportsFunctions::add_declared_function(std::string const& name, pBoundFunctionDecl const& func)
{
    m_declared_functions.insert({ name, func });
}

FunctionRegistry const& ExportsFunctions::declared_functions() const
{
    return m_declared_functions;
}

void ExportsFunctions::add_exported_function(pBoundFunctionDecl const& func)
{
    m_exported_functions.push_back(func);
}

BoundFunctionDecls const& ExportsFunctions::exported_functions() const
{
    return m_exported_functions;
}

pBoundFunctionDecl ExportsFunctions::match(std::string const& name, ObjectTypes arg_types) const
{
    debug(bind, "matching function {}({})", name, arg_types);
    pBoundFunctionDecl func_decl = nullptr;
    auto range = m_declared_functions.equal_range(name);
    for (auto iter = range.first; iter != range.second; ++iter) {
        debug(bind, "checking {}({})", iter->first, iter->second->parameters());
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
        debug(bind, "match() returns {}", *func_decl);
    else
        debug(bind, "No matching function found");
    return func_decl;
}

// -- ModuleContext ---------------------------------------------------------

ModuleContext::ModuleContext(pContextImpl parent_impl, std::string name)
    : ExportsFunctions(BindContextType::ModuleContext, std::move(parent_impl))
    , m_name(std::move(name))
{
}

std::string const& ModuleContext::name() const
{
    return m_name;
}

void ModuleContext::add_imported_function(pBoundFunctionDecl const& func)
{
    m_imported_functions.push_back(func);
}

BoundFunctionDecls const& ModuleContext::imported_functions() const
{
    return m_imported_functions;
}

void ModuleContext::dump() const
{
    std::cerr << name() << "\n";
    for (int count = 0; count < name().length(); ++count) std::cerr << "=";
    std::cerr << "\n";
    for (auto const& fnc : exported_functions()) {
        std::cerr << fnc->to_string() << "\n";
    }
    std::cerr << "\n";
}

// -- RootContext -----------------------------------------------------------

RootContext::RootContext(pContextImpl parent_impl)
    : ExportsFunctions(BindContextType::RootContext, std::move(parent_impl))
{
}

void RootContext::add_custom_type(pObjectType type)
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

void RootContext::add_module(pBoundModule const& module)
{
    m_modules.insert({ module->name(), module });
}

pBoundModule RootContext::module(std::string const& name) const
{
    if (m_modules.contains(name))
        return m_modules.at(name);
    return nullptr;
}

pModuleContext RootContext::module_context(const std::string& name)
{
    if (m_module_contexts.contains(name))
        return m_module_contexts.at(name);
    return nullptr;
}

void RootContext::add_module_context(pModuleContext const& ctx)
{
    if (!m_module_contexts.contains(ctx->name()))
        m_module_contexts[ctx->name()] = ctx;
}

void RootContext::dump() const
{
    std::cerr << "\nExported Functions:\n";
    for (auto const& mod : m_module_contexts) {
        mod.second->dump();
    }
    if (!m_unresolved_functions.empty()) {
        std::cerr << "Unresolved:\n\n";
        for (auto const& unresolved : m_unresolved_functions) {
            std::cerr << unresolved.first->to_string() << "\n";
        }
        std::cerr << "\n";
    }
}

// -- BindContext -----------------------------------------------------------

BindContext::BindContext()
    : BindContext(BindContextType::RootContext)
{
}

BindContext::BindContext(BindContextType type)
{
    switch (type) {
    case BindContextType::SubContext:
        m_impl = std::make_shared<SubContext>(impl());
        break;
    case BindContextType::ModuleContext:
        break;
    case BindContextType::RootContext:
        m_impl = std::make_shared<RootContext>(impl());
        break;
    }
}

BindContext& BindContext::make_subcontext()
{
    auto& sub = m_children.emplace_back(BindContextType::SubContext);
    sub.m_impl = std::make_shared<SubContext>(impl());
    sub.return_type = return_type;
    m_impl->m_child_impls.push_back(sub.m_impl);
    return sub;
}

BindContext& BindContext::make_modulecontext(const std::string& name)
{
    assert(m_impl->type() == BindContextType::RootContext);
    auto& sub = m_children.emplace_back(BindContextType::ModuleContext);
    auto root = std::dynamic_pointer_cast<RootContext>(m_impl);
    auto impl = root->module_context(name);
    if (impl == nullptr) {
        impl = std::make_shared<ModuleContext>(this->impl(), name);
        root->add_module_context(impl);
    }
    sub.m_impl = impl;
    sub.return_type = return_type;
    m_impl->m_child_impls.push_back(sub.m_impl);
    return sub;
}

void BindContext::add_custom_type(pObjectType type)
{
    m_impl->root_impl()->add_custom_type(std::move(type));
}

ObjectTypes const& BindContext::custom_types() const
{
    return m_impl->root_impl()->custom_types();
}

std::optional<SyntaxError> BindContext::declare(std::string const& name, Obelix::pBoundVariableDeclaration const& decl)
{
    return impl()->declare(name, decl);
}

std::optional<pBoundVariableDeclaration> BindContext::get(std::string const& name) const
{
    return impl()->get(name);
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

void BindContext::add_declared_function(std::string const& name, pBoundFunctionDecl const& func)
{
    m_impl->module_impl()->add_declared_function(name, func);
}

FunctionRegistry const& BindContext::declared_functions() const
{
    return m_impl->module_impl()->declared_functions();
}

void BindContext::add_imported_function(pBoundFunctionDecl const& func)
{
    m_impl->module_impl()->add_imported_function(func);
}

BoundFunctionDecls const& BindContext::imported_functions() const
{
    return m_impl->module_impl()->imported_functions();
}

void BindContext::add_exported_function(pBoundFunctionDecl const& func)
{
    m_impl->module_impl()->add_exported_function(func);
}

BoundFunctionDecls const& BindContext::exported_functions() const
{
    return m_impl->module_impl()->exported_functions();
}

void BindContext::add_module(pBoundModule const& module)
{
    m_impl->root_impl()->add_module(module);
}

pBoundModule BindContext::module(std::string const& name) const
{
    return m_impl->root_impl()->module(name);
}

pBoundFunctionDecl BindContext::match(std::string const& name, ObjectTypes arg_types, bool also_check_root) const
{
    if (auto ret = m_impl->module_impl()->match(name, arg_types); ret != nullptr)
        return ret;
    if (!also_check_root)
        return nullptr;
    auto root = m_impl->module_impl("/");
    if (root != nullptr)
        return root->match(name, arg_types);
    return nullptr;
}

pBoundFunctionDecl BindContext::match(std::string const& module, std::string const& name, ObjectTypes arg_types) const
{
    return m_impl->module_impl(module)->match(name, arg_types);
}

void BindContext::dump() const
{
    m_impl->root_impl()->dump();
}

}
