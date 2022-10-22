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

template<>
struct Converter<BindContextType> {
    static std::string to_string(BindContextType val)
    {
        return BindContextType_name(val);
    }

    static double to_double(BindContextType val)
    {
        return static_cast<double>(val);
    }

    static long to_long(BindContextType val)
    {
        return static_cast<long>(val);
    }
};

class BindContext;
class ContextImpl;
class ModuleContext;
class RootContext;

using RawContextImpls = std::vector<ContextImpl*>;
using ContextImpls = std::vector<std::shared_ptr<ContextImpl>>;

class ContextImpl : public std::enable_shared_from_this<ContextImpl> {
public:
    ContextImpl(BindContextType, BindContext&);
    virtual ~ContextImpl();

    [[nodiscard]] BindContextType type() const;

    [[nodiscard]] std::shared_ptr<ContextImpl> parent_impl();
    [[nodiscard]] std::shared_ptr<ModuleContext> module_impl();
    [[nodiscard]] std::shared_ptr<RootContext> root_impl();
    [[nodiscard]] ContextImpls children() const;
private:
    BindContextType m_type;
    std::shared_ptr<ContextImpl> m_parent_impl { nullptr };
    RawContextImpls m_child_impls;
};

class SubContext : public ContextImpl {
public:
    explicit SubContext(BindContext&);
};

using FunctionRegistry = std::multimap<std::string, std::shared_ptr<BoundFunctionDecl>>;

class ExportsFunctions : public ContextImpl {
public:
    void add_declared_function(std::string const&, std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] FunctionRegistry const& declared_functions() const;
    void add_exported_function(std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] BoundFunctionDecls const& exported_functions() const;
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> match(std::string const& name, ObjectTypes arg_types) const;

protected:
    ExportsFunctions(BindContextType, BindContext&);

private:
    FunctionRegistry m_declared_functions;
    BoundFunctionDecls m_exported_functions;

};

class ModuleContext : public ExportsFunctions {
public:
    explicit ModuleContext(BindContext&, std::string name);

    [[nodiscard]] std::string const& name() const;
    void add_imported_function(std::shared_ptr<BoundFunctionDecl> const&);
    [[nodiscard]] BoundFunctionDecls const& imported_functions() const;

private:
    std::string m_name;
    BoundFunctionDecls m_imported_functions;
};

using ModuleContexts = std::vector<std::shared_ptr<ModuleContext>>;

class RootContext : public ExportsFunctions {
public:
    RootContext(BindContext&);

    void add_custom_type(std::shared_ptr<ObjectType>);
    [[nodiscard]] ObjectTypes const& custom_types() const;
    void add_unresolved_function(FunctionCall);
    [[nodiscard]] FunctionCalls const& unresolved_functions() const;
    void clear_unresolved_functions();
    void add_module(std::shared_ptr<BoundModule> const& module);
    [[nodiscard]] std::shared_ptr<BoundModule> module(std::string const& name) const;
    [[nodiscard]] std::shared_ptr<ModuleContext> module_context(std::string const& name);
    void add_module_context(std::shared_ptr<ModuleContext>);

private:
    ObjectTypes m_custom_types;
    FunctionCalls m_unresolved_functions;
    std::unordered_map<std::string, std::shared_ptr<BoundModule>> m_modules;
    ModuleContexts m_module_contexts;
};

class BindContext : public Context<std::shared_ptr<SyntaxNode>> {
public:
    std::shared_ptr<ObjectType> return_type { nullptr };
    int stage { 0 };

    BindContext();
    explicit BindContext(BindContext& parent, BindContextType type = BindContextType::SubContext);
    BindContext(BindContext& parent, std::string name);
    BindContext(BindContext*) = delete;

    BindContextType type() const { return m_impl->type(); }
    void add_custom_type(std::shared_ptr<ObjectType>);
    [[nodiscard]] ObjectTypes const& custom_types() const;
    void add_unresolved_function(FunctionCall);
    [[nodiscard]] FunctionCalls const& unresolved_functions() const;
    void clear_unresolved_functions();
    void add_declared_function(std::string const& name, std::shared_ptr<BoundFunctionDecl> const& func);
    [[nodiscard]] FunctionRegistry const& declared_functions() const;
    void add_imported_function(std::shared_ptr<BoundFunctionDecl> const& func);
    [[nodiscard]] BoundFunctionDecls const& imported_functions() const;
    void add_exported_function(std::shared_ptr<BoundFunctionDecl> const& func);
    [[nodiscard]] BoundFunctionDecls const& exported_functions() const;
    void add_module(std::shared_ptr<BoundModule> const& module);
    [[nodiscard]] std::shared_ptr<BoundModule> module(std::string const& name) const;
    [[nodiscard]] std::shared_ptr<BoundFunctionDecl> match(std::string const& name, ObjectTypes arg_types, bool = false) const;

protected:
    friend class ContextImpl;
    [[nodiscard]] std::shared_ptr<ContextImpl> const& impl() const { return m_impl; };
    [[nodiscard]] std::shared_ptr<ContextImpl> impl() { return m_impl; }

private:
    std::shared_ptr<ContextImpl> m_impl;
};



}
