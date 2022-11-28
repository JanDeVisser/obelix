/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/boundsyntax/Function.h>
#include <obelix/boundsyntax/Typedef.h>
#include <obelix/boundsyntax/Variable.h>

namespace Obelix {

extern_logging_category(parser);

// -- BoundVariableAccess ---------------------------------------------------

BoundVariableAccess::BoundVariableAccess(std::shared_ptr<Expression> reference, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(reference), std::move(type))
{
}

BoundVariableAccess::BoundVariableAccess(Token token, std::shared_ptr<ObjectType> type)
    : BoundExpression(std::move(token), std::move(type))
{
}

// -- BoundIdentifier -------------------------------------------------------

BoundIdentifier::BoundIdentifier(std::shared_ptr<Identifier> const& identifier, std::shared_ptr<ObjectType> type)
    : BoundVariableAccess(identifier, std::move(type))
    , m_identifier(identifier->name())
{
}

BoundIdentifier::BoundIdentifier(Token token, std::string identifier, std::shared_ptr<ObjectType> type)
    : BoundVariableAccess(std::move(token), std::move(type))
    , m_identifier(std::move(identifier))
{
}

std::string const& BoundIdentifier::name() const
{
    return m_identifier;
}

std::string BoundIdentifier::attributes() const
{
    return format(R"(name="{}" type="{}")", name(), type());
}

std::string BoundIdentifier::to_string() const
{
    return format("{}: {}", name(), type()->to_string());
}

std::string BoundIdentifier::qualified_name() const
{
    return name();
}

// -- BoundVariable ---------------------------------------------------------

BoundVariable::BoundVariable(std::shared_ptr<Variable> identifier, std::shared_ptr<ObjectType> type)
    : BoundIdentifier(std::move(identifier), std::move(type))
{
}

BoundVariable::BoundVariable(Token token, std::string name, std::shared_ptr<ObjectType> type)
    : BoundIdentifier(std::move(token), std::move(name), std::move(type))
{
}

// -- BoundMemberAccess -----------------------------------------------------

BoundMemberAccess::BoundMemberAccess(std::shared_ptr<BoundExpression> strukt, std::shared_ptr<BoundIdentifier> member)
    : BoundVariableAccess(strukt->token(), member->type())
    , m_struct(std::move(strukt))
    , m_member(std::move(member))
{
}

std::shared_ptr<BoundExpression> const& BoundMemberAccess::structure() const
{
    return m_struct;
}

std::shared_ptr<BoundIdentifier> const& BoundMemberAccess::member() const
{
    return m_member;
}

std::string BoundMemberAccess::qualified_name() const
{
    return format("{}.{}", structure()->qualified_name(), member()->name());
}

std::string BoundMemberAccess::attributes() const
{
    return format(R"(type="{}")", type());
}

Nodes BoundMemberAccess::children() const
{
    return { m_struct, m_member };
}

std::string BoundMemberAccess::to_string() const
{
    return format("{}.{}: {}", structure(), member(), type_name());
}

// -- UnboundMemberAccess -----------------------------------------------------

UnboundMemberAccess::UnboundMemberAccess(std::shared_ptr<BoundExpression> strukt, std::shared_ptr<Variable> member)
    : Expression(strukt->token(), member->type())
    , m_struct(std::move(strukt))
    , m_member(std::move(member))
{
}

std::shared_ptr<BoundExpression> const& UnboundMemberAccess::structure() const
{
    return m_struct;
}

std::shared_ptr<Variable> const& UnboundMemberAccess::member() const
{
    return m_member;
}

std::string UnboundMemberAccess::attributes() const
{
    return format(R"(type="{}")", type());
}

Nodes UnboundMemberAccess::children() const
{
    return { m_struct, m_member };
}

std::string UnboundMemberAccess::to_string() const
{
    return format("{}.{}: {}", structure(), member(), type_name());
}

// -- BoundMemberAssignment -------------------------------------------------

BoundMemberAssignment::BoundMemberAssignment(std::shared_ptr<BoundExpression> strukt, std::shared_ptr<BoundIdentifier> member)
    : BoundMemberAccess(std::move(strukt), std::move(member))
{
}

// -- BoundArrayAccess ------------------------------------------------------

BoundArrayAccess::BoundArrayAccess(std::shared_ptr<BoundExpression> array, std::shared_ptr<BoundExpression> index, std::shared_ptr<ObjectType> type)
    : BoundVariableAccess(array->token(), std::move(type))
    , m_array(std::move(array))
    , m_index(std::move(index))
{
}

std::shared_ptr<BoundExpression> const& BoundArrayAccess::array() const
{
    return m_array;
}

std::shared_ptr<BoundExpression> const& BoundArrayAccess::subscript() const
{
    return m_index;
}

std::string BoundArrayAccess::attributes() const
{
    return format(R"(type="{}")", type());
}

Nodes BoundArrayAccess::children() const
{
    return { m_array, m_index };
}

std::string BoundArrayAccess::to_string() const
{
    return format("{}[{}]: {}", array(), subscript(), type_name());
}

std::string BoundArrayAccess::qualified_name() const
{
    return format("{}[{}]", array()->qualified_name(), subscript());
}

// -- BoundVariableDeclaration ----------------------------------------------

BoundVariableDeclaration::BoundVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : BoundStatement(decl->token())
    , m_variable(std::move(variable))
    , m_const(decl->is_const())
    , m_expression(std::move(expr))
{
}

BoundVariableDeclaration::BoundVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : BoundStatement(std::move(token))
    , m_variable(std::move(variable))
    , m_const(is_const)
    , m_expression(std::move(expr))
{
}

std::string BoundVariableDeclaration::attributes() const
{
    return format(R"(name="{}" type="{}" is_const="{}")", name(), type(), is_const());
}

Nodes BoundVariableDeclaration::children() const
{
    if (m_expression)
        return { m_expression };
    return {};
}

std::string BoundVariableDeclaration::to_string() const
{
    auto ret = format("{} {}: {}", (is_const()) ? "const" : "var", name(), type());
    if (m_expression)
        ret += format(" = {}", m_expression->to_string());
    return ret;
}

std::string const& BoundVariableDeclaration::name() const
{
    return m_variable->name();
}

std::shared_ptr<ObjectType> BoundVariableDeclaration::type() const
{
    return m_variable->type();
}

bool BoundVariableDeclaration::is_const() const
{
    return m_const;
}

bool BoundVariableDeclaration::is_static() const
{
    return false;
}

std::shared_ptr<BoundExpression> const& BoundVariableDeclaration::expression() const
{
    return m_expression;
}

std::shared_ptr<BoundIdentifier> const& BoundVariableDeclaration::variable() const
{
    return m_variable;
}

// -- BoundStaticVariableDeclaration ----------------------------------------

BoundStaticVariableDeclaration::BoundStaticVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(decl, std::move(variable), std::move(expr))
{
}

BoundStaticVariableDeclaration::BoundStaticVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(std::move(token), std::move(variable), is_const, std::move(expr))
{
}

std::string BoundStaticVariableDeclaration::to_string() const
{
    return "static " + BoundVariableDeclaration::to_string();
}

bool BoundStaticVariableDeclaration::is_static() const
{
    return true;
}

// -- BoundLocalVariableDeclaration -----------------------------------------

BoundLocalVariableDeclaration::BoundLocalVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(decl, std::move(variable), std::move(expr))
{
}

BoundLocalVariableDeclaration::BoundLocalVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(std::move(token), std::move(variable), is_const, std::move(expr))
{
}

std::string BoundLocalVariableDeclaration::to_string() const
{
    return "local " + BoundVariableDeclaration::to_string();
}

// -- BoundGlobalVariableDeclaration ----------------------------------------

BoundGlobalVariableDeclaration::BoundGlobalVariableDeclaration(std::shared_ptr<VariableDeclaration> const& decl, std::shared_ptr<BoundIdentifier> variable, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(decl, std::move(variable), std::move(expr))
{
}

BoundGlobalVariableDeclaration::BoundGlobalVariableDeclaration(Token token, std::shared_ptr<BoundIdentifier> variable, bool is_const, std::shared_ptr<BoundExpression> expr)
    : BoundVariableDeclaration(std::move(token), std::move(variable), is_const, std::move(expr))
{
}

std::string BoundGlobalVariableDeclaration::to_string() const
{
    return "global " + BoundVariableDeclaration::to_string();
}

bool BoundGlobalVariableDeclaration::is_static() const
{
    return false;
}

// -- BoundAssignment -------------------------------------------------------

BoundAssignment::BoundAssignment(Token token, std::shared_ptr<BoundVariableAccess> assignee, std::shared_ptr<BoundExpression> expression)
    : BoundExpression(std::move(token), assignee->type())
    , m_assignee(std::move(assignee))
    , m_expression(std::move(expression))
{
    debug(parser, "m_identifier->type() = {} m_expression->type() = {}", m_assignee->type(), m_expression->type());
    assert(m_expression->type()->is_assignable_to(m_assignee->type()));
}

std::shared_ptr<BoundVariableAccess> const& BoundAssignment::assignee() const
{
    return m_assignee;
}

std::shared_ptr<BoundExpression> const& BoundAssignment::expression() const
{
    return m_expression;
}

Nodes BoundAssignment::children() const
{
    return { assignee(), expression() };
}

std::string BoundAssignment::to_string() const
{
    return format("{} = {}", assignee()->to_string(), expression()->to_string());
}

}
