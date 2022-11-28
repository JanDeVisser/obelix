/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Processor.h>
#include <obelix/SyntaxNodeType.h>
#include <obelix/interp/Interp.h>

namespace Obelix {

extern_logging_category(parser);

class FoldContextPayload {
public:
    FoldContextPayload() = default;

    void push_switch_expression(std::shared_ptr<BoundExpression> const& expr)
    {
        m_switch_expressions.push_back(expr);
    }

    void pop_switch_expression()
    {
        if (!m_switch_expressions.empty())
            m_switch_expressions.pop_back();
    }

    [[nodiscard]] std::shared_ptr<BoundExpression> last_switch_expression() const
    {
        if (m_switch_expressions.empty())
            return nullptr;
        return m_switch_expressions.back();
    }
private:
    std::vector<std::shared_ptr<BoundExpression>> m_switch_expressions;
};

using FoldContext = Context<pBoundLiteral, FoldContextPayload>;

INIT_NODE_PROCESSOR(FoldContext)

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
    auto expr = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
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

ALIAS_NODE_PROCESSOR(BoundStaticVariableDeclaration, BoundVariableDeclaration)

// FIXME: Global and local module-scoped variables are not const-folded, because
// they may be referenced by other modules processed later. In that case the declaration
// will be gone, resulting in a crash at compile time

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
    BoundExpressions processed_args;
    for (auto const& arg : call->arguments()) {
        auto processed = TRY_AND_CAST(BoundExpression, arg, ctx);
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
    auto lhs = TRY_AND_CAST(BoundExpression, expr->lhs(), ctx);
    auto rhs = TRY_AND_CAST(BoundExpression, expr->rhs(), ctx);

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
    auto branch = std::dynamic_pointer_cast<BoundBranch>(tree);
    auto cond = branch->condition();
    if (auto switch_expr = ctx.data().last_switch_expression(); switch_expr != nullptr && cond != nullptr) {
        cond = std::make_shared<BoundBinaryExpression>(branch->token(), switch_expr, BinaryOperator::Equals, branch->condition(), ObjectType::get(PrimitiveType::Boolean));
    }
    auto stmt = TRY_AND_CAST(Statement, branch->statement(), ctx);
    if (cond == nullptr)
        return std::make_shared<BoundBranch>(branch->token(), nullptr, stmt);
    cond = TRY_AND_CAST(BoundExpression, cond, ctx);

    auto cond_literal = std::dynamic_pointer_cast<BoundBooleanLiteral>(cond);
    if (cond_literal != nullptr) {
        auto cond_value = cond_literal->value();
        if (cond_value) {
            return stmt;
        } else {
            return nullptr;
        }
    }
    cond = TRY_AND_CAST(BoundExpression, branch->condition(), ctx);
    return std::make_shared<BoundBranch>(branch->token(), cond, stmt);
}

ErrorOr<BoundBranches, SyntaxError> new_branches(BoundBranches current_branches, FoldContext& ctx, ProcessResult &result)
{
    Statements branches;
    for (auto const& branch : current_branches) {
        auto b = TRY_AND_CAST(Statement, branch, ctx);
        if (b != nullptr) // b is nullptr if the condition is constant false
            branches.push_back(b);
    }

    bool constant_true_added { false };
    BoundBranches new_branches;
    for (auto const& branch : branches) {
        switch (branch->node_type()) {
        case SyntaxNodeType::BoundBranch:
            new_branches.push_back(std::dynamic_pointer_cast<BoundBranch>(branch));
            break;
        default: {
            // This is a constant-true branch. If we're not building a new
            // statement, return this statement. Else add this if it's the
            // first constant true branch:
            auto else_branch = std::make_shared<BoundBranch>(branch->token(), nullptr, branch);
            if (new_branches.empty())
                return BoundBranches { else_branch };
            new_branches.push_back(else_branch);
            constant_true_added = true;
            break;
        }
        }
        if (constant_true_added)
            break;
    }
    return new_branches;
}

NODE_PROCESSOR(BoundIfStatement)
{
    auto stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);
    ctx.data().push_switch_expression(nullptr);
    auto branches = TRY(new_branches(stmt->branches(), ctx, result));
    ctx.data().pop_switch_expression();

    if (branches.empty())
        // Nothing left. Everything was false and there was no 'else' branch:
        return std::make_shared<Pass>(stmt->token());

    if ((branches.size() == 1) && (branches[0]->condition() == nullptr))
        // First branch is always true, or only 'else' branch left:
        return branches[0]->statement();

    return std::make_shared<BoundIfStatement>(stmt->token(), branches);
}

NODE_PROCESSOR(BoundSwitchStatement)
{
    auto stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
    auto expr = TRY_AND_CAST(BoundExpression, stmt->expression(), ctx);
    ctx().push_switch_expression(expr);
    auto branches = TRY(new_branches(stmt->cases(), ctx, result));
    ctx().pop_switch_expression();
    auto default_branch = TRY_AND_CAST(BoundBranch, stmt->default_case(), ctx);

    // Nothing left. Everything was false:
    if (branches.empty()) {
        if (default_branch != nullptr) {
            return default_branch->statement();
        }
        return std::make_shared<Pass>(stmt->token());
    }

    if ((branches.size() == 1) && (branches[0]->condition() == nullptr))
        return branches[0]->statement();
    if (branches[branches.size()-1]->condition() == nullptr)
        default_branch = nullptr;
    return std::make_shared<BoundSwitchStatement>(stmt->token(), expr, branches, default_branch);
}

ProcessResult fold_constants(std::shared_ptr<SyntaxNode> const& tree)
{
    Config config;
    FoldContext ctx(config);
    return process<FoldContext>(tree, ctx);
}

}
