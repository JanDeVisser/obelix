/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Logging.h>
#include <obelix/BoundFunction.h>
#include <obelix/Parser.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

int SyntaxNode::s_current_id = 0;
int Label::m_current_id = 0;

SyntaxNode::SyntaxNode()
{
}

SyntaxNode::SyntaxNode(SyntaxNode const& ancestor)
{
}


Label::Label(std::shared_ptr<Goto> const& goto_stmt)
    : Statement()
    , m_label_id(goto_stmt->label_id())
{
}

ErrorOr<std::shared_ptr<SyntaxNode>> to_literal(std::shared_ptr<Expression> const& expr)
{
    auto obj_maybe = TRY(expr->to_object());
    if (obj_maybe.has_value())
        return std::make_shared<Literal>(obj_maybe.value());
    return expr;
};

ErrorOr<std::optional<Obj>> BinaryExpression::to_object() const
{
    auto right_maybe = TRY(rhs()->to_object());
    auto left_maybe = TRY(lhs()->to_object());
    if (right_maybe.has_value() && left_maybe.has_value() && (m_operator.code() != TokenCode::Equals)) {
        auto ret_maybe = left_maybe.value().evaluate(op().value(), right_maybe.value());
        if (!ret_maybe.has_value())
            return Error(ErrorCode::OperatorUnresolved, m_operator.value(), left_maybe.value());
        return ret_maybe.value();
    }
    return std::optional<Obj> {};
}

}
