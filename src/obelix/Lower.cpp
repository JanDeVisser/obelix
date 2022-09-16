/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <obelix/Syntax.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

using LowerContext = Context<int>;

INIT_NODE_PROCESSOR(LowerContext);

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    if (func_def->statement()) {
        auto statement = TRY_AND_CAST(Statement, process(func_def->statement(), ctx));
        switch (statement->node_type()) {
        case SyntaxNodeType::Block: {
            auto block = std::dynamic_pointer_cast<Block>(statement);
            return std::make_shared<BoundFunctionDef>(func_def, std::make_shared<FunctionBlock>(block->token(), block->statements()));
        }
        case SyntaxNodeType::FunctionBlock: {
            auto block = std::dynamic_pointer_cast<FunctionBlock>(statement);
            return std::make_shared<BoundFunctionDef>(func_def, block);
        }
        default:
            return std::make_shared<BoundFunctionDef>(func_def, std::make_shared<FunctionBlock>(statement->token(), statement));
        }
    }
    return tree;
}

NODE_PROCESSOR(BoundCaseStatement)
{
    auto case_stmt = std::dynamic_pointer_cast<BoundCaseStatement>(tree);
    auto condition = TRY_AND_CAST(BoundExpression, process(case_stmt->condition(), ctx));
    auto stmt = TRY_AND_CAST(Statement, process(case_stmt->statement(), ctx));
    return std::make_shared<BoundCaseStatement>(case_stmt->token(), condition, stmt);
}

NODE_PROCESSOR(BoundSwitchStatement)
{
    auto switch_stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
    auto switch_expr = TRY_AND_CAST(BoundExpression, process(switch_stmt->expression(), ctx));
    auto default_case = TRY_AND_CAST(BoundCaseStatement, process(switch_stmt->default_case(), ctx));

    BoundCaseStatements cases;
    for (auto& c : switch_stmt->cases()) {
        auto new_case = TRY_AND_CAST(BoundCaseStatement, process(c, ctx));
        cases.push_back(new_case);
    }

    BoundBranches branches;
    for (auto& c : cases) {
        branches.push_back(std::make_shared<BoundBranch>(c->token(),
            std::make_shared<BoundBinaryExpression>(switch_expr->token(), switch_expr, BinaryOperator::Equals, c->condition(), ObjectType::get(PrimitiveType::Boolean)),
            c->statement()));
    }
    if (default_case) {
        auto default_stmt = TRY_AND_CAST(Statement, process(default_case->statement(), ctx));
        branches.push_back(std::make_shared<BoundBranch>(default_case->token(), nullptr, default_stmt));
    }
    return process(std::make_shared<BoundIfStatement>(switch_stmt->token(), branches), ctx);
}

NODE_PROCESSOR(BoundWhileStatement)
{
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
    auto condition = TRY_AND_CAST(BoundExpression, process(while_stmt->condition(), ctx));
    auto stmt = TRY_AND_CAST(Statement, process(while_stmt->statement(), ctx));

    Statements while_block;
    auto start_of_loop = std::make_shared<Label>(while_stmt->token());
    auto jump_out_of_loop = std::make_shared<Goto>(while_stmt->token());
    while_block.push_back(start_of_loop);

    BoundBranches branches {
        std::make_shared<BoundBranch>(while_stmt->token(),
            std::make_shared<BoundUnaryExpression>(condition->token(),
                condition, UnaryOperator::LogicalInvert, ObjectType::get(PrimitiveType::Boolean)),
            jump_out_of_loop),
    };
    while_block.push_back(std::make_shared<BoundIfStatement>(condition->token(), branches));
    while_block.push_back(stmt);
    while_block.push_back(std::make_shared<Goto>(while_stmt->token(), start_of_loop));
    while_block.push_back(std::make_shared<Label>(jump_out_of_loop));
    return process(std::make_shared<Block>(while_stmt->token(), while_block), ctx);
}

