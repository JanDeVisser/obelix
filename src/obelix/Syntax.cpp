/*
 * Syntax.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/Logging.h>
#include <obelix/BoundFunction.h>
#include <obelix/Parser.h>
#include <obelix/Syntax.h>

namespace Obelix {

extern_logging_category(parser);

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
