/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <map>

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

class ModuleContext;
class RootContext;

using FunctionCall = std::pair<std::shared_ptr<BoundFunction>, ObjectTypes>;
using FunctionCalls = std::vector<FunctionCall>;

FunctionCall make_functioncall(std::shared_ptr<BoundFunction>, ObjectTypes);

#define ENUMERATE_BINDCONTEXT_TYPES(S) \
    S(RootContext)                     \
    S(ModuleContext)                   \
    S(SubContext)

enum class BindContextType {
#undef ENUM_BINDCONTEXT_TYPE
#define ENUM_BINDCONTEXT_TYPE(type) type,
    ENUMERATE_BINDCONTEXT_TYPES(ENUM_BINDCONTEXT_TYPE)
#undef ENUM_BINDCONTEXT_TYPE
};

constexpr char const* BindContextType_name(BindContextType type)
{
    switch (type) {
#undef ENUM_BINDCONTEXT_TYPE
#define ENUM_BINDCONTEXT_TYPE(type) \
    case BindContextType::type:    \
        return #type;
        ENUMERATE_BINDCONTEXT_TYPES(ENUM_BINDCONTEXT_TYPE)
#undef ENUM_BINDCONTEXT_TYPE
    default:
        fatal("Unknown BindContextType value '{}'", (int)type);
    }
}

class BindContext;
class ModuleContext;
class RootContext;

class ContextImpl {
public:
    explicit ContextImpl(BindContextType, BindContext*);
    [[nodiscard]] BindContextType type() const;
    [[nodiscard]] BindContext const* owner() const;
    [[nodiscard]] BindContext* owner();

    [[nodiscard]] ContextImpl const* parent_impl() const;
    [[nodiscard]] ContextImpl* parent_impl();
    [[nodiscard]] ModuleContext const* module_impl() const;
    [[nodiscard]] ModuleContext* module_impl();
    [[nodiscard]] RootContext const* root_impl() const;
    [[nodiscard]] RootContext* root_impl();
private:
    BindContextType m_type;
    BindContext* m_owner;
};

class SubContext : public ContextImpl {
public:
    explicit SubContext(BindContext*);
};

using FunctionRegistry = std::multimap<std::string, std::shared_ptr<BoundFunctionDecl>>;

class ModuleContext : public ContextImpl {
public:
    explicit ModuleContext(BindContext*);

    void add_declared_function(std::string const&, std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] FunctionRegistry const& declared_functions() const;
    void clear_declared_functions();
    void add_imported_function(std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] BoundFunctionDecls const& imported_functions() const;
    void clear_imported_functions();
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> match(std::string const& name, ObjectTypes arg_types) const;

private:
    FunctionRegistry m_declared_functions;
    BoundFunctionDecls m_imported_functions;
};

class RootContext : public ContextImpl {
public:
    RootContext(BindContext*);

    void add_unresolved_function(FunctionCall);
    [[nodiscard]] FunctionCalls const& unresolved_functions() const;
    void clear_unresolved_functions();
    void add_module(std::shared_ptr<BoundModule> const& module);
    [[nodiscard]] std::shared_ptr<BoundModule> module(std::string const& name) const;

private:
    FunctionCalls m_unresolved_functions;
    std::unordered_map<std::string, std::shared_ptr<BoundModule>> m_modules;
};

class BindContext : public Context<std::shared_ptr<SyntaxNode>> {
public:
    std::shared_ptr<ObjectType> return_type { nullptr };
    int stage { 0 };

    BindContext();
    explicit BindContext(BindContext&, BindContextType = BindContextType::SubContext);
    BindContext(BindContext*) = delete;

    BindContextType type() const { return m_impl->type(); }
    void add_unresolved_function(FunctionCall);
    [[nodiscard]] FunctionCalls const& unresolved_functions() const;
    void clear_unresolved_functions();
    void add_declared_function(std::string const& name, std::shared_ptr<BoundFunctionDecl> const& func);
    [[nodiscard]] FunctionRegistry const& declared_functions() const;
    void add_imported_function(std::shared_ptr<BoundFunctionDecl> const& func);
    [[nodiscard]] BoundFunctionDecls const& imported_functions() const;
    void add_module(std::shared_ptr<BoundModule> const& module);
    [[nodiscard]] std::shared_ptr<BoundModule> module(std::string const& name) const;
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> match(std::string const& name, ObjectTypes arg_types) const;

protected:
    friend class ContextImpl;
    [[nodiscard]] ContextImpl const* impl() const { return m_impl.get(); };
    [[nodiscard]] ContextImpl* impl() { return m_impl.get(); }

private:
    std::unique_ptr<ContextImpl> m_impl;
};

}
