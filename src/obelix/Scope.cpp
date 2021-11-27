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
    : Object(TypeScope)
    , m_parent(parent)
{
}

std::optional<Symbol> Scope::get_declared_symbol(const std::string& name) const
{
    Symbol sym(name);
    auto found = m_variables.find(sym);
    if (found == m_variables.end())
        return {};
    return found->first;
}

std::optional<Obj> Scope::declare(Symbol const& name, Obj const& value)
{
    if (m_variables.contains(name))
        return make_obj<Exception>(ErrorCode::VariableAlreadyDeclared, name.identifier());
    if ((name.type() != TypeUnknown) && (value->type() != name.type()))
        return make_obj<Exception>(ErrorCode::TypeMismatch, "declaration", ObelixType_name(name.type()), value->type_name());
    m_variables[name] = value;
    return {};
}

std::optional<Obj> Scope::set(Symbol const& name, Obj const& value)
{
    if (auto sym = get_declared_symbol(name.identifier()); sym.has_value()) {
        if ((sym.value().type() != TypeUnknown) && (value->type() != sym.value().type()))
            return make_obj<Exception>(ErrorCode::TypeMismatch, "assignment", ObelixType_name(sym.value().type()), value->type_name());
        m_variables[name] = value;
        return {};
    }
    if (!m_parent)
        return make_obj<Exception>(ErrorCode::UndeclaredVariable, name.identifier());
    return m_parent->set(name, value);
}

std::optional<Obj> Scope::resolve(std::string const& name) const
{
    Symbol sym(name);
    if (m_variables.contains(sym))
        return m_variables.at(sym);
    if (m_parent)
        return m_parent->resolve(name);
    return {};
}

std::optional<Obj> Scope::assign(std::string const& name, Obj const& value)
{
    return set(Symbol { name, TypeUnknown }, value);
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