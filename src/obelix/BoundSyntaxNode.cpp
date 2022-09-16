/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/BoundSyntaxNode.h>

namespace Obelix {

// -- BoundIfStatement ------------------------------------------------------

BoundIfStatement::BoundIfStatement(std::shared_ptr<IfStatement> const& if_stmt, BoundBranches branches, std::shared_ptr<Statement> else_stmt)
    : Statement(if_stmt->token())
    , m_branches(std::move(branches))
    , m_else(std::move(else_stmt))
{
}

BoundIfStatement::BoundIfStatement(Token token, BoundBranches branches, std::shared_ptr<Statement> else_stmt)
    : Statement(std::move(token))
    , m_branches(std::move(branches))
    , m_else(std::move(else_stmt))
{
}

SyntaxNodeType BoundIfStatement::node_type() const
{
    return SyntaxNodeType::BoundIfStatement;
}

Nodes BoundIfStatement::children() const
{
    Nodes ret;
    for (auto& branch : m_branches) {
        ret.push_back(branch);
    }
    return ret;
}

std::string BoundIfStatement::to_string() const
{
    std::string ret;
    bool first;
    for (auto& branch : m_branches) {
        if (first)
            ret = branch->to_string();
        else
            ret += "el" + branch->to_string();
        first = false;
    }
    if (m_else)
        ret += "else\n" + m_else->to_string();
    return ret;
}

BoundBranches const& BoundIfStatement::branches() const
{
    return m_branches;
}

// -- BoundIntLiteral -------------------------------------------------------

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
    return nullptr;
}

// -- BoundWhileStatement ---------------------------------------------------

BoundWhileStatement::BoundWhileStatement(std::shared_ptr<SyntaxNode> const& node, std::shared_ptr<BoundExpression> condition, std::shared_ptr<Statement> stmt)
    : Statement(node->token())
    , m_condition(move(condition))
    , m_stmt(move(stmt))
{
}

SyntaxNodeType BoundWhileStatement::node_type() const
{
    return SyntaxNodeType::BoundWhileStatement;
}

Nodes BoundWhileStatement::children() const
{
    return { m_condition, m_stmt };
}

std::shared_ptr<BoundExpression> const& BoundWhileStatement::condition() const
{
    return m_condition;
}

std::shared_ptr<Statement> const& BoundWhileStatement::statement() const
{
    return m_stmt;
}

std::string BoundWhileStatement::to_string() const
{
    return format("while ({})\n{}", m_condition->to_string(), m_stmt->to_string());
}

}
