/*
 * BoundFunction.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <core/Object.h>
#include <obelix/Scope.h>

namespace Obelix {

class FunctionDef;

class BoundFunction : public Object {
public:
    BoundFunction(Scope& scope, FunctionDef const& definition)
        : Object("boundfunction")
        , m_scope(scope)
        , m_definition(definition)
    {
    }

    Obj call(Ptr<Arguments> args) override;
    Obj call(std::string const& name, Ptr<Arguments> args);

private:
    Scope& m_scope;
    FunctionDef const& m_definition;
};

}