/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <obelix/SyntaxNodeType.h>
#include <obelix/interp/Interp.h>

namespace Obelix {

extern_logging_category(parser);

using FoldContext = Context<std::shared_ptr<BoundLiteral>>;

INIT_NODE_PROCESSOR(FoldContext);

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
    auto expr = TRY_AND_CAST(BoundExpression, process(var_decl->expression(), ctx));
    auto literal = std::dynamic_pointer_cast<BoundLiteral>(expr);
    if (var_decl->is_const() && (literal != nullptr)) {
        ctx.declare(var_decl->name(), literal);
        return make_node<Pass>(var_decl);
    }
    switch (var_decl->node_type()) {
    case SyntaxNodeType::BoundVariableDeclaration:
        return make_node<BoundVariableDeclaration>(var_decl->token(), var_decl->variable(), var_decl->is_const(), expr);
    case SyntaxNodeType::BoundStaticVariableDeclaration:
        return make_node<BoundStaticVariableDeclaration>(var_decl->token(), var_decl->variable(), var_decl->is_const(), expr);
    default:
        fatal("Unreachable: node type = {}", var_decl->node_type());
    }
}

ALIAS_NODE_PROCESSOR(BoundStaticVariableDeclaration, BoundVariableDeclaration);

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
    BoundExpressions processed_args;
    for (auto const& arg : call->arguments()) {
        auto processed = TRY_AND_CAST(BoundExpression, process(arg, ctx));
        processed_args.push_back(processed);
    }
    auto processed_call = make_node<BoundIntrinsicCall>(call, processed_args, std::dynamic_pointer_cast<BoundIntrinsicDecl>(call->declaration()));
    return TRY(interpret(processed_call));
}

#if 0

// The rejigging of BinaryExpressions needs to be refitted into ResolveOperators.

NODE_PROCESSOR(BoundBinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, process(expr->lhs(), ctx));
    auto rhs = TRY_AND_CAST(BoundExpression, process(expr->rhs(), ctx));

    if (lhs->node_type() == SyntaxNodeType::BoundIntLiteral && rhs->node_type() == SyntaxNodeType::BoundIntLiteral && expr->op() != BinaryOperator::Range && !BinaryOperator_is_assignment(expr->op())) {
        return TRY(interpret(expr));
    }

    //
    // lhs is a constant and rhs is another binary (with the same precedence), and the
    // rhs of that binary is constant, we can fold as follows:
    //
    //     +                     +
    //    / \                 /     \.
    //   c1   +      =====> c1+c2    x
    //       / \.
    //     c2   x
    //
    // And similarly when rhs is constant, lhs is binary, and rhs of lhs is constant.
    //
    // FIXME: This implies that equals precedence implies that the operations are associative.
    // This is not necessarily true.
    //
    auto lhs_literal = std::dynamic_pointer_cast<BoundLiteral>(lhs);
    if ((lhs_literal != nullptr) && rhs->node_type() == SyntaxNodeType::BinaryExpression) {
        auto rhs_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(rhs);
        if ((std::dynamic_pointer_cast<BoundLiteral>(rhs_expr->lhs()) != nullptr) && BinaryOperator_precedence(expr->op()) == BinaryOperator_precedence(rhs_expr->op())) {
            std::shared_ptr<BoundExpression> new_lhs = make_node<BoundBinaryExpression>(lhs->token(), lhs, expr->op(), rhs_expr->lhs(), expr->type());
            new_lhs = TRY_AND_CAST(BoundExpression, interpret(new_lhs));
            if (std::dynamic_pointer_cast<BoundLiteral>(new_lhs) != nullptr) {
                return std::make_shared<BoundBinaryExpression>(expr->token(),
                    new_lhs,
                    rhs_expr->op(),
                    rhs_expr->rhs(),
                    expr->type());
            }
        }
    }

    auto rhs_literal = std::dynamic_pointer_cast<BoundLiteral>(rhs);
    if ((rhs_literal != nullptr) && lhs->node_type() == SyntaxNodeType::BinaryExpression) {
        auto lhs_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(lhs);
        if ((std::dynamic_pointer_cast<BoundLiteral>(lhs_expr->rhs()) != nullptr) && BinaryOperator_precedence(expr->op()) == BinaryOperator_precedence(lhs_expr->op())) {
            std::shared_ptr<BoundExpression> new_rhs = make_node<BoundBinaryExpression>(rhs->token(), lhs_expr->rhs(), expr->op(), rhs, expr->type());
            new_rhs = TRY_AND_CAST(BoundExpression, interpret(new_rhs));
            if (std::dynamic_pointer_cast<BoundLiteral>(new_rhs) != nullptr) {
                return std::make_shared<BoundBinaryExpression>(expr->token(),
                    lhs_expr->lhs(),
                    lhs_expr->op(),
                    new_rhs,
                    expr->type());
            }
        }
    }
    return expr;
}

#endif

NODE_PROCESSOR(BoundVariable)
{
    auto variable = std::dynamic_pointer_cast<BoundVariable>(tree);
    if (auto constant_maybe = ctx.get(variable->name()); constant_maybe.has_value()) {
        return constant_maybe.value();
    }
    return tree;
}

NODE_PROCESSOR(BoundBranch)
{
    auto elif = std::dynamic_pointer_cast<BoundBranch>(tree);
    auto cond = TRY_AND_CAST(BoundExpression, process(elif->condition(), ctx));
    auto stmt = TRY_AND_CAST(Statement, process(elif->statement(), ctx));
    if (cond == nullptr)
        return stmt;

    auto cond_literal = std::dynamic_pointer_cast<BoundBooleanLiteral>(cond);
    if (cond_literal != nullptr) {
        auto cond_value = cond_literal->value();
        if (cond_value) {
            return stmt;
        } else {
            return nullptr;
        }
    }
    return std::make_shared<BoundBranch>(elif->token(), cond, stmt);
}

NODE_PROCESSOR(BoundIfStatement)
{
    auto stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);
    Statements branches;
    for (auto const& branch : stmt->branches()) {
        auto b = TRY_AND_CAST(Statement, process(branch, ctx));
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

ErrorOrNode fold_constants(std::shared_ptr<SyntaxNode> const& tree)
{
    return process<FoldContext>(tree);
}

}
