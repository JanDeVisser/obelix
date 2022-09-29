/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/syntax/Typedef.h>

namespace Obelix {

extern_logging_category(parser);

// -- StructForward ---------------------------------------------------------

StructForward::StructForward(Token token, std::string name)
    : Statement(std::move(token))
    , m_name(std::move(name))
{
}

std::string StructForward::to_string() const
{
    return format("forward struct {}", name());
}

std::string StructForward::attributes() const
{
    return format(R"(name="{}")", name());
}

std::string const& StructForward::name() const
{
    return m_name;
}

// -- StructDefinition ------------------------------------------------------

StructDefinition::StructDefinition(Token token, std::string name, Identifiers fields)
    : Statement(std::move(token))
    , m_name(std::move(name))
    , m_fields(std::move(fields))
{
}

Nodes StructDefinition::children() const
{
    Nodes ret;
    for (auto const& field : m_fields) {
        ret.push_back(field);
    }
    return ret;
}

std::string StructDefinition::to_string() const
{
    auto ret = format("struct {} {{", name());
    for (auto const& field : m_fields) {
        ret += field->to_string() + " ";
    }
    ret += "}";
    return ret;
}

std::string StructDefinition::attributes() const
{
    return format(R"(name="{}")", name());
}

std::string const& StructDefinition::name() const
{
    return m_name;
}

Identifiers const& StructDefinition::fields() const
{
    return m_fields;
}

// -- EnumValue -------------------------------------------------------------

EnumValue::EnumValue(Token token, std::string label, std::optional<long> value)
    : SyntaxNode(std::move(token))
    , m_value(std::move(value))
    , m_label(std::move(label))
{
}

std::optional<long> const& EnumValue::value() const
{
    return m_value;
}

std::string const& EnumValue::label() const
{
    return m_label;
}

std::string EnumValue::attributes() const
{
    if (value().has_value())
        return format(R"(label="{}" value="{}")", label(), value().value());
    return format(R"(label="{}")", label());
}

std::string EnumValue::to_string() const
{
    if (value().has_value())
        return format(R"({}: {})", label(), value().value());
    return label();
}

// -- EnumDef ---------------------------------------------------------------

EnumDef::EnumDef(Token token, std::string name, EnumValues values, bool extend)
    : Statement(std::move(token))
    , m_name(std::move(name))
    , m_values(std::move(values))
    , m_extend(extend)
{
}

std::string const& EnumDef::name() const
{
    return m_name;
}

EnumValues const& EnumDef::values() const
{
    return m_values;
}

bool EnumDef::extend() const
{
    return m_extend;
}

Nodes EnumDef::children() const
{
    Nodes ret;
    for (auto const& value : m_values) {
        ret.push_back(value);
    }
    return ret;
}
std::string EnumDef::attributes() const
{
    return format(R"(name="{}" extend="{}")", name(), extend());
}

std::string EnumDef::to_string() const
{
    auto ret = format("enum {}{} {{", (extend()) ? "extend " : "", name());
    for (auto const& value : m_values) {
        ret += value->to_string();
        ret += ", ";
    }
    ret += '}';
    return ret;
}

}
