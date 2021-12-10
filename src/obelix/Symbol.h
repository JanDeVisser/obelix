/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <compare>
#include <functional>
#include <string>
#include <vector>

#include <core/Type.h>

namespace Obelix {

class Symbol {
public:
    explicit Symbol(std::string identifier, ObelixType type = TypeUnknown)
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
