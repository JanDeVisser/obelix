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
        auto statement = TRY_AND_CAST(Statement, processor.process(func_def->statement(), ctx));
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
});

NODE_PROCESSOR(BoundCaseStatement)
{
    auto case_stmt = std::dynamic_pointer_cast<BoundCaseStatement>(tree);
    auto condition = TRY_AND_CAST(BoundExpression, processor.process(case_stmt->condition(), ctx));
    auto stmt = TRY_AND_CAST(Statement, processor.process(case_stmt->statement(), ctx));
    return make_node<BoundCaseStatement>(case_stmt->token(), condition, stmt);
});

NODE_PROCESSOR(BoundSwitchStatement)
{
    auto switch_stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
    auto switch_expr = TRY_AND_CAST(BoundExpression, processor.process(switch_stmt->expression(), ctx));
    auto default_case = TRY_AND_CAST(BoundCaseStatement, processor.process(switch_stmt->default_case(), ctx));

    BoundCaseStatements cases;
    for (auto& c : switch_stmt->cases()) {
        auto new_case = TRY_AND_CAST(BoundCaseStatement, processor.process(c, ctx));
        cases.push_back(new_case);
    }

    BoundBranches branches;
    for (auto& c : cases) {
        branches.push_back(make_node<BoundBranch>(c->token(),
            make_node<BoundBinaryExpression>(switch_expr->token(), switch_expr, BinaryOperator::Equals, c->condition(), ObjectType::get(PrimitiveType::Boolean)),
            c->statement()));
    }
    if (default_case) {
        auto default_stmt = TRY_AND_CAST(Statement, processor.process(default_case->statement(), ctx));
        branches.push_back(make_node<BoundBranch>(default_case->token(), nullptr, default_stmt));
    }
    return processor.process(make_node<BoundIfStatement>(switch_stmt->token(), branches), ctx);
});

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
    auto condition = TRY_AND_CAST(BoundExpression, processor.process(while_stmt->condition(), ctx));
    auto stmt = TRY_AND_CAST(Statement, processor.process(while_stmt->statement(), ctx));

    Statements while_block;
    auto start_of_loop = make_node<Label>(while_stmt->token());
    auto jump_out_of_loop = make_node<Goto>(while_stmt->token());
    while_block.push_back(start_of_loop);

    BoundBranches branches {
        make_node<BoundBranch>(while_stmt->token(),
            make_node<BoundUnaryExpression>(condition->token(),
                condition, UnaryOperator::LogicalInvert, ObjectType::get(PrimitiveType::Boolean)),
            jump_out_of_loop),
    };
    while_block.push_back(make_node<BoundIfStatement>(condition->token(), branches));
    while_block.push_back(stmt);
    while_block.push_back(make_node<Goto>(while_stmt->token(), start_of_loop));
    while_block.push_back(make_node<Label>(jump_out_of_loop));
    return processor.process(make_node<Block>(while_stmt->token(), while_block), ctx);
});

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
    auto range_expr = TRY_AND_CAST(BoundExpression, processor.process(for_stmt->range(), ctx));
    auto stmt = TRY_AND_CAST(Statement, processor.process(for_stmt->statement(), ctx));

    if (range_expr->node_type() != SyntaxNodeType::BoundBinaryExpression)
        return SyntaxError { ErrorCode::SyntaxError, for_stmt->token(), "Invalid for-loop range" };
    auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(range_expr);
    if (range_binary_expr->op() != BinaryOperator::Range)
        return SyntaxError { ErrorCode::SyntaxError, range_expr->token(), "Invalid for-loop range" };
    auto variable_type = range_binary_expr->lhs()->type();

    Statements for_block;

    if (for_stmt->must_declare_variable()) {
        for_block.push_back(
            make_node<BoundVariableDeclaration>(for_stmt->token(),
                make_node<BoundIdentifier>(for_stmt->token(), for_stmt->variable(), variable_type),
                false,
                range_binary_expr->lhs()));
    } else {
        for_block.push_back(
            make_node<BoundExpressionStatement>(for_stmt->token(),
                make_node<BoundAssignment>(for_stmt->token(),
                    make_node<BoundIdentifier>(for_stmt->token(), for_stmt->variable(), variable_type),
                    range_binary_expr->lhs())));
    }
    auto jump_past_loop = make_node<Goto>();
    auto jump_back_label = make_node<Label>();

    for_block.push_back(jump_back_label);
    BoundBranches branches {
        make_node<BoundBranch>(for_stmt->token(),
            make_node<BoundBinaryExpression>(for_stmt->token(),
                make_node<BoundIdentifier>(for_stmt->token(), for_stmt->variable(), ObjectType::get(PrimitiveType::Int)),
                BinaryOperator::GreaterEquals,
                range_binary_expr->rhs(),
                ObjectType::get(PrimitiveType::Boolean)),
            jump_past_loop),
    };
    for_block.push_back(make_node<BoundIfStatement>(for_stmt->token(), branches));
    for_block.push_back(stmt);
    for_block.push_back(
        make_node<BoundExpressionStatement>(range_binary_expr->token(),
            make_node<BoundUnaryExpression>(range_binary_expr->token(),
                make_node<BoundIdentifier>(range_binary_expr->token(),
                    for_stmt->variable(), variable_type),
                UnaryOperator::UnaryIncrement,
                variable_type)));
    for_block.push_back(make_node<Goto>(for_stmt->token(), jump_back_label));
    for_block.push_back(make_node<Label>(jump_past_loop));
    return processor.process(make_node<Block>(stmt->token(), for_block), ctx);
});

NODE_PROCESSOR(BoundBinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, processor.process(expr->lhs(), ctx));
    auto rhs = TRY_AND_CAST(BoundExpression, processor.process(expr->rhs(), ctx));

    if (expr->op() == BinaryOperator::GreaterEquals) {
        return make_node<BoundBinaryExpression>(expr->token(),
            make_node<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            BinaryOperator::LogicalOr,
            make_node<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Greater, rhs, ObjectType::get(PrimitiveType::Boolean)),
            ObjectType::get(PrimitiveType::Boolean));
    }

    if (expr->op() == BinaryOperator::LessEquals) {
        return make_node<BoundBinaryExpression>(expr->token(),
            make_node<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            BinaryOperator::LogicalOr,
            make_node<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Less, rhs, ObjectType::get(PrimitiveType::Boolean)),
            ObjectType::get(PrimitiveType::Boolean));
    }

    if (expr->op() == BinaryOperator::NotEquals) {
        return make_node<BoundUnaryExpression>(expr->token(),
            make_node<BoundBinaryExpression>(expr->token(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            UnaryOperator::LogicalInvert,
            ObjectType::get(PrimitiveType::Boolean));
    }

    return expr;
});

NODE_PROCESSOR(BoundUnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, processor.process(expr->operand(), ctx));
    if (expr->op() == UnaryOperator::UnaryIncrement || expr->op() == UnaryOperator::UnaryDecrement) {
        auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(operand);
        auto new_rhs = make_node<BoundBinaryExpression>(expr->token(),
            identifier,
            (expr->op() == UnaryOperator::UnaryIncrement) ? BinaryOperator::Add : BinaryOperator::Subtract,
            make_node<BoundLiteral>(expr->token(), 1),
            identifier->type());
        debug(parser, "identifier->type() = {} new_rhs->type() = {}", identifier->type(), new_rhs->type());
        return make_node<BoundAssignment>(expr->token(), identifier, new_rhs);
    }
    return tree;
});

ErrorOrNode lower(std::shared_ptr<SyntaxNode> const& tree)
{
    return processor_for_context<LowerContext>(tree);
}

}
