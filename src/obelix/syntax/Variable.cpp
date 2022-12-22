/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/syntax/Variable.h>

namespace Obelix {

extern_logging_category(parser);

// -- VariableDeclaration ---------------------------------------------------

VariableDeclaration::VariableDeclaration(Span location, std::shared_ptr<Identifier> identifier, std::shared_ptr<Expression> expr, bool constant)
    : Statement(std::move(location))
    , m_identifier(std::move(identifier))
    , m_const(constant)
    , m_expression(std::move(expr))
{
}

std::string VariableDeclaration::attributes() const
{
    return format(R"(name="{}" type="{}" is_const="{}")", name(), type(), is_const());
}

Nodes VariableDeclaration::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string VariableDeclaration::to_string() const
{
    auto ret = format("{} {}: {}", (is_const()) ? "const" : "var", name(), type());
    if (m_expression)
        ret += format(" = {}", m_expression->to_string());
    return ret;
}

std::shared_ptr<Identifier> const& VariableDeclaration::identifier() const
{
    return m_identifier;
}

std::string const& VariableDeclaration::name() const
{
    return m_identifier->name();
}

std::shared_ptr<ExpressionType> const& VariableDeclaration::type() const
{
    return m_identifier->type();
}

bool VariableDeclaration::is_typed() const
{
    return type() != nullptr;
}

bool VariableDeclaration::is_const() const
{
    return m_const;
}

std::shared_ptr<Expression> const& VariableDeclaration::expression() const
{
    return m_expression;
}

// -- StaticVariableDeclaration ---------------------------------------------

StaticVariableDeclaration::StaticVariableDeclaration(Span location, std::shared_ptr<Identifier> identifier, std::shared_ptr<Expression> expr, bool constant)
    : VariableDeclaration(std::move(location), std::move(identifier), std::move(expr), constant)
{
}

std::string StaticVariableDeclaration::to_string() const
{
    return format("static {}", VariableDeclaration::to_string());
}

// -- LocalVariableDeclaration ----------------------------------------------

LocalVariableDeclaration::LocalVariableDeclaration(Span location, std::shared_ptr<Identifier> identifier, std::shared_ptr<Expression> expr, bool constant)
    : VariableDeclaration(std::move(location), std::move(identifier), std::move(expr), constant)
{
}

std::string LocalVariableDeclaration::to_string() const
{
    return format("local {}", VariableDeclaration::to_string());
}

// -- GlobalVariableDeclaration ---------------------------------------------

GlobalVariableDeclaration::GlobalVariableDeclaration(Span location, std::shared_ptr<Identifier> identifier, std::shared_ptr<Expression> expr, bool constant)
    : VariableDeclaration(std::move(location), std::move(identifier), std::move(expr), constant)
{
}

std::string GlobalVariableDeclaration::to_string() const
{
    return format("global {}", VariableDeclaration::to_string());
}

}
