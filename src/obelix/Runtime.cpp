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
#include <obelix/Processor.h>
#include <obelix/Runtime.h>
#include <obelix/Syntax.h>

namespace Obelix {

Runtime::Runtime(Config const& config, bool stdlib)
    : Scope()
    , m_as_scope(make_null<Scope>())
    , m_config(config)
    , m_stdlib(stdlib)
{
}

void Runtime::construct()
{
    if (m_stdlib)
        _import_module("");
}

ExecutionResult Runtime::run(std::string const& file_name)
{
    auto scope = _import_module(file_name);
    return scope->result();
}

Ptr<Scope> Runtime::_import_module(std::string const& module)
{
    std::shared_ptr<Module> tree;
    if (!m_modules.contains(module)) {
        auto file_name = module;
        for (auto ix = 0u; ix < module.length(); ++ix) {
            if ((file_name[ix] == '.') && (file_name.substr(ix) != ".obl"))
                file_name[ix] = '/';
        }
        Parser parser(m_config, file_name);
        tree = std::dynamic_pointer_cast<Module>(parser.parse());
        auto transformed = fold_constants(tree);
        if (config().show_tree)
            printf("%s\n", transformed->to_string(0).c_str());
        if (!tree || parser.has_errors()) {
            for (auto& error : parser.errors()) {
                fprintf(stderr, "%s\n", error.to_string().c_str());
            }
            exit(1);
        }
        auto as_scope = ptr_cast<Scope>(self());
        Ptr<Scope> scope = (module.empty()) ? as_scope : make_typed<Scope>(as_scope);
        tree->execute_in(scope);
        m_modules[module] = scope;
        return scope;
    } else {
        return m_modules.at(module);
    }
}

}