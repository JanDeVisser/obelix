/*
 * Runtime.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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
#include <obelix/Runtime.h>
#include <obelix/Syntax.h>

namespace Obelix {

Runtime::Runtime()
    : m_root(_import_file("", make_null<Scope>()))
{
}

std::shared_ptr<Module> Runtime::import_module(std::string const& module)
{
    if (!m_modules.contains(module))
        m_modules[module] = _import_file(module, m_root->scope());
    return m_modules[module];
}

ExecutionResult Runtime::run(std::string const& file_name)
{
    auto mod = _import_file(file_name, m_root->scope());
    return mod->scope()->result();
}

std::shared_ptr<Module> Runtime::_import_file(std::string const& module, Ptr<Scope> into_scope)
{
    auto file_name = module;
    for (auto ix = 0u; ix < module.length(); ++ix) {
        if ((file_name[ix] == '.') && (file_name.substr(ix) != ".obl"))
            file_name[ix] = '/';
    }
    Parser parser(file_name);
    auto tree = parser.parse(*this);
    if (!tree || parser.has_errors()) {
        for (auto& error : parser.errors()) {
            fprintf(stderr, "%s\n", error.to_string().c_str());
        }
        exit(1);
    }
    tree->execute(std::move(into_scope));
    return tree;
}

}