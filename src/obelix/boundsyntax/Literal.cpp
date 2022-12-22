/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/StringUtil.h>
#include <obelix/boundsyntax/Literal.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundLiteral ----------------------------------------------------------

BoundLiteral::BoundLiteral(Token token, pObjectType type)
    : BoundExpression(token.location(), std::move(type))
    , m_token(std::move(token))
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

Token const& BoundLiteral::token() const
{
    return m_token;
}

// -- BoundIntLiteral -------------------------------------------------------

BoundIntLiteral::BoundIntLiteral(std::shared_ptr<IntLiteral> const& literal, std::shared_ptr<ObjectType> type)
    : BoundLiteral(literal->token(), (type) ? std::move(type) : ObjectType::get("s64"))
{
    auto int_maybe = token_value<long>(literal->token());
    if (int_maybe.is_error())
        fatal("Error instantiating BoundIntLiteral: {}", int_maybe.error());
    m_int = int_maybe.value();
}

BoundIntLiteral::BoundIntLiteral(std::shared_ptr<BoundIntLiteral> const& literal, std::shared_ptr<ObjectType> type)
    : BoundLiteral(literal->token(), (type) ? std::move(type) : ObjectType::get("s64"))
{
}

BoundIntLiteral::BoundIntLiteral(Span location, long value, std::shared_ptr<ObjectType> type)
    : BoundLiteral(Token { location, TokenCode::Integer, Obelix::to_string(value) }, std::move(type))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, unsigned long value, std::shared_ptr<ObjectType> type)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, std::move(type))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, long value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("s64"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, unsigned long value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("u64"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, int value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("s32"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, unsigned value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("u32"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, short value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("s16"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, unsigned char value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("u8"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, signed char value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("s8"))
    , m_int(value)
{
}

BoundIntLiteral::BoundIntLiteral(Span location, unsigned short value)
    : BoundLiteral(Token { std::move(location), TokenCode::Integer, Obelix::to_string(value) }, ObjectType::get("u16"))
    , m_int(value)
{
}

ErrorOr<std::shared_ptr<BoundIntLiteral>, SyntaxError> BoundIntLiteral::cast(std::shared_ptr<ObjectType> const& target_type) const
{
    auto can_cast = type()->can_cast_to(target_type);
    if (can_cast == CanCast::Always || can_cast == CanCast::Sometimes) {
        switch (target_type->size()) {
        case 1: {
            auto char_maybe = token_value<char>(token());
            if (char_maybe.has_value())
                return std::make_shared<BoundIntLiteral>(location(), char_maybe.value());
            return char_maybe.error();
        }
        case 2: {
            auto short_maybe = token_value<short>(token());
            if (short_maybe.has_value())
                return std::make_shared<BoundIntLiteral>(location(), short_maybe.value());
            return short_maybe.error();
        }
        case 4: {
            auto int_maybe = token_value<int>(token());
            if (int_maybe.has_value())
                return std::make_shared<BoundIntLiteral>(location(), int_maybe.value());
            return int_maybe.error();
        }
        case 8: {
            auto long_probably = token_value<long>(token());
            assert(long_probably.has_value());
            return std::make_shared<BoundIntLiteral>(location(), long_probably.value());
        }
        default:
            fatal("Unexpected int size {}", target_type->size());
        }
    }
    return SyntaxError { location(), "Cannot cast literal value {} of type {} to type {}", token().value(), type(), target_type };
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
    : BoundStringLiteral(literal->location(), literal->token().value())
{
}

BoundStringLiteral::BoundStringLiteral(Span location, std::string value)
    : BoundLiteral(Token { std::move(location), TokenCode::DoubleQuotedString, value }, get_type<std::string>())
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
    : BoundBooleanLiteral(literal->location(), token_value<bool>(literal->token()).value())
{
}

BoundBooleanLiteral::BoundBooleanLiteral(Span location, bool value)
    : BoundLiteral(Token { std::move(location), TokenCode::Identifier, Obelix::to_string(value) }, get_type<bool>())
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

BoundTypeLiteral::BoundTypeLiteral(Span location, std::shared_ptr<ObjectType> type)
    : BoundLiteral(Token { std::move(location), TokenCode::Identifier, type->name() }, get_type<ObjectType>())
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

BoundEnumValue::BoundEnumValue(Span location, std::shared_ptr<ObjectType> enum_type, std::string label, long value)
    : BoundExpression(std::move(location), std::move(enum_type))
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
    : BoundExpression(variable->location(), PrimitiveType::Module)
    , m_name(variable->name())
{
}

BoundModuleLiteral::BoundModuleLiteral(Span location, std::string name)
    : BoundExpression(std::move(location), PrimitiveType::Module)
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
