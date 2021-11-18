/*
 * Scope.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <obelix/Parser.h>
#include <obelix/Scope.h>

namespace Obelix {

Scope::Scope(Ptr<Scope> const& parent)
    : Object("scope")
    , m_parent(parent)
{
}

void Scope::declare(std::string const& name, Obj const& value)
{
    if (m_variables.contains(name))
        fatal("Variable {} already declared in scope", name);
    m_variables.put(name, value);
}

void Scope::set(std::string const& name, Obj const& value)
{
    if (m_variables.contains(name)) {
        m_variables.put(name, value);
        return;
    }
    if (!m_parent)
        fatal("Undeclared variable {}", name);
    m_parent->set(name, value);
}

std::optional<Obj> Scope::resolve(std::string const& name) const
{
    if (m_variables.contains(name))
        return m_variables.get(name);
    if (m_parent)
        return m_parent->resolve(name);
    return {};
}

std::optional<Obj> Scope::assign(std::string const& name, Obj const& value)
{
    if (m_variables.contains(name)) {
        m_variables.put(name, value);
        return value;
    }
    if (!m_parent)
        return {};
    return m_parent->assign(name, value);
}

[[nodiscard]] Ptr<Object> Scope::copy() const
{
    auto ret = make_typed<Scope>(m_parent);
    for (Obj const& elem : m_variables) {
        auto nvp = Obelix::ptr_cast<Obelix::NVP>(elem);
        ret->declare(nvp->name(), nvp->value());
    }
    return to_obj(ret);
}

Ptr<Scope> Scope::eval(std::string const& src)
{
    StringBuffer source_text(src);
    Parser parser(config(), source_text);
    auto tree = parser.parse();
    if (!tree || parser.has_errors()) {
        auto errors = make_typed<List>();
        for (auto& error : parser.errors()) {
            errors->push_back(make_obj<Exception>(ErrorCode::SyntaxError, error.to_string()));
        }
        set_result({ ExecutionResultCode::Error, to_obj<List>(errors) });
        return ptr_cast<Scope>(self());
    }
    auto as_scope = ptr_cast<Scope>(self());
    tree->execute_in(as_scope);
    return tree->scope();
}

}