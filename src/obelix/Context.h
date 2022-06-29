/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <core/Error.h>
#include <core/Logging.h>
#include <lexer/Token.h>

namespace Obelix {

extern_logging_category(parser);

using SyntaxError = Error<Token>;

template <typename T>
std::string dump_value(T const& value)
{
    return format("{}", value);
}

template<typename T>
class Context {
public:
    Context() = default;

    Context(Context<T>&&) = delete;

    explicit Context(Context<T>* parent)
        : m_parent(parent)
    {
        if (parent)
            parent->m_children.push_back(this);
    }

    explicit Context(Context<T>& parent)
        : m_parent(&parent)
    {
        parent.m_children.push_back(this);
    }

    virtual ~Context()
    {
        if (m_parent) {
            std::remove_if(m_parent->m_children.begin(), m_parent->m_children.end(),
                [this](auto child) { return (this == child); });
        }
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

    std::optional<SyntaxError> declare(std::string const& name, T const& value)
    {
        if (m_names.contains(name))
            return SyntaxError(ErrorCode::VariableAlreadyDeclared, name);
        m_names[name] = value;
        return {};
    }

    std::optional<SyntaxError> declare_global(std::string const& name, T const& value)
    {
        if (m_parent)
            return m_parent->declare_global(name, value);

        if (m_names.contains(name))
            return SyntaxError(ErrorCode::VariableAlreadyDeclared, name);
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

    [[nodiscard]] std::vector<Context<T>*> const& children() const
    {
        return m_children;
    }

    void dump(int level) const
    {
        std::string indent;
        while (indent.length() < 2*level)
            indent += ' ';
        for (auto const& p : m_names) {
            std::cout << indent << p.first << ": " << dump_value(p.second) << "\n";
        }
        for (auto const& child : children()) {
            child->dump(level+1);
        }
    }

    template <typename Payload>
    ErrorOr<Payload, SyntaxError> add_if_error(ErrorOr<Payload, SyntaxError> maybe_error)
    {
        if (maybe_error.is_error()) {
            add_error(maybe_error.error());
        }
        return maybe_error;
    }

    void add_error(SyntaxError error)
    {
        if (m_parent)
            return m_parent->add_error(error);
        else
            m_errors.push_back(error);
    }

protected:
    [[nodiscard]] Context<T>* parent() { return m_parent; }
    [[nodiscard]] Context<T>* parent() const { return m_parent; }

private:
    std::unordered_map<std::string, T> m_names {};
    Context<T>* m_parent { nullptr };
    std::vector<Context<T>*> m_children {};
    std::vector<SyntaxError> m_errors {};
};

}
