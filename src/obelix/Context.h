/*
 * Context.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include <core/Error.h>
#include <obelix/Syntax.h>

namespace Obelix {

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
        if (m_names.contains(name))
            return m_names.at(name);
        if (m_parent)
            return m_parent->get(name);
        return {};
    }

    void set(std::string const& name, T const& value)
    {
        m_names[name] = value;
    }

    std::optional<Error> declare(std::string const& name, T const& value)
    {
        if (m_names.contains(name))
            return Error(ErrorCode::VariableAlreadyDeclared, name);
        m_names[name] = value;
        return {};
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
    std::unordered_map<std::string, T> m_names;
    Context<T>* m_parent;
    ProcessorMap m_map {};
    std::vector<Error> m_errors;
};

}
