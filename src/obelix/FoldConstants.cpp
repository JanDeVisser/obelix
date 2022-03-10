/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

using FoldContext = Context<std::shared_ptr<BoundLiteral>>;

ErrorOrNode fold_constants_processor(std::shared_ptr<SyntaxNode> const& tree, FoldContext& ctx)
{
    if (!tree)
        return tree;
    debug(parser, "fold_constants_processor({} = {})", tree->node_type(), tree);

    switch (tree->node_type()) {
    case SyntaxNodeType::BoundVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, fold_constants_processor(var_decl->expression(), ctx));
        if (var_decl->is_const() && expr->node_type() == SyntaxNodeType::BoundLiteral) {
            auto literal = std::dynamic_pointer_cast<BoundLiteral>(expr);
            ctx.declare(var_decl->name(), literal);
            return make_node<Pass>(var_decl);
        }
        return make_node<BoundVariableDeclaration>(var_decl->token(), var_decl->variable(), var_decl->is_const(), expr);
    }

    case SyntaxNodeType::BoundBinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
        auto lhs = TRY_AND_CAST(BoundExpression, fold_constants_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(BoundExpression, fold_constants_processor(expr->rhs(), ctx));

        if (lhs->node_type() == SyntaxNodeType::BoundLiteral && rhs->node_type() == SyntaxNodeType::BoundLiteral && expr->op() != BinaryOperator::Range && !BinaryOperator_is_assignment(expr->op())) {
            auto lhs_literal = std::dynamic_pointer_cast<BoundLiteral>(lhs);
            auto rhs_literal = std::dynamic_pointer_cast<BoundLiteral>(rhs);

            auto ret_maybe = lhs_literal->literal().evaluate(BinaryOperator_name(expr->op()), rhs_literal->literal());
            if (ret_maybe.has_value() && ret_maybe.value().type() != ObelixType::TypeException)
                return std::make_shared<BoundLiteral>(expr->token(), ret_maybe.value());
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
        if (lhs->node_type() == SyntaxNodeType::BoundLiteral && rhs->node_type() == SyntaxNodeType::BinaryExpression) {
            auto lhs_literal = std::dynamic_pointer_cast<BoundLiteral>(lhs);
            auto rhs_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(rhs);
            if (rhs_expr->lhs()->node_type() == SyntaxNodeType::BoundLiteral && BinaryOperator_precedence(expr->op()) == BinaryOperator_precedence(rhs_expr->op())) {
                auto rhs_literal = std::dynamic_pointer_cast<BoundLiteral>(rhs_expr->lhs());
                auto ret_maybe = lhs_literal->literal().evaluate(BinaryOperator_name(expr->op()), rhs_literal->literal());
                if (ret_maybe.has_value() && ret_maybe.value().type() != ObelixType::TypeException)
                    return std::make_shared<BoundBinaryExpression>(expr->token(),
                        std::make_shared<BoundLiteral>(expr->token(), ret_maybe.value()),
                        rhs_expr->op(),
                        rhs_expr->rhs(),
                        expr->type());
            }
        }

        if (rhs->node_type() == SyntaxNodeType::BoundLiteral && lhs->node_type() == SyntaxNodeType::BinaryExpression) {
            auto rhs_literal = std::dynamic_pointer_cast<BoundLiteral>(rhs);
            auto lhs_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(lhs);
            if (lhs_expr->rhs()->node_type() == SyntaxNodeType::BoundLiteral && BinaryOperator_precedence(expr->op()) == BinaryOperator_precedence(lhs_expr->op())) {
                auto lhs_literal = std::dynamic_pointer_cast<BoundLiteral>(lhs_expr->lhs());
                auto ret_maybe = lhs_literal->literal().evaluate(BinaryOperator_name(expr->op()), rhs_literal->literal());
                if (ret_maybe.has_value() && ret_maybe.value().type() != ObelixType::TypeException)
                    if (ret_maybe.has_value() && ret_maybe.value().type() != ObelixType::TypeException)
                        return std::make_shared<BoundBinaryExpression>(expr->token(),
                            lhs_expr->lhs(),
                            lhs_expr->op(),
                            std::make_shared<BoundLiteral>(expr->token(), ret_maybe.value()),
                            expr->type());
            }
        }
        return expr;
    }

    case SyntaxNodeType::BoundUnaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
        auto operand = TRY_AND_CAST(BoundExpression, fold_constants_processor(expr->operand(), ctx));

        if (expr->op() == UnaryOperator::Identity)
            return operand;

        if (operand->node_type() == SyntaxNodeType::BoundLiteral) {
            auto literal = std::dynamic_pointer_cast<BoundLiteral>(operand);

            auto ret_maybe = literal->literal().evaluate(UnaryOperator_name(expr->op()));
            if (ret_maybe.has_value() && ret_maybe.value().type() != ObelixType::TypeException)
                return std::make_shared<BoundLiteral>(expr->token(), ret_maybe.value());
        }
        return std::make_shared<BoundUnaryExpression>(expr->token(), operand, expr->op(), expr->type());
    }

    case SyntaxNodeType::BoundIdentifier: {
        auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(tree);
        if (auto constant_maybe = ctx.get(identifier->name()); constant_maybe.has_value()) {
            return constant_maybe.value();
        }
        return tree;
    }

    case SyntaxNodeType::BoundBranch: {
        auto elif = std::dynamic_pointer_cast<BoundBranch>(tree);
        auto cond = TRY_AND_CAST(BoundExpression, fold_constants_processor(elif->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, fold_constants_processor(elif->statement(), ctx));
        if (cond == nullptr)
            return stmt;

        if (cond->node_type() == SyntaxNodeType::BoundLiteral) {
            auto cond_literal = std::dynamic_pointer_cast<BoundLiteral>(cond);
            auto cond_obj = cond_literal->literal();
            if (cond_obj) {
                return stmt;
            } else {
                return nullptr;
            }
        }
        return std::make_shared<BoundBranch>(elif->token(), cond, stmt);
    }

    case SyntaxNodeType::BoundIfStatement: {
        auto stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        Statements branches;
        for (auto const& branch : stmt->branches()) {
            auto b = TRY_AND_CAST(Statement, fold_constants_processor(branch, ctx));
            if (b)
                branches.push_back(b);
        }

        bool constant_true_added { false };
        BoundBranches new_branches;
        for (auto const& branch : branches) {
            switch (branch->node_type()) {
            case SyntaxNodeType::BoundBranch: {
                new_branches.push_back(std::dynamic_pointer_cast<BoundBranch>(branch));
                break;
            }
            default:
                // This is a constant-true branch. If we're not building a new
                // statement, return this statement. Else add this if it's the
                // first constant true branch:
                if (new_branches.empty())
                    return branch;
                if (!constant_true_added) {
                    new_branches.push_back(std::make_shared<BoundBranch>(branch->token(), nullptr, branch));
                    constant_true_added = true;
                }
                break;
            }
        }

        // Nothing left. Everything was false:
        if (new_branches.empty())
            return std::make_shared<Pass>(stmt->token());

        return std::make_shared<BoundIfStatement>(stmt->token(), new_branches);
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
