/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/bind/BindContext.h>

namespace Obelix {

extern_logging_category(bind);

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
    case BindContextType::StructContext:
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
    case BindContextType::StructContext:
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
    case BindContextType::StructContext:
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

ErrorOr<void, SyntaxError> ContextImpl::declare(std::string const& name, pBoundVariableDeclaration const& decl)
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

void ExportsFunctions::add_declared_variable(std::string const& name, pBoundVariableDeclaration const& variable)
{
    m_declared_variables[name] = variable;
}

VariableRegistry const& ExportsFunctions::declared_variables() const
{
    return m_declared_variables;
}

std::optional<pBoundVariableDeclaration> ExportsFunctions::declared_variable(std::string const& name) const
{
    if (!m_declared_variables.contains(name))
        return {};
    return m_declared_variables.at(name);
}

BoundStatements ExportsFunctions::exports() const
{
    BoundStatements ret;
    for (auto const& name_decl : m_declared_functions) {
        ret.push_back(name_decl.second);
    }
    for (auto const& name_decl : m_declared_variables) {
        if (std::dynamic_pointer_cast<BoundGlobalVariableDeclaration>(name_decl.second) != nullptr)
            ret.push_back(name_decl.second);
    }
    return ret;
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

void ModuleContext::add_import(pBoundStatement const& import)
{
    m_imports.push_back(import);
}

BoundStatements const& ModuleContext::imports() const
{
    return m_imports;
}

void ModuleContext::dump() const
{
    std::cerr << name() << "\n";
    for (int count = 0; count < name().length(); ++count)
        std::cerr << "=";
    std::cerr << "\n";
    for (auto const& fnc : exports()) {
        std::cerr << fnc->to_string() << "\n";
    }
    std::cerr << "\n";
}

// -- StructContext ---------------------------------------------------------

StructContext::StructContext(pContextImpl parent_impl, std::string name)
    : ExportsFunctions(BindContextType::StructContext, std::move(parent_impl))
    , m_name(std::move(name))
{
}

std::string const& StructContext::name() const
{
    return m_name;
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

void RootContext::add_unresolved(pExpression expr)
{
    m_unresolved.push_back(std::move(expr));
}

Expressions const& RootContext::unresolved() const
{
    return m_unresolved;
}

void RootContext::clear_unresolved()
{
    m_unresolved.clear();
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

pBoundStructDefinition RootContext::struct_definition(std::string const& name)
{
    if (auto ctx = struct_context(name); ctx != nullptr)
        return ctx->struct_definition;
    return nullptr;
}

pStructContext RootContext::struct_context(const std::string& name)
{
    if (m_struct_contexts.contains(name))
        return m_struct_contexts.at(name);
    return nullptr;
}

void RootContext::add_struct_context(pStructContext const& ctx)
{
    if (!m_struct_contexts.contains(ctx->name()))
        m_struct_contexts[ctx->name()] = ctx;
}

void RootContext::dump() const
{
    std::cerr << "\nExported Functions:\n";
    for (auto const& mod : m_module_contexts) {
        mod.second->dump();
    }
    if (!m_unresolved.empty()) {
        std::cerr << "Unresolved:\n\n";
        for (auto const& unresolved : m_unresolved) {
            std::cerr << unresolved->to_string() << "\n";
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
    case BindContextType::StructContext:
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
        impl = std::make_shared<ModuleContext>(m_impl, name);
        root->add_module_context(impl);
    }
    sub.m_impl = impl;
    m_impl->m_child_impls.push_back(sub.m_impl);
    return sub;
}

BindContext& BindContext::make_structcontext(const std::string& name)
{
    assert(m_impl->type() == BindContextType::ModuleContext);
    auto& sub = m_children.emplace_back(BindContextType::StructContext);
    auto module = std::dynamic_pointer_cast<ModuleContext>(m_impl);
    auto root = std::dynamic_pointer_cast<RootContext>(m_impl->root_impl());
    auto impl = root->struct_context(name);
    if (impl == nullptr) {
        impl = std::make_shared<StructContext>(m_impl, name);
        root->add_struct_context(impl);
    }
    sub.m_impl = impl;
    m_impl->m_child_impls.push_back(sub.m_impl);
    return sub;
}

std::string BindContext::scope_name() const
{
    auto impl = m_impl;
    while (impl != nullptr) {
        if (impl->type() == BindContextType::StructContext)
            return std::dynamic_pointer_cast<StructContext>(impl)->name();
        if (impl->type() == BindContextType::ModuleContext)
            return std::dynamic_pointer_cast<ModuleContext>(impl)->name();
        impl = impl->parent_impl();
    }
    fatal("Unreachable");
}

void BindContext::add_custom_type(pObjectType type)
{
    m_impl->root_impl()->add_custom_type(std::move(type));
}

ObjectTypes const& BindContext::custom_types() const
{
    return m_impl->root_impl()->custom_types();
}

ErrorOr<void, SyntaxError> BindContext::declare(std::string const& name, Obelix::pBoundVariableDeclaration const& decl)
{
    return impl()->declare(name, decl);
}

std::optional<pBoundVariableDeclaration> BindContext::get(std::string const& name) const
{
    return impl()->get(name);
}

void BindContext::add_unresolved(pExpression expr)
{
    m_impl->root_impl()->add_unresolved(std::move(expr));
}

Expressions const& BindContext::unresolved() const
{
    return m_impl->root_impl()->unresolved();
}

void BindContext::clear_unresolved()
{
    m_impl->root_impl()->clear_unresolved();
}

void BindContext::add_declared_function(std::string const& name, pBoundFunctionDecl const& func)
{
    m_impl->module_impl()->add_declared_function(name, func);
}

FunctionRegistry const& BindContext::declared_functions() const
{
    return m_impl->module_impl()->declared_functions();
}

void BindContext::add_exported_variable(std::string const& name, Obelix::pBoundVariableDeclaration const& variable)
{
    m_impl->module_impl()->add_declared_variable(name, variable);
}

VariableRegistry const& BindContext::exported_variables() const
{
    return m_impl->module_impl()->declared_variables();
}

std::optional<pBoundVariableDeclaration> BindContext::exported_variable(std::string const& name) const
{
    return m_impl->module_impl()->declared_variable(name);
}

std::optional<pBoundVariableDeclaration> BindContext::exported_variable(std::string const& module, std::string const& name) const
{
    auto mod = m_impl->module_impl(module);
    if (mod == nullptr)
        return {};
    return mod->declared_variable(name);
}

void BindContext::add_import(pBoundStatement const& import)
{
    m_impl->module_impl()->add_import(import);
}

BoundStatements const& BindContext::imports() const
{
    return m_impl->module_impl()->imports();
}

BoundStatements BindContext::exports() const
{
    return m_impl->module_impl()->exports();
}

void BindContext::add_module(pBoundModule const& module)
{
    m_impl->root_impl()->add_module(module);
}

pBoundModule BindContext::module(std::string const& name) const
{
    return m_impl->root_impl()->module(name);
}

pBoundStructDefinition BindContext::struct_definition(std::string const& name)
{
    return m_impl->root_impl()->struct_definition(name);
}

void BindContext::set_struct_definition(pBoundStructDefinition struct_def)
{
    assert(m_impl->type() == BindContextType::StructContext);
    std::dynamic_pointer_cast<StructContext>(m_impl)->struct_definition = std::move(struct_def);
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
    auto mod = m_impl->module_impl(module);
    if (mod == nullptr)
        return nullptr;
    return mod->match(name, arg_types);
}

void BindContext::dump() const
{
    m_impl->root_impl()->dump();
}

bool BindContext::in_struct_def() const
{
    auto impl = m_impl;
    while (impl != nullptr) {
        if (impl->type() == BindContextType::StructContext)
            return true;
        impl = impl->parent_impl();
    }
    return false;
}

}
