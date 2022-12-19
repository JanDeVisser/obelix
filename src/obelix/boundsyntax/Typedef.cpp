/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/Typedef.h>
#include <obelix/boundsyntax/Typedef.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundType -------------------------------------------------------------

BoundType::BoundType(Token token, std::shared_ptr<ObjectType> type)
    : SyntaxNode(std::move(token))
    , m_type(std::move(type))
{
}

std::shared_ptr<ObjectType> BoundType::type() const
{
    return m_type;
}

std::string BoundType::attributes() const
{
    return format(R"(name="{}" type="{}")", type()->name(), type()->to_string());
}

std::string BoundType::to_string() const
{
    return m_type->to_string();
}

// -- BoundStructDefinition -------------------------------------------------

BoundStructDefinition::BoundStructDefinition(pStructDefinition const& struct_def, pObjectType type, BoundIdentifiers fields, Statements methods)
    : BoundStatement(struct_def->token())
    , m_name(struct_def->name())
    , m_type(std::move(type))
    , m_fields(std::move(fields))
    , m_methods(std::move(methods))
{
}

BoundStructDefinition::BoundStructDefinition(Token token, pObjectType type, BoundIdentifiers fields, Statements methods)
    : BoundStatement(std::move(token))
    , m_name(type->name())
    , m_type(std::move(type))
    , m_fields(std::move(fields))
    , m_methods(std::move(methods))
{
}

std::string const& BoundStructDefinition::name() const
{
    return m_name;
}

std::shared_ptr<ObjectType> BoundStructDefinition::type() const
{
    return m_type;
}

std::string BoundStructDefinition::attributes() const
{
    return format(R"(name="{}")", name());
}

Nodes BoundStructDefinition::children() const
{
    Nodes ret;
    ret.push_back(std::make_shared<NodeList<Statement>>("methods", m_methods));
    return ret;
}

std::string BoundStructDefinition::to_string() const
{
    auto ret = format("struct {} {", name());
    for (auto const& field : type()->fields()) {
        ret += " ";
        ret += format("{}: {}", field.name, field.type->to_string());
    }
    ret += " }";
    return ret;
}

BoundIdentifiers const& BoundStructDefinition::fields() const
{
    return m_fields;
}

Statements const& BoundStructDefinition::methods() const
{
    return m_methods;
}

bool BoundStructDefinition::is_fully_bound() const
{
    return std::all_of(m_methods.begin(), m_methods.end(),
            [](pStatement const& stmt) {
                return stmt->is_fully_bound();
            });
}

// -- BoundEnumValueDef -----------------------------------------------------

BoundEnumValueDef::BoundEnumValueDef(Token token, std::string label, long value)
    : SyntaxNode(std::move(token))
    , m_value(value)
    , m_label(std::move(label))
{
}

long const& BoundEnumValueDef::value() const
{
    return m_value;
}

std::string const& BoundEnumValueDef::label() const
{
    return m_label;
}

std::string BoundEnumValueDef::attributes() const
{
    return format(R"(label="{}" value="{}")", label(), value());
}

std::string BoundEnumValueDef::to_string() const
{
    return format(R"({}: {})", label(), value());
}

// -- BoundEnumDef ----------------------------------------------------------

BoundEnumDef::BoundEnumDef(std::shared_ptr<EnumDef> const& enum_def, std::shared_ptr<ObjectType> type, BoundEnumValueDefs values)
    : BoundStatement(enum_def->token())
    , m_name(enum_def->name())
    , m_type(std::move(type))
    , m_values(std::move(values))
    , m_extend(enum_def->extend())
{
}

BoundEnumDef::BoundEnumDef(Token token, std::string name, std::shared_ptr<ObjectType> type, BoundEnumValueDefs values, bool extend)
    : BoundStatement(std::move(token))
    , m_name(std::move(name))
    , m_type(std::move(type))
    , m_values(std::move(values))
    , m_extend(extend)
{
}

std::string const& BoundEnumDef::name() const
{
    return m_name;
}

std::shared_ptr<ObjectType> BoundEnumDef::type() const
{
    return m_type;
}

BoundEnumValueDefs const& BoundEnumDef::values() const
{
    return m_values;
}

bool BoundEnumDef::extend() const
{
    return m_extend;
}

std::string BoundEnumDef::attributes() const
{
    return format(R"(name="{}")", name());
}

Nodes BoundEnumDef::children() const
{
    Nodes ret;
    for (auto const& value : m_values) {
        ret.push_back(value);
    }
    return ret;
}

std::string BoundEnumDef::to_string() const
{
    auto values = m_type->template_argument_values<NVP>("values");
    return format("enum {} { {} }", name(), join(values, ", ", [](NVP const& value) {
        return format("{}: {}", value.first, value.second);
    }));
}

// -- BoundTypeDef -------------------------------------------------------

BoundTypeDef::BoundTypeDef(Token token, std::string name, std::shared_ptr<BoundType> type)
    : BoundStatement(std::move(token))
    , m_name(std::move(name))
    , m_type(std::move(type))
{
}

std::string const& BoundTypeDef::name() const
{
    return m_name;
}

std::shared_ptr<BoundType> const& BoundTypeDef::type() const
{
    return m_type;
}

Nodes BoundTypeDef::children() const
{
    return Nodes { type() };
}

std::string BoundTypeDef::attributes() const
{
    return format(R"(name="{}")", name());
}

std::string BoundTypeDef::to_string() const
{
    return format("{}: {}", name(), type()->type()->to_string());
}

}
