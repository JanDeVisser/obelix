/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Logging.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/BoundFunction.h>
#include <obelix/Parser.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

int Label::m_current_id = 0;

SyntaxNode::SyntaxNode(Token token)
    : m_token(std::move(token))
{
}

Label::Label(std::shared_ptr<Goto> const& goto_stmt)
    : Statement(goto_stmt->token())
    , m_label_id(goto_stmt->label_id())
{
}

ErrorOr<std::optional<Obj>> BoundBinaryExpression::to_object() const
{
    auto right_maybe = TRY(rhs()->to_object());
    auto left_maybe = TRY(lhs()->to_object());
    if (right_maybe.has_value() && left_maybe.has_value() && BinaryOperator_is_assignment(m_operator)) {
        auto ret_maybe = left_maybe.value().evaluate(BinaryOperator_name(op()), right_maybe.value());
        if (!ret_maybe.has_value())
            return Error(ErrorCode::OperatorUnresolved, BinaryOperator_name(op()), left_maybe.value());
        return ret_maybe.value();
    }
    return std::optional<Obj> {};
}

ErrorOr<std::optional<Obj>> BoundUnaryExpression::to_object() const
{
    auto operand = TO_OBJECT(m_operand);
    if (operand->is_exception())
        return operand;
    auto ret_maybe = operand->evaluate(UnaryOperator_name(m_operator));
    if (!ret_maybe.has_value())
        return make_obj<Exception>(ErrorCode::OperatorUnresolved, UnaryOperator_name(m_operator), operand);
    return ret_maybe.value();
}

}
