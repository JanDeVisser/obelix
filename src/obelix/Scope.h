//
// Created by Jan de Visser on 2021-10-11.
//

#pragma once

#include <core/Arguments.h>
#include <core/Dictionary.h>
#include <core/Object.h>
#include <lexer/Token.h>
#include <string>

namespace Obelix {
class Scope {
public:
    explicit Scope(Scope* parent = nullptr)
        : m_parent(parent)
    {
    }

    void declare(std::string const& name, Obj value)
    {
        if (m_variables.contains(name))
            fatal("Variable {} already declared in scope", name);
        m_variables.put(name, std::move(value));
    }

    void set(std::string const& name, Obj value)
    {
        if (m_variables.contains(name)) {
            m_variables.put(name, std::move(value));
            return;
        }
        if (!m_parent)
            fatal("Undeclared variable {}", name);
        m_parent->set(name, std::move(value));
    }

    [[nodiscard]] std::optional<Obj> get(std::string const& name) const
    {
        if (m_variables.contains(name))
            return m_variables.get(name);
        if (m_parent)
            return m_parent->get(name);
        return {};
    }

private:
    Scope* m_parent;
    Dictionary m_variables {};
};

}