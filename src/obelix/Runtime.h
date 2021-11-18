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
#include <obelix/Scope.h>

namespace Obelix {

struct ExecutionResult;
class Module;

class Runtime : public Scope {
public:
    explicit Runtime(Config const&, bool = true);
    ExecutionResult run(std::string const&);
    [[nodiscard]] Config const& config() const override { return m_config; }
    Ptr<Scope> import_module(std::string const& module) override { return _import_module(module); }
    [[nodiscard]] Ptr<Scope> make_scope() const { return make_typed<Scope>(ptr_cast<Scope>(self())); }
    [[nodiscard]] Ptr<Scope>& as_scope()
    {
        if (!m_as_scope)
            m_as_scope = ptr_cast<Scope>(self());
        return m_as_scope;
    }
    void construct() override;

private:
    Ptr<Scope> _import_module(std::string const&);

    Ptr<Scope> m_as_scope;
    Config m_config;
    std::unordered_map<std::string, Ptr<Scope>> m_modules;
    bool m_stdlib;
};

}
