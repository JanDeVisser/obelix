/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <core/Object.h>
#include <obelix/Context.h>
#include <obelix/Syntax.h>

namespace Obelix {

class FunctionDef;

class BoundFunction : public Object {
public:
    BoundFunction(Context<Obj>&, FunctionDef);
    Obj call(Ptr<Arguments> args) override;
    Obj call(std::string const& name, Ptr<Arguments> args);
    [[nodiscard]] std::string to_string() const override;

private:
    Context<Obj>& m_enclosing_scope;
    FunctionDef m_definition;
};

}
