/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

using FoldContext = Context<std::shared_ptr<Literal>>;

ErrorOrNode fold_constants_processor(std::shared_ptr<SyntaxNode> const& tree, FoldContext& ctx)
{
    if (!tree)
        return tree;

    switch (tree->node_type()) {
    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, fold_constants_processor(var_decl->expression(), ctx));
        if (var_decl->is_const() && expr->node_type() == SyntaxNodeType::Literal) {
            auto literal = std::dynamic_pointer_cast<Literal>(expr);
            ctx.declare(var_decl->variable().identifier(), literal);
        }
        if (expr != var_decl->expression())
            return std::make_shared<VariableDeclaration>(var_decl->variable(), expr, var_decl->is_const());
        return tree;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto obj_maybe = TRY(expr->to_object());
        if (obj_maybe.has_value()) {
            if (obj_maybe.value()->is_exception())
                return ptr_cast<Exception>(obj_maybe.value())->error();
            return std::make_shared<Literal>(obj_maybe.value());
        }

        auto lhs = TRY_AND_CAST(Expression, fold_constants_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, fold_constants_processor(expr->rhs(), ctx));
        auto left_maybe = TRY(lhs->to_object());
        auto right_maybe = TRY(rhs->to_object());
        if (!left_maybe.has_value() && lhs->node_type() == SyntaxNodeType::Identifier) {
            auto identifier = std::dynamic_pointer_cast<Identifier>(lhs);
            if (auto value_maybe = ctx.get(identifier->name()); value_maybe.has_value()) {
                left_maybe = TRY(value_maybe.value()->to_object());
            }
        }
        if (!right_maybe.has_value() && rhs->node_type() == SyntaxNodeType::Identifier) {
            auto identifier = std::dynamic_pointer_cast<Identifier>(rhs);
            if (auto value_maybe = ctx.get(identifier->name()); value_maybe.has_value()) {
                right_maybe = TRY(value_maybe.value()->to_object());
            }
        }

        if (left_maybe.has_value() && right_maybe.has_value() && expr->op().code() != TokenCode::Equals && expr->op().code() != Parser::KeywordRange && !Parser::is_assignment_operator(expr->op().code())) {
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
        if (left_maybe.has_value() && (rhs->node_type() == SyntaxNodeType::BinaryExpression)) {
            auto rhs_expr = std::dynamic_pointer_cast<BinaryExpression>(rhs);
            auto lhs_rhs_maybe = TRY(rhs_expr->lhs()->to_object());
            if (lhs_rhs_maybe.has_value() && (Parser::binary_precedence(expr->op().code()) == Parser::binary_precedence(rhs_expr->op().code()))) {
                auto result_maybe = left_maybe.value().evaluate(expr->op().value(), lhs_rhs_maybe.value());
                if (result_maybe.has_value()) {
                    if (result_maybe.value()->is_exception())
                        return ptr_cast<Exception>(result_maybe.value())->error();
                    auto literal = std::make_shared<Literal>(result_maybe.value());
                    return std::make_shared<BinaryExpression>(literal, rhs_expr->op(), rhs_expr->rhs());
                }
            }
        }

        if (right_maybe.has_value() && (lhs->node_type() == SyntaxNodeType::BinaryExpression)) {
            auto lhs_expr = std::dynamic_pointer_cast<BinaryExpression>(lhs);
            auto rhs_lhs_maybe = TRY(lhs_expr->rhs()->to_object());
            if (rhs_lhs_maybe.has_value() && (Parser::binary_precedence(expr->op().code()) == Parser::binary_precedence(lhs_expr->op().code()))) {
                auto result_maybe = right_maybe.value()->evaluate(expr->op().value(), make_typed<Arguments>(rhs_lhs_maybe.value()));
                if (result_maybe.has_value()) {
                    if (result_maybe.value()->is_exception())
                        return ptr_cast<Exception>(result_maybe.value())->error();
                    auto literal = std::make_shared<Literal>(result_maybe.value());
                    return std::make_shared<BinaryExpression>(lhs_expr->lhs(), lhs_expr->op(), literal);
                }
            }
        }

        return std::make_shared<BinaryExpression>(lhs, expr->op(), rhs, expr->type());
    }

    case SyntaxNodeType::UnaryExpression: {
        return to_literal(std::dynamic_pointer_cast<Expression>(tree));
    }

    case SyntaxNodeType::Branch: {
        auto elif = std::dynamic_pointer_cast<Branch>(tree);
        auto cond = TRY_AND_CAST(Expression, fold_constants_processor(elif->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, fold_constants_processor(elif->statement(), ctx));
        if (cond == nullptr)
            return stmt;

        if (cond->node_type() == SyntaxNodeType::Literal) {
            auto cond_literal = std::dynamic_pointer_cast<Literal>(cond);
            auto cond_obj = cond_literal->to_object().value();
            if (cond_obj) {
                return stmt;
            } else {
                return nullptr;
            }
        }
        return std::make_shared<Branch>(cond, stmt);
    }

    case SyntaxNodeType::IfStatement: {
        auto stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        Statements branches;
        for (auto const& branch : stmt->branches()) {
            auto b = TRY_AND_CAST(Statement, fold_constants_processor(branch, ctx));
            if (b)
                branches.push_back(b);
        }

        bool constant_true_added { false };
        Branches new_branches;
        for (auto const& branch : branches) {
            switch (branch->node_type()) {
            case SyntaxNodeType::Branch: {
                new_branches.push_back(std::dynamic_pointer_cast<Branch>(branch));
                break;
            }
            default:
                // This is a constant-true branch. If we're not building a new
                // statement, return this statement. Else add this if it's the
                // first constant true branch:
                if (new_branches.empty())
                    return branch;
                if (!constant_true_added) {
                    new_branches.push_back(std::make_shared<Branch>(nullptr, branch));
                    constant_true_added = true;
                }
                break;
            }
        }

        // Nothing left. Everything was false:
        if (new_branches.empty())
            return std::make_shared<Pass>();

        return std::make_shared<IfStatement>(new_branches);
    }
    default:
        return process_tree(tree, ctx, fold_constants_processor);
    }
}

ErrorOrNode fold_constants(std::shared_ptr<SyntaxNode> const& tree)
{
    FoldContext root;
    return fold_constants_processor(tree, root);
}

}
