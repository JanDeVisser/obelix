/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <compare>
#include <functional>
#include <string>
#include <vector>

#include "Type.h"

namespace Obelix {

class Symbol {
public:
    explicit Symbol(std::string identifier, std::shared_ptr<ObjectType> type = nullptr)
        : m_identifier(move(identifier))
        , m_type(move(type))
    {
    }

    Symbol(std::string identifier, ObelixType type)
        : m_identifier(move(identifier))
        , m_type(ObjectType::get(type))
    {
    }

    std::string to_string() { return format("{}: {}", m_identifier, type_name(type())); }
    [[nodiscard]] std::string const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return m_type; }
    [[nodiscard]] bool is_typed() const { return m_type != nullptr; }
    [[nodiscard]] bool operator==(Symbol const& other) const { return m_identifier == other.m_identifier; }

private:
    std::string m_identifier;
    std::shared_ptr<ObjectType> m_type { nullptr };
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
