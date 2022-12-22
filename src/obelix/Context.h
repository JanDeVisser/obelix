/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <map>
#include <vector>

#include <core/Error.h>
#include <core/Logging.h>
#include <lexer/Token.h>
#include <obelix/Config.h>

namespace Obelix {

extern_logging_category(parser);

template<typename T, typename Payload = int>
class Context {
public:
    using Ctx = Context<T, Payload>;

    Context(Config const& config)
        : m_config(config)
    {
    }

    Context(Config const& config, Payload payload)
        : m_data(std::move(payload))
        , m_config(config)
    {
    }

    Context(Ctx* parent)
        : m_config(parent->config())
        , m_parent(parent)
    {
    }

    Context(Ctx* parent, Payload payload)
        : m_data(std::move(payload))
        , m_config(parent->config())
        , m_parent(parent)
    {
    }

    virtual ~Context() = default;

    [[nodiscard]] Ctx& make_subcontext()
    {
        return m_children.emplace_back(this);
    }

    [[nodiscard]] Ctx& make_subcontext(Payload payload)
    {
        return m_children.emplace_back(this, std::move(payload));
    }

    [[nodiscard]] bool contains(std::string const& name) const
    {
        if (m_names.contains(name))
            return true;
        if (m_parent)
            return m_parent->contains(name);
        return false;
    }

    [[nodiscard]] std::optional<T> get(std::string const& name)
    {
        if (m_names.contains(name)) {
            auto value = m_names.at(name);
            return value;
        }
        if (m_parent)
            return m_parent->get(name);
        return {};
    }

    bool set(std::string const& name, T const& value)
    {
        if (m_names.contains(name)) {
            m_names[name] = value;
            return true;
        }
        if (m_parent)
            return m_parent->set(name, value);
        return false;
    }

    ErrorOr<void, SyntaxError> declare(std::string const& name, T const& value)
    {
        if (m_names.contains(name))
            return SyntaxError { ErrorCode::VariableAlreadyDeclared, name };
        m_names[name] = value;
        return {};
    }

    ErrorOr<void, SyntaxError> declare_global(std::string const& name, T const& value)
    {
        if (m_parent)
            return m_parent->declare_global(name, value);

        if (m_names.contains(name))
            return SyntaxError { ErrorCode::VariableAlreadyDeclared, name };
        m_names[name] = value;
        return {};
    }

    void unset(std::string const& name)
    {
        if (m_names.contains(name)) {
            m_names.erase(name);
            return;
        }
        if (m_parent)
            return m_parent->unset(name);
    }

    [[nodiscard]] std::map<std::string, T> const& names() const
    {
        return m_names;
    }

    [[nodiscard]] std::map<std::string, T> const& global_names() const
    {
        if (m_parent)
            return m_parent->global_names();
        return m_names;
    }

    void clear_global_names()
    {
        if (m_parent)
            return m_parent->clear_global_names();
        m_names.clear();
    }

    [[nodiscard]] Config const& config() const
    {
        return m_config;
    }

    [[nodiscard]] Payload& data()
    {
        return m_data;
    }

    [[nodiscard]] Payload const& data() const
    {
        return m_data;
    }

    [[nodiscard]] Payload& operator()()
    {
        return m_data;
    }

    [[nodiscard]] Payload const& operator()() const
    {
        return m_data;
    }

    Payload& root_data()
    {
        if (parent() != nullptr)
            return parent()->root_data();
        return data();
    }

    [[nodiscard]] Payload const& root_data() const
    {
        if (parent() != nullptr)
            return parent()->root_data();
        return data();
    }

    Payload& ancestor_data(std::function<bool(Context<T,Payload> const&)> const& predicate)
    {
        auto cur = this;
        while (!predicate(*cur)) {
            if (cur->parent() == nullptr)
                break;
            cur = cur->parent();
        }
        return cur->data();
    }

    [[nodiscard]] Payload const& ancestor_data(std::function<bool(Context<T,Payload> const&)> const& predicate) const
    {
        auto cur = this;
        while (!predicate(*cur)) {
            if (cur.parent() == nullptr)
                break;
            cur = cur->parent();
        }
        return cur->data();
    }

    template <typename Return>
    std::optional<Return> call_on_ancestors(std::function<std::optional<Return>(Context<T,Payload>&)> const& action)
    {
        auto result_maybe = action(*this);
        if (result_maybe.has_value())
            return result_maybe.value();
        if (parent() == nullptr)
            return {};
        return parent()->template call_on_ancestors<Return>(action);
    }

    void call_on_ancestors(std::function<bool(Context<T,Payload>&)> const& action)
    {
        if (action(*this) || (parent() == nullptr))
            return;
        parent()->call_on_ancestors(action);
    }

    template <typename Return>
    [[nodiscard]] std::optional<Return> call_on_ancestors(std::function<std::optional<Return>(Context<T,Payload> const&)> const& action) const
    {
        auto result_maybe = action(*this);
        if (result_maybe.has_value())
            return result_maybe.value();
        if (parent() == nullptr)
            return {};
        return parent()->template call_on_ancestors<Return>(action);
    }

    void call_on_ancestors(std::function<bool(Context<T,Payload> const&)> const& action) const
    {
        if (action(*this) || (parent() == nullptr))
            return;
        parent()->call_on_ancestors(action);
    }

    template <typename Return, typename F>
    Return call_on_root(F f)
    {
        if (parent() != nullptr)
            return parent()->template call_on_root<Return>(f);
        return f(*this);
    }

    template <typename F>
    void call_on_root(F f)
    {
        if (parent() != nullptr) {
            parent()->template call_on_root<void>(f);
            return;
        }
        f(*this);
    }

    template <typename Return, typename F>
    [[nodiscard]] Return call_on_root(F f) const
    {
        if (parent() != nullptr)
            return parent()->template call_on_root<Return>(f);
        return f(*this);
    }

    template <typename F>
    void call_on_root(F f) const
    {
        if (parent() != nullptr)
            parent()->template call_on_root<void>(f);
        f(*this);
    }

    [[nodiscard]] Ctx* parent() { return m_parent; }
    [[nodiscard]] Ctx const* parent() const { return m_parent; }

private:
    Payload m_data {};
    Config const& m_config;
    std::map<std::string, T> m_names {};
    Ctx* m_parent { nullptr };
    std::vector<Ctx> m_children;
};

template<typename Ctx>
inline Ctx& make_subcontext(Ctx& ctx)
{
    return ctx.make_subcontext();
}

template<typename T, typename Payload>
inline Context<T, Payload>& make_subcontext(Context<T, Payload>& ctx)
{
    return ctx.make_subcontext();
}

}