NODE_PROCESSOR(BoundForStatement)
{
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
    auto range_expr = TRY_AND_CAST(BoundExpression, process(for_stmt->range(), ctx));
    auto stmt = TRY_AND_CAST(Statement, process(for_stmt->statement(), ctx));

    if (range_expr->node_type() != SyntaxNodeType::BoundBinaryExpression)
        return SyntaxError { ErrorCode::SyntaxError, for_stmt->token(), "Invalid for-loop range" };
    auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(range_expr);
    if (range_binary_expr->op() != BinaryOperator::Range)
        return SyntaxError { ErrorCode::SyntaxError, range_expr->token(), "Invalid for-loop range" };
    auto variable_type = for_stmt->variable()->type();

    Statements for_block;

    if (for_stmt->must_declare_variable()) {
        for_block.push_back(
            std::make_shared<BoundVariableDeclaration>(for_stmt->token(), for_stmt->variable(), false, range_binary_expr->lhs()));
    } else {
        for_block.push_back(
            std::make_shared<BoundExpressionStatement>(for_stmt->token(),
                std::make_shared<BoundAssignment>(for_stmt->token(),
                    for_stmt->variable(),
                    range_binary_expr->lhs())));
    }
    auto jump_past_loop = std::make_shared<Goto>();
    auto jump_back_label = std::make_shared<Label>();

    for_block.push_back(jump_back_label);

    auto rhs = range_binary_expr->rhs();
    auto rhs_int = std::dynamic_pointer_cast<BoundIntLiteral>(rhs);
    if ((rhs_int != nullptr) && (*(rhs_int->type()) != *variable_type)) {
        rhs = TRY(rhs_int->cast(variable_type));
    }

    for_block.push_back(std::make_shared<BoundIfStatement>(for_stmt->token(),
        BoundBranches {
            std::make_shared<BoundBranch>(for_stmt->token(),
                std::make_shared<BoundBinaryExpression>(for_stmt->token(),
                    for_stmt->variable(), BinaryOperator::GreaterEquals, rhs, ObjectType::get(PrimitiveType::Boolean)),
                jump_past_loop),
        }));
    for_block.push_back(stmt);
    for_block.push_back(
        std::make_shared<BoundExpressionStatement>(range_binary_expr->token(),
            std::make_shared<BoundUnaryExpression>(range_binary_expr->token(),
                for_stmt->variable(),
                UnaryOperator::UnaryIncrement,
                variable_type)));
    for_block.push_back(std::make_shared<Goto>(for_stmt->token(), jump_back_label));
    for_block.push_back(std::make_shared<Label>(jump_past_loop));
    return process(std::make_shared<Block>(stmt->token(), for_block), ctx);
}

NODE_PROCESSOR(BoundBinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, process(expr->lhs(), ctx));
    auto rhs = TRY_AND_CAST(BoundExpression, process(expr->rhs(), ctx));

    if (expr->op() == BinaryOperator::GreaterEquals) {
        return std::make_shared<BoundBinaryExpression>(expr->token(),
            std::make_shared<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            BinaryOperator::LogicalOr,
            std::make_shared<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Greater, rhs, ObjectType::get(PrimitiveType::Boolean)),
            ObjectType::get(PrimitiveType::Boolean));
    }

    if (expr->op() == BinaryOperator::LessEquals) {
        return std::make_shared<BoundBinaryExpression>(expr->token(),
            std::make_shared<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            BinaryOperator::LogicalOr,
            std::make_shared<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Less, rhs, ObjectType::get(PrimitiveType::Boolean)),
            ObjectType::get(PrimitiveType::Boolean));
    }

    if (expr->op() == BinaryOperator::NotEquals) {
        return std::make_shared<BoundUnaryExpression>(expr->token(),
            std::make_shared<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            UnaryOperator::LogicalInvert,
            ObjectType::get(PrimitiveType::Boolean));
    }

    return expr;
}

NODE_PROCESSOR(BoundUnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, process(expr->operand(), ctx));
    if (expr->op() == UnaryOperator::UnaryIncrement || expr->op() == UnaryOperator::UnaryDecrement) {
        auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(operand);
        auto new_rhs = std::make_shared<BoundBinaryExpression>(expr->token(),
            identifier,
            (expr->op() == UnaryOperator::UnaryIncrement) ? BinaryOperator::Add : BinaryOperator::Subtract,
            std::make_shared<BoundIntLiteral>(expr->token(), 1l, identifier->type()),
            identifier->type());
        return std::make_shared<BoundAssignment>(expr->token(), identifier, new_rhs);
    }
    return tree;
}

ErrorOrNode lower(std::shared_ptr<SyntaxNode> const& tree)
{
    auto lowered = TRY(process<LowerContext>(tree));
    return resolve_operators(lowered);
}

}
