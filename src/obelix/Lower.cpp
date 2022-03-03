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

using LowerContext = Context<int>;

ErrorOrNode lower_processor(std::shared_ptr<SyntaxNode> const& tree, LowerContext ctx)
{
    if (!tree)
        return tree;

    switch (tree->node_type()) {

    case SyntaxNodeType::BoundFunctionDef: {
        auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
        if (func_def->statement()) {
            auto statement = TRY_AND_CAST(Statement, lower_processor(func_def->statement(), ctx));
            switch (statement->node_type()) {
            case SyntaxNodeType::Block: {
                auto block = std::dynamic_pointer_cast<Block>(statement);
                return make_node<BoundFunctionDef>(func_def, make_node<FunctionBlock>(block->token(), block->statements()));
            }
            case SyntaxNodeType::FunctionBlock: {
                auto block = std::dynamic_pointer_cast<FunctionBlock>(statement);
                return make_node<BoundFunctionDef>(func_def, block);
            }
            default:
                return make_node<BoundFunctionDef>(func_def, make_node<FunctionBlock>(statement->token(), statement));
            }
        }
        return tree;
    }

    case SyntaxNodeType::BoundCaseStatement: {
        auto case_stmt = std::dynamic_pointer_cast<BoundCaseStatement>(tree);
        auto condition = TRY_AND_CAST(BoundExpression, lower_processor(case_stmt->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, lower_processor(case_stmt->statement(), ctx));
        return make_node<BoundCaseStatement>(case_stmt->token(), condition, stmt);
    }

    case SyntaxNodeType::BoundSwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
        auto switch_expr = TRY_AND_CAST(BoundExpression, lower_processor(switch_stmt->expression(), ctx));
        auto default_case = TRY_AND_CAST(BoundCaseStatement, lower_processor(switch_stmt->default_case(), ctx));

        BoundCaseStatements cases;
        for (auto& c : switch_stmt->cases()) {
            auto new_case = TRY_AND_CAST(BoundCaseStatement, lower_processor(c, ctx));
            cases.push_back(new_case);
        }

        BoundBranches branches;
        for (auto& c : cases) {
            branches.push_back(std::make_shared<BoundBranch>(c->token(),
                std::make_shared<BoundBinaryExpression>(switch_expr->token(), switch_expr, BinaryOperator::Equals, c->condition(), ObjectType::get(ObelixType::TypeBoolean)),
                c->statement()));
        }
        if (default_case) {
            auto default_stmt = TRY_AND_CAST(Statement, lower_processor(default_case->statement(), ctx));
            branches.push_back(std::make_shared<BoundBranch>(default_case->token(), nullptr, default_stmt));
        }
        return make_node<BoundIfStatement>(switch_stmt->token(), branches);
    }

    case SyntaxNodeType::BoundForStatement: {
        //
        // for (x in 1..5) {
        //   foo(x);
        // }
        // ==>
        // {
        //    var x: int = 1;
        // label_1:
        //    if (x >= 5) goto label_0;
        //    foo(x);
        //    x = x + 1;
        //    goto label_1;
        // label_0:
        // }
        //
        auto for_stmt = std::dynamic_pointer_cast<BoundForStatement>(tree);
        auto range_expr = TRY_AND_CAST(BoundExpression, lower_processor(for_stmt->range(), ctx));
        auto stmt = TRY_AND_CAST(Statement, lower_processor(for_stmt->statement(), ctx));

        if (range_expr->node_type() != SyntaxNodeType::BoundBinaryExpression)
            return Error { ErrorCode::SyntaxError, "Invalid for-loop range" };
        auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(range_expr);
        if (range_binary_expr->op() != BinaryOperator::Range)
            return Error { ErrorCode::SyntaxError, "Invalid for-loop range" };

        Statements for_block;

        for_block.push_back(
            make_node<BoundVariableDeclaration>(for_stmt->token(),
                make_node<BoundIdentifier>(for_stmt->token(), for_stmt->variable(), ObjectType::get(ObelixType::TypeInt)),
                false,
                range_binary_expr->lhs()));
        auto jump_past_loop = std::make_shared<Goto>();
        auto jump_back_label = std::make_shared<Label>();

        BoundBranches branches {
            make_node<BoundBranch>(for_stmt->token(),
                make_node<BoundBinaryExpression>(for_stmt->token(),
                    make_node<BoundIdentifier>(for_stmt->token(), for_stmt->variable(), ObjectType::get(ObelixType::TypeInt)),
                    BinaryOperator::GreaterEquals,
                    range_binary_expr->rhs(),
                    ObjectType::get(ObelixType::TypeBoolean)),
                jump_past_loop),
        };
        for_block.push_back(make_node<BoundIfStatement>(for_stmt->token(), branches));
        for_block.push_back(stmt);
        for_block.push_back(
            make_node<BoundExpressionStatement>(range_binary_expr->token(),
                make_node<BoundBinaryExpression>(range_binary_expr->token(),
                    make_node<BoundIdentifier>(range_binary_expr->token(),
                        for_stmt->variable(), ObjectType::get(ObelixType::TypeInt)),
                    BinaryOperator::Assign,
                    make_node<BoundBinaryExpression>(range_binary_expr->token(),
                        make_node<BoundIdentifier>(range_binary_expr->token(),
                            for_stmt->variable(), ObjectType::get(ObelixType::TypeInt)),
                        BinaryOperator::Add,
                        make_node<BoundLiteral>(range_binary_expr->token(), make_obj<Integer>(1)),
                        ObjectType::get(ObelixType::TypeInt)),
                    ObjectType::get(ObelixType::TypeInt))));
        for_block.push_back(make_node<Goto>(for_stmt->token(), jump_back_label));
        for_block.push_back(make_node<Label>(jump_past_loop));
        return make_node<Block>(stmt->token(), for_block);
    }

    case SyntaxNodeType::WhileStatement: {
        //
        // while (x < 10) {
        //   foo(x);
        //   x += 1;
        // }
        // ==>
        // {
        // label_0:
        //   if (!(x<10)) goto label_1;
        //   foo(x);
        //   x += 1;
        //   goto label_0;
        // label_1:
        // }
        //
        auto while_stmt = std::dynamic_pointer_cast<BoundWhileStatement>(tree);
        auto condition = TRY_AND_CAST(BoundExpression, lower_processor(while_stmt->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, lower_processor(while_stmt->statement(), ctx));

        Statements while_block;
        auto start_of_loop = make_node<Label>(while_stmt->token());
        auto jump_out_of_loop = make_node<Goto>(while_stmt->token());
        while_block.push_back(start_of_loop);

        BoundBranches branches {
            make_node<BoundBranch>(while_stmt->token(),
                make_node<BoundUnaryExpression>(condition->token(),
                    condition, UnaryOperator::LogicalInvert, ObjectType::get(ObelixType::TypeBoolean)),
                jump_out_of_loop),
        };
        while_block.push_back(make_node<BoundIfStatement>(condition->token(), branches));
        while_block.push_back(stmt);
        while_block.push_back(make_node<Goto>(while_stmt->token(), start_of_loop));
        while_block.push_back(make_node<Label>(jump_out_of_loop));
        return make_node<Block>(while_stmt->token(), while_block);
    }

    case SyntaxNodeType::BoundBinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
        auto lhs = TRY_AND_CAST(BoundExpression, lower_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(BoundExpression, lower_processor(expr->rhs(), ctx));

        if (expr->op() == BinaryOperator::Assign && lhs->node_type() == SyntaxNodeType::BoundIdentifier) {
            auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(lhs);
            return std::make_shared<BoundAssignment>(expr->token(), identifier, rhs);
        }

        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        if (BinaryOperator_is_assignment(expr->op()) && lhs->node_type() == SyntaxNodeType::BoundIdentifier) {
            auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(lhs);
            auto new_rhs = std::make_shared<BoundBinaryExpression>(expr->token(),
                identifier, BinaryOperator_for_assignment_operator(expr->op()), rhs, expr->type());
            return std::make_shared<BoundAssignment>(expr->token(), identifier, new_rhs);
        }

        if (expr->op() == BinaryOperator::GreaterEquals) {
            return std::make_shared<BoundBinaryExpression>(expr->token(),
                std::make_shared<BoundBinaryExpression>(expr->token(),
                    lhs, BinaryOperator::Equals, rhs, ObjectType::get(ObelixType::TypeBoolean)),
                BinaryOperator::LogicalOr,
                std::make_shared<BoundBinaryExpression>(expr->token(),
                    lhs, BinaryOperator::Greater, rhs, ObjectType::get(ObelixType::TypeBoolean)),
                ObjectType::get(ObelixType::TypeBoolean));
        }

        if (expr->op() == BinaryOperator::LessEquals) {
            return std::make_shared<BoundBinaryExpression>(expr->token(),
                std::make_shared<BoundBinaryExpression>(expr->token(),
                    lhs, BinaryOperator::Equals, rhs, ObjectType::get(ObelixType::TypeBoolean)),
                BinaryOperator::LogicalOr,
                std::make_shared<BoundBinaryExpression>(expr->token(),
                    lhs, BinaryOperator::Less, rhs, ObjectType::get(ObelixType::TypeBoolean)),
                ObjectType::get(ObelixType::TypeBoolean));
        }

        if (expr->op() == BinaryOperator::NotEquals) {
            return std::make_shared<BoundUnaryExpression>(expr->token(),
                std::make_shared<BoundBinaryExpression>(expr->token(),
                    lhs, BinaryOperator::Equals, rhs, ObjectType::get(ObelixType::TypeBoolean)),
                UnaryOperator::LogicalInvert,
                ObjectType::get(ObelixType::TypeBoolean));
        }

        return expr;
    }

    default:
        return process_tree(tree, ctx, lower_processor);
    }
}

ErrorOrNode lower(std::shared_ptr<SyntaxNode> const& tree)
{
    LowerContext ctx;
    return lower_processor(tree, ctx);
}

}
