/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/boundsyntax/Literal.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundLiteral ----------------------------------------------------------

BoundLiteral::BoundLiteral(Token token, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(token), std::move(type))
{
}

long BoundLiteral::int_value() const
{
    fatal("Called int_value() on '{}'", node_type());
}

std::string BoundLiteral::string_value() const
{
    fatal("Called string_value() on '{}'", node_type());
}

bool BoundLiteral::bool_value() const
{
    fatal("Called bool_value() on '{}'", node_type());
}

// -- BoundIntLiteral -------------------------------------------------------

BoundIntLiteral::BoundIntLiteral(std::shared_ptr<IntLiteral> const& literal, std::shared_ptr<ObjectType> type)
    : BoundLiteral(literal->token(), (type) ? std::move(type) : ObjectType::get("s64"))
{
    auto int_maybe = token_value<long>(token());
    if (int_maybe.is_error())
        fatal("Error instantiating BoundIntLiteral: {}", int_maybe.error());
    m_int = int_maybe.value();
}

BoundIntLiteral::BoundIntLiteral(std::shared_ptr<BoundIntLiteral> const& literal, std::shared_ptr<ObjectType> type)
    : BoundLiteral(literal->token(), (type) ? std::move(type) : ObjectType::get("s64"))
{
}

BoundIntLiteral::BoundIntLiteral(Token t, long value, std::shared_ptr<ObjectType> type)
    : BoundLiteral(std::move(t), std::move(type))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, unsigned long value, std::shared_ptr<ObjectType> type)
    : BoundLiteral(std::move(t), std::move(type))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, long value)
    : BoundLiteral(std::move(t), ObjectType::get("s64"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, unsigned long value)
    : BoundLiteral(std::move(t), ObjectType::get("u64"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, int value)
    : BoundLiteral(std::move(t), ObjectType::get("s32"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, unsigned value)
    : BoundLiteral(std::move(t), ObjectType::get("u32"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, short value)
    : BoundLiteral(std::move(t), ObjectType::get("s16"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, unsigned char value)
    : BoundLiteral(std::move(t), ObjectType::get("u8"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, signed char value)
    : BoundLiteral(std::move(t), ObjectType::get("s8"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Token t, unsigned short value)
    : BoundLiteral(std::move(t), ObjectType::get("u16"))
    , m_int(value)
{
}

ErrorOr<std::shared_ptr<BoundIntLiteral>, SyntaxError> BoundIntLiteral::cast(std::shared_ptr<ObjectType> const& type) const
{
    switch (type->size()) {
    case 1: {
        auto char_maybe = token_value<char>(token());
        if (char_maybe.has_value())
            return std::make_shared<BoundIntLiteral>(token(), char_maybe.value());
        return char_maybe.error();
    }
    case 2: {
        auto short_maybe = token_value<short>(token());
        if (short_maybe.has_value())
            return std::make_shared<BoundIntLiteral>(token(), short_maybe.value());
        return short_maybe.error();
    }
    case 4: {
        auto int_maybe = token_value<int>(token());
        if (int_maybe.has_value())
            return std::make_shared<BoundIntLiteral>(token(), int_maybe.value());
        return int_maybe.error();
    }
    case 8: {
        auto long_probably = token_value<long>(token());
        assert(long_probably.has_value());
        return std::make_shared<BoundIntLiteral>(token(), long_probably.value());
    }
    default:
        fatal("Unexpected int size {}", type->size());
    }
}

std::string BoundIntLiteral::attributes() const
{
    return format(R"(value="{}" type="{}")", value(), type());
}

std::string BoundIntLiteral::to_string() const
{
    return format("{}: {}", value(), type());
}

long BoundIntLiteral::int_value() const
{
    return value<long>();
}

// -- BoundStringLiteral ----------------------------------------------------

BoundStringLiteral::BoundStringLiteral(std::shared_ptr<StringLiteral> const& literal)
    : BoundStringLiteral(literal->token(), literal->token().value())
{
}

BoundStringLiteral::BoundStringLiteral(Token t, std::string value)
    : BoundLiteral(std::move(t), get_type<std::string>())
    , m_string(std::move(value))
{
}

std::string BoundStringLiteral::attributes() const
{
    return format(R"(value="{}" type="{}")", value(), type());
}

std::string BoundStringLiteral::to_string() const
{
    return format("{}: {}", value(), type());
}

std::string const& BoundStringLiteral::value() const
{
    return m_string;
}

std::string BoundStringLiteral::string_value() const
{
    return value();
}

// -- BoundBooleanLiteral ---------------------------------------------------

BoundBooleanLiteral::BoundBooleanLiteral(std::shared_ptr<BooleanLiteral> const& literal)
    : BoundBooleanLiteral(literal->token(), token_value<bool>(literal->token()).value())
{
}

BoundBooleanLiteral::BoundBooleanLiteral(Token t, bool value)
    : BoundLiteral(std::move(t), get_type<bool>())
    , m_value(value)
{
}

std::string BoundBooleanLiteral::attributes() const
{
    return format(R"(value="{}" type="{}")", value(), type());
}

std::string BoundBooleanLiteral::to_string() const
{
    return format("{}: {}", value(), type());
}

bool BoundBooleanLiteral::value() const
{
    return m_value;
}

bool BoundBooleanLiteral::bool_value() const
{
    return value();
}

// -- BoundTypeLiteral ------------------------------------------------------

BoundTypeLiteral::BoundTypeLiteral(Token t, std::shared_ptr<ObjectType> type)
    : BoundLiteral(std::move(t), get_type<ObjectType>())
    , m_type_value(std::move(type))
{
}

std::string BoundTypeLiteral::attributes() const
{
    return format(R"(type="{}")", type());
}

std::string BoundTypeLiteral::to_string() const
{
    return type()->to_string();
}

std::shared_ptr<ObjectType> const& BoundTypeLiteral::value() const
{
    return m_type_value;
}

// -- BoundEnumValue --------------------------------------------------------

BoundEnumValue::BoundEnumValue(Token token, std::shared_ptr<ObjectType> enum_type, std::string label, long value)
    : BoundExpression(std::move(token), std::move(enum_type))
    , m_value(value)
    , m_label(std::move(label))
{
}

long const& BoundEnumValue::value() const
{
    return m_value;
}

std::string const& BoundEnumValue::label() const
{
    return m_label;
}

std::string BoundEnumValue::attributes() const
{
    return format(R"(label="{}" value="{}")", label(), value());
}

std::string BoundEnumValue::to_string() const
{
    return format(R"({}: {})", label(), value());
}

// -- BoundModuleLiteral ----------------------------------------------------

BoundModuleLiteral::BoundModuleLiteral(std::shared_ptr<Variable> const& variable)
    : BoundExpression(variable->token(), PrimitiveType::Module)
    , m_name(variable->name())
{
}

BoundModuleLiteral::BoundModuleLiteral(Token token, std::string name)
    : BoundExpression(std::move(token), PrimitiveType::Module)
    , m_name(std::move(name))
{
}

std::string const& BoundModuleLiteral::name() const
{
    return m_name;
}

std::string BoundModuleLiteral::attributes() const
{
    return format(R"(name="{}")", m_name);
}

std::string BoundModuleLiteral::to_string() const
{
    return format("module {}", m_name);
}

}
