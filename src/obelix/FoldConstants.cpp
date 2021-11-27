/*
 * Processor.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

ErrorOrNode fold_constants(std::shared_ptr<SyntaxNode> const& tree)
{
    using FoldContext = Context<std::shared_ptr<Literal>>;
    FoldContext::ProcessorMap fold_constants_map;

    fold_constants_map[SyntaxNodeType::VariableDeclaration] = [](std::shared_ptr<SyntaxNode> const& tree, FoldContext &ctx) -> ErrorOrNode {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        if (var_decl->is_const() && var_decl->expression()->node_type() == SyntaxNodeType::Literal) {
            auto literal = std::dynamic_pointer_cast<Literal>(var_decl->expression());
            ctx.declare(var_decl->variable().identifier(), literal);
        }
        return tree;
    };

    fold_constants_map[SyntaxNodeType::BinaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, FoldContext &ctx) -> ErrorOrNode
    {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto obj_maybe = TRY(expr->to_object());
        if (obj_maybe.has_value()) {
            if (obj_maybe.value()->is_exception())
                return ptr_cast<Exception>(obj_maybe.value())->error();
            return std::make_shared<Literal>(obj_maybe.value());
        }

        auto left_maybe = TRY(expr->lhs()->to_object());
        auto right_maybe = TRY(expr->rhs()->to_object());
        if (!left_maybe.has_value() && expr->lhs()->node_type() == SyntaxNodeType::Identifier) {
            auto identifier = std::dynamic_pointer_cast<Identifier>(expr->lhs());
            if (auto value_maybe = ctx.get(identifier->name()); value_maybe.has_value()) {
                left_maybe = TRY(value_maybe.value()->to_object());
            }
        }

        if (left_maybe.has_value() && right_maybe.has_value() &&
            expr->op().code() != TokenCode::Equals && !Parser::is_assignment_operator(expr->op().code())) {
            auto result_maybe = left_maybe.value().evaluate(expr->op().value(), right_maybe.value());
            if (result_maybe.has_value()) {
                if (result_maybe.value()->is_exception())
                    return ptr_cast<Exception>(result_maybe.value())->error();
                return std::make_shared<Literal>(result_maybe.value());
            }
        }

        //
        // lhs is a constant and rhs is another binary (with the same precedence), and the
        // rhs of that binary is constant, we can fold as follows:
        //
        //     +                     +
        //    / \                 /     \
        //   c1   +      =====> c1+c2    x
        //       / \
        //     c2   x
        //
        // And similarly when rhs is constant, lhs is binary, and rhs of lhs is constant.
        //
        // FIXME: This implies that equals precedence implies that the operations are associative.
        // This is not necessarily true.
        //
        if (left_maybe.has_value() && (expr->rhs()->node_type() == SyntaxNodeType::BinaryExpression)) {
            auto rhs_expr = std::dynamic_pointer_cast<BinaryExpression>(expr->rhs());
            auto lhs_rhs_maybe = TRY(rhs_expr->lhs()->to_object());
            if (lhs_rhs_maybe.has_value() &&
                (Parser::binary_precedence(expr->op().code()) == Parser::binary_precedence(rhs_expr->op().code()))) {
                auto result_maybe = left_maybe.value().evaluate(expr->op().value(), lhs_rhs_maybe.value());
                if (result_maybe.has_value()) {
                    if (result_maybe.value()->is_exception())
                        return ptr_cast<Exception>(result_maybe.value())->error();
                    auto literal = std::make_shared<Literal>(result_maybe.value());
                    return std::make_shared<BinaryExpression>(literal, rhs_expr->op(), rhs_expr->rhs());
                }
            }
        }

        if (right_maybe.has_value() && (expr->lhs()->node_type() == SyntaxNodeType::BinaryExpression)) {
            auto lhs_expr = std::dynamic_pointer_cast<BinaryExpression>(expr->lhs());
            auto rhs_lhs_maybe = TRY(lhs_expr->rhs()->to_object());
            if (rhs_lhs_maybe.has_value() &&
                (Parser::binary_precedence(expr->op().code()) == Parser::binary_precedence(lhs_expr->op().code()))) {
                auto result_maybe = right_maybe.value()->evaluate(expr->op().value(), make_typed<Arguments>(rhs_lhs_maybe.value()));
                if (result_maybe.has_value()) {
                    if (result_maybe.value()->is_exception())
                        return ptr_cast<Exception>(result_maybe.value())->error();
                    auto literal = std::make_shared<Literal>(result_maybe.value());
                    return std::make_shared<BinaryExpression>(lhs_expr->lhs(), lhs_expr->op(), literal);
                }
            }
        }

        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        if (Parser::is_assignment_operator(expr->op().code()) && expr->lhs()->node_type() == SyntaxNodeType::Identifier) {
            auto id = std::dynamic_pointer_cast<Identifier>(expr->lhs());
            auto tok = expr->op().value();
            Token new_op(Parser::operator_for_assignment_operator(expr->op().code()), tok.substr(0, tok.length()-1));
            auto new_rhs = std::make_shared<BinaryExpression>(std::make_shared<Identifier>(id->name()), new_op, expr->rhs());
            return std::make_shared<BinaryExpression>(id, Token(TokenCode::Equals, "="), new_rhs);
        }

        return tree;
    };

    fold_constants_map[SyntaxNodeType::UnaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, FoldContext& ctx) -> ErrorOrNode
    {
        return to_literal(std::dynamic_pointer_cast<Expression>(tree));
    };

    FoldContext root(fold_constants_map);
    return process_tree(tree, root);
}

}
