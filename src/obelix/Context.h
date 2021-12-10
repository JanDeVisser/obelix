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
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

using ErrorOrNode = ErrorOr<std::shared_ptr<SyntaxNode>>;

template<typename T>
class Context {
public:
    using Processor = std::function<ErrorOrNode(std::shared_ptr<Obelix::SyntaxNode> const&, Context<T>&)>;
    using ProcessorMap = std::unordered_map<SyntaxNodeType, Processor>;

    Context() = default;

    explicit Context(ProcessorMap const& map)
        : m_parent(nullptr)
        , m_map(map)
    {
    }

    explicit Context(Context<T>* parent)
        : m_parent(parent)
        , m_map(parent->map())
    {
    }

    Context(Context<T>* parent, ProcessorMap const& map)
        : m_parent(parent)
        , m_map(map)
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
            debug(parser, "Context::get({}) = '{}'", name, value);
            return value;
        }
        if (m_parent)
            return m_parent->get(name);
        return {};
    }

    bool set(std::string const& name, T const& value)
    {
        if (m_names.contains(name)) {
            debug(parser, "Context::set({}, '{}')", name, value);
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
        debug(parser, "Context::declare({}, '{}')", name, value);
        m_names[name] = value;
        return {};
    }

    std::optional<Error> declare_global(std::string const& name, T const& value)
    {
        if (m_parent)
            return m_parent->declare_global(name, value);

        if (m_names.contains(name))
            return Error(ErrorCode::VariableAlreadyDeclared, name);
        debug(parser, "Context::declare_global({}, '{}')", name, value);
        m_names[name] = value;
        return {};
    }

    void unset(std::string const& name)
    {
        if (m_names.contains(name)) {
            debug(parser, "Context::unset({})", name);
            m_names.erase(name);
            return;
        }
        if (m_parent)
            return m_parent->unset(name);
    }

    [[nodiscard]] ProcessorMap const& map() const { return m_map; }
    [[nodiscard]] bool has_processor_for(SyntaxNodeType type) { return m_map.contains(type); }
    [[nodiscard]] std::optional<Processor> processor_for(SyntaxNodeType type)
    {
        if (has_processor_for(type))
            return m_map.at(type);
        return {};
    }

    ErrorOrNode add_if_error(ErrorOrNode maybe_error)
    {
        if (maybe_error.is_error()) {
            if (m_parent)
                return m_parent->add_if_error(maybe_error);
            else
                m_errors.push_back(maybe_error.error());
        }
        return maybe_error;
    }

    ErrorOrNode process(std::shared_ptr<SyntaxNode> const& tree)
    {
        if (!tree)
            return tree;
        if (auto processor_maybe = processor_for(tree->node_type()); processor_maybe.has_value()) {
            auto new_tree_or_error = processor_maybe.value()(tree, *this);
            return add_if_error(new_tree_or_error);
        }
        return tree;
    }

private:
    std::unordered_map<std::string, T> m_names {};
    Context<T>* m_parent { nullptr };
    ProcessorMap m_map {};
    std::vector<Error> m_errors {};
};

}
