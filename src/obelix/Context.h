/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include <core/Error.h>
#include <core/Logging.h>

namespace Obelix {

extern_logging_category(parser);

template<typename T>
class Context {
public:
    Context() = default;

    Context(Context<T>&&) = delete;

    explicit Context(Context<T>* parent)
        : m_parent(parent)
    {
    }

    Context(Context<T>& parent)
        : m_parent(&parent)
    {
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

    std::optional<Error> declare(std::string const& name, T const& value)
    {
        if (m_names.contains(name))
            return Error(ErrorCode::VariableAlreadyDeclared, name);
        m_names[name] = value;
        return {};
    }

    std::optional<Error> declare_global(std::string const& name, T const& value)
    {
        if (m_parent)
            return m_parent->declare_global(name, value);

        if (m_names.contains(name))
            return Error(ErrorCode::VariableAlreadyDeclared, name);
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

    template <typename Payload>
    ErrorOr<Payload> add_if_error(ErrorOr<Payload> maybe_error)
    {
        if (maybe_error.is_error()) {
            if (m_parent)
                return m_parent->add_if_error(maybe_error);
            else
                m_errors.push_back(maybe_error.error());
        }
        return maybe_error;
    }

protected:
    [[nodiscard]] Context<T>* parent() { return m_parent; }
    [[nodiscard]] Context<T>* parent() const { return m_parent; }

private:
    std::unordered_map<std::string, T> m_names {};
    Context<T>* m_parent { nullptr };
    std::vector<Error> m_errors {};
};

}
