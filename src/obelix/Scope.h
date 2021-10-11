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
    Scope() = default;

    void set(std::string const& name, Obj value)
    {
        m_variables.put(name, std::move(value));
    }

    [[nodiscard]] std::optional<Obj> get(std::string const& name) const
    {
        return m_variables.get(name);
    }

private:
    Dictionary m_variables {};
};

}