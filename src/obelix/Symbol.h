/*
 * Symbo.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <compare>
#include <functional>
#include <string>
#include <vector>

#include <obelix/Type.h>

namespace Obelix {

class Symbol {
public:
    explicit Symbol(std::string identifier, ObelixType type = ObelixType::Unknown)
        : m_identifier(move(identifier))
        , m_type(type)
    {
    }

    std::string to_string() { return format("{}: {}", m_identifier, ObelixType_name(m_type)); }
    [[nodiscard]] std::string const& identifier() const { return m_identifier; }
    [[nodiscard]] ObelixType type() const { return m_type; }
    [[nodiscard]] bool operator == (Symbol const& other) const { return m_identifier == other.m_identifier; }

private:
    std::string m_identifier;
    ObelixType m_type;
};

typedef std::vector<Symbol> Symbols;

}

template<>
struct std::hash<Obelix::Symbol> {
    std::size_t operator()(Obelix::Symbol const& symbol) const noexcept
    {
        return std::hash<std::string>{}(symbol.identifier());
    }
};
