/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <map>

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/Processor.h>

namespace Obelix {

class ModuleContext;
class RootContext;

using FunctionRegistry = std::multimap<std::string, pBoundFunctionDecl>;
using VariableRegistry = std::map<std::string, pBoundVariableDeclaration>;

#define ENUMERATE_BINDCONTEXT_TYPES(S) \
    S(RootContext)                     \
    S(ModuleContext)                   \
    S(StructContext)                   \
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
class SubContext;
class ModuleContext;
class StructContext;
class RootContext;

using pContextImpl = std::shared_ptr<ContextImpl>;
using pSubContext = std::shared_ptr<SubContext>;
using pModuleContext = std::shared_ptr<ModuleContext>;
using pStructContext = std::shared_ptr<StructContext>;
using pRootContext = std::shared_ptr<RootContext>;
using ContextImpls = std::vector<pContextImpl>;
using ContextImpls = std::vector<std::shared_ptr<ContextImpl>>;
using ModuleContexts = std::map<std::string, pModuleContext>;
using StructContexts = std::map<std::string, pStructContext>;

class ContextImpl : public std::enable_shared_from_this<ContextImpl> {
public:
    ContextImpl(BindContextType, pContextImpl);
    virtual ~ContextImpl();

    [[nodiscard]] BindContextType type() const;

    [[nodiscard]] pContextImpl parent_impl();
    [[nodiscard]] pModuleContext module_impl();
    [[nodiscard]] pRootContext root_impl();
    [[nodiscard]] pModuleContext module_impl(std::string const&);
    [[nodiscard]] ContextImpls children() const;
    ErrorOr<void,SyntaxError> declare(std::string const&, pBoundVariableDeclaration const&);
    [[nodiscard]] std::optional<pBoundVariableDeclaration> get(std::string const&) const;

private:
    friend class BindContext;

    BindContextType m_type;
    pContextImpl m_parent_impl { nullptr };
    ContextImpls m_child_impls;
    std::map<std::string, pBoundVariableDeclaration> m_variables;
};

class SubContext : public ContextImpl {
public:
    explicit SubContext(pContextImpl);
};

class ExportsFunctions : public ContextImpl {
public:
    void add_declared_function(std::string const&, pBoundFunctionDecl const&);
    [[nodiscard]] FunctionRegistry const& declared_functions() const;
    void add_declared_variable(std::string const&, pBoundVariableDeclaration const&);
    [[nodiscard]] VariableRegistry const& declared_variables() const;
    [[nodiscard]] std::optional<pBoundVariableDeclaration> declared_variable(std::string const&) const;
    [[nodiscard]] BoundStatements exports() const;
    [[nodiscard]] pBoundFunctionDecl match(std::string const& name, ObjectTypes arg_types) const;

protected:
    ExportsFunctions(BindContextType, pContextImpl);

private:
    FunctionRegistry m_declared_functions;
    VariableRegistry m_declared_variables;
};


class StructContext : public ExportsFunctions {
public:
    explicit StructContext(pContextImpl, std::string);
    [[nodiscard]] std::string const& name() const;
    pBoundStructDefinition struct_definition;

private:
    std::string m_name;
};

class ModuleContext : public ExportsFunctions {
public:
    explicit ModuleContext(pContextImpl, std::string);

    [[nodiscard]] std::string const& name() const;
    void add_import(pBoundStatement const&);
    [[nodiscard]] BoundStatements const& imports() const;

    void dump() const;

private:
    std::string m_name;
    BoundStatements m_imports;
};

class RootContext : public ExportsFunctions {
public:
    RootContext(pContextImpl);

    void add_custom_type(pObjectType);
    [[nodiscard]] ObjectTypes const& custom_types() const;
    void add_unresolved(pExpression);
    [[nodiscard]] Expressions const& unresolved() const;
    void clear_unresolved();
    void add_module(pBoundModule const& module);
    [[nodiscard]] pBoundModule module(std::string const& name) const;
    [[nodiscard]] pModuleContext module_context(std::string const& name);
    void add_module_context(pModuleContext const&);
    [[nodiscard]] pBoundStructDefinition struct_definition(std::string const& name);
    [[nodiscard]] pStructContext struct_context(std::string const& name);
    void add_struct_context(pStructContext const&);

    void dump() const;

private:
    ObjectTypes m_custom_types;
    Expressions m_unresolved;
    std::unordered_map<std::string, pBoundModule> m_modules;
    ModuleContexts m_module_contexts;
    std::unordered_map<std::string, pBoundStructDefinition> m_structs;
    StructContexts m_struct_contexts;
};

class BindContext {
public:
    BindContext();
    BindContext(BindContextType);
    [[nodiscard]] BindContext& make_subcontext();
    [[nodiscard]] BindContext& make_modulecontext(std::string const& name);
    [[nodiscard]] BindContext& make_structcontext(std::string const& name);

    BindContextType type() const { return m_impl->type(); }
    std::string scope_name() const;
    void add_custom_type(pObjectType);
    [[nodiscard]] ObjectTypes const& custom_types() const;
    ErrorOr<void, SyntaxError> declare(std::string const&, pBoundVariableDeclaration const&);
    [[nodiscard]] std::optional<pBoundVariableDeclaration> get(std::string const&) const;
    void add_unresolved(pExpression);
    [[nodiscard]] Expressions const& unresolved() const;
    void clear_unresolved();
    void add_declared_function(std::string const&, pBoundFunctionDecl const&);
    [[nodiscard]] FunctionRegistry const& declared_functions() const;
    void add_exported_variable(std::string const&, pBoundVariableDeclaration const&);
    [[nodiscard]] VariableRegistry const& exported_variables() const;
    [[nodiscard]] std::optional<pBoundVariableDeclaration> exported_variable(std::string const&) const;
    [[nodiscard]] std::optional<pBoundVariableDeclaration> exported_variable(std::string const&, std::string const&) const;
    void add_import(pBoundStatement const&);
    [[nodiscard]] BoundStatements const& imports() const;
    [[nodiscard]] BoundStatements exports() const;
    void add_module(pBoundModule const&);
    [[nodiscard]] pBoundModule module(std::string const&) const;
    [[nodiscard]] pBoundStructDefinition struct_definition(std::string const& name);
    void set_struct_definition(pBoundStructDefinition);

    [[nodiscard]] pBoundFunctionDecl match(std::string const&, ObjectTypes, bool = false) const;
    [[nodiscard]] pBoundFunctionDecl match(std::string const&, std::string const&, ObjectTypes arg_types) const;
    [[nodiscard]] bool in_struct_def() const;

    void dump() const;

    pObjectType return_type { nullptr };
    int stage { 0 };

protected:
    friend class ContextImpl;
    [[nodiscard]] pContextImpl const& impl() const { return m_impl; };
    [[nodiscard]] pContextImpl impl() { return m_impl; }

private:
    pContextImpl m_impl;
    std::vector<BindContext> m_children {};
};

template<>
inline BindContext& make_subcontext(BindContext& ctx)
{
    assert(ctx.type() != BindContextType::RootContext);
    return ctx.make_subcontext();
}

}
