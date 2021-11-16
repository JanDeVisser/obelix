/*
 * Runtime.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#pragma once

#include <unordered_map>

namespace Obelix {

struct ExecutionResult;
class Module;
class Scope;

class Runtime {
public:
    struct Config {
        bool show_tree { false };
    };

    explicit Runtime(Runtime::Config const&, bool = true);
    std::shared_ptr<Module> import_module(std::string const&);
    ExecutionResult run(std::string const&);
    Ptr<Scope> evaluate(std::string const&);
    Ptr<Scope> evaluate(std::string const&, Ptr<Scope> scope);
    [[nodiscard]] Config const& config() const { return m_config; }
    [[nodiscard]] Ptr<Scope> new_scope() const;

private:
    std::shared_ptr<Module> _import_file(std::string const&);
    std::shared_ptr<Module> _import_file(std::string const&, Ptr<Scope>);

    Config m_config;
    std::shared_ptr<Module> m_root;
    std::unordered_map<std::string, std::shared_ptr<Module>> m_modules;
};

}
