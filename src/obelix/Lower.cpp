/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/Syntax.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

class LowerContextPayload {
};

using LowerContext = Context<bool, LowerContextPayload>;

INIT_NODE_PROCESSOR(LowerContext)

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    if (func_def->statement()) {
        auto statement = TRY_AND_CAST(Statement, func_def->statement(), ctx);
        switch (statement->node_type()) {
        case SyntaxNodeType::Block: {
            auto block = std::dynamic_pointer_cast<Block>(statement);
            return std::make_shared<BoundFunctionDef>(func_def, std::make_shared<FunctionBlock>(block->location(), block->statements(), func_def->declaration()));
        }
        case SyntaxNodeType::FunctionBlock: {
            auto block = std::dynamic_pointer_cast<FunctionBlock>(statement);
            return std::make_shared<BoundFunctionDef>(func_def, block);
        }
        default:
            return std::make_shared<BoundFunctionDef>(func_def, std::make_shared<FunctionBlock>(statement->location(), statement, func_def->declaration()));
        }
    }
    return tree;
}

NODE_PROCESSOR(BoundSwitchStatement)
{
    auto switch_stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
    auto switch_expr = TRY_AND_CAST(BoundExpression, switch_stmt->expression(), ctx);

    if (ctx.config().target == Architecture::C_TRANSPILER) {
        switch (switch_expr->type()->type()) {
        case PrimitiveType::IntegerNumber:
        case PrimitiveType::SignedIntegerNumber:
        case PrimitiveType::Enum:
            return tree;
        default:
            break;
        }
    }

    auto default_case = TRY_AND_CAST(BoundBranch, switch_stmt->default_case(), ctx);

    BoundBranches cases;
    for (auto& c : switch_stmt->cases()) {
        auto new_case = TRY_AND_CAST(BoundBranch, c, ctx);
        cases.push_back(new_case);
    }

    BoundBranches branches;
    for (auto& c : cases) {
        branches.push_back(std::make_shared<BoundBranch>(c->location(),
            std::make_shared<BoundBinaryExpression>(switch_expr->location(), switch_expr, BinaryOperator::Equals, c->condition(), ObjectType::get(PrimitiveType::Boolean)),
            c->statement()));
    }
    if (default_case) {
        auto default_stmt = TRY_AND_CAST(Statement, default_case->statement(), ctx);
        branches.push_back(std::make_shared<BoundBranch>(default_case->location(), nullptr, default_stmt));
    }
    return TRY(process(std::make_shared<BoundIfStatement>(switch_stmt->location(), branches), ctx, result));
}

NODE_PROCESSOR(BoundWhileStatement)
{
    auto while_stmt = std::dynamic_pointer_cast<BoundWhileStatement>(tree);
    auto condition = TRY_AND_CAST(BoundExpression, while_stmt->condition(), ctx);
    auto stmt = TRY_AND_CAST(Statement, while_stmt->statement(), ctx);

    if (ctx.config().target == Architecture::C_TRANSPILER) {
        return std::make_shared<BoundWhileStatement>(while_stmt, condition, stmt);
    }

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

    Statements while_block;
    auto start_of_loop = std::make_shared<Label>(while_stmt->location());
    auto jump_out_of_loop = std::make_shared<Goto>(while_stmt->location());
    while_block.push_back(start_of_loop);

    BoundBranches branches {
        std::make_shared<BoundBranch>(while_stmt->location(),
            std::make_shared<BoundUnaryExpression>(condition->location(),
                condition, UnaryOperator::LogicalInvert, ObjectType::get(PrimitiveType::Boolean)),
            jump_out_of_loop),
    };
    while_block.push_back(std::make_shared<BoundIfStatement>(condition->location(), branches));
    while_block.push_back(stmt);
    while_block.push_back(std::make_shared<Goto>(while_stmt->location(), start_of_loop));
    while_block.push_back(std::make_shared<Label>(jump_out_of_loop));
    return TRY(process(std::make_shared<Block>(while_stmt->location(), while_block), ctx, result));
}

NODE_PROCESSOR(BoundForStatement)
{
    if (ctx.config().target == Architecture::C_TRANSPILER) {
        auto for_stmt = std::dynamic_pointer_cast<BoundForStatement>(tree);
        auto variable = TRY_AND_CAST(BoundVariable, for_stmt->variable(), ctx);
        auto range = std::dynamic_pointer_cast<BoundBinaryExpression>(for_stmt->range());
        auto range_low = TRY_AND_CAST(BoundExpression, range->lhs(), ctx);
        auto range_high = TRY_AND_CAST(BoundExpression, range->rhs(), ctx);
        range = std::make_shared<BoundBinaryExpression>(range->location(), range_low, BinaryOperator::Range, range_high, range->type());
        auto stmt = TRY_AND_CAST(Statement, for_stmt->statement(), ctx);
        return std::make_shared<BoundForStatement>(for_stmt, variable, range, stmt);
    }

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
    auto range_expr = TRY_AND_CAST(BoundExpression, for_stmt->range(), ctx);
    auto stmt = TRY_AND_CAST(Statement, for_stmt->statement(), ctx);

    if (range_expr->node_type() != SyntaxNodeType::BoundBinaryExpression)
        return SyntaxError { for_stmt->location(), "Invalid for-loop range" };
    auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(range_expr);
    if (range_binary_expr->op() != BinaryOperator::Range)
        return SyntaxError { range_expr->location(), "Invalid for-loop range" };
    auto variable_type = for_stmt->variable()->type();

    Statements for_block;

    if (for_stmt->must_declare_variable()) {
        for_block.push_back(
            std::make_shared<BoundVariableDeclaration>(for_stmt->location(), for_stmt->variable(), false, range_binary_expr->lhs()));
    } else {
        for_block.push_back(
            std::make_shared<BoundExpressionStatement>(for_stmt->location(),
                std::make_shared<BoundAssignment>(for_stmt->location(),
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

    for_block.push_back(std::make_shared<BoundIfStatement>(for_stmt->location(),
        BoundBranches {
            std::make_shared<BoundBranch>(for_stmt->location(),
                std::make_shared<BoundBinaryExpression>(for_stmt->location(),
                    for_stmt->variable(), BinaryOperator::GreaterEquals, rhs, ObjectType::get(PrimitiveType::Boolean)),
                jump_past_loop),
        }));
    for_block.push_back(stmt);
    for_block.push_back(
        std::make_shared<BoundExpressionStatement>(range_binary_expr->location(),
            std::make_shared<BoundUnaryExpression>(range_binary_expr->location(),
                for_stmt->variable(),
                UnaryOperator::UnaryIncrement,
                variable_type)));
    for_block.push_back(std::make_shared<Goto>(for_stmt->location(), jump_back_label));
    for_block.push_back(std::make_shared<Label>(jump_past_loop));
    return TRY(process(std::make_shared<Block>(stmt->location(), for_block), ctx, result));
}

NODE_PROCESSOR(BoundBinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, expr->lhs(), ctx);
    auto rhs = TRY_AND_CAST(BoundExpression, expr->rhs(), ctx);

    if (expr->op() == BinaryOperator::Range) {
        return tree;
    }

    if (expr->op() == BinaryOperator::GreaterEquals) {
        return std::make_shared<BoundBinaryExpression>(expr->location(),
            std::make_shared<BoundBinaryExpression>(expr->location(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            BinaryOperator::LogicalOr,
            std::make_shared<BoundBinaryExpression>(expr->location(),
                lhs, BinaryOperator::Greater, rhs, ObjectType::get(PrimitiveType::Boolean)),
            ObjectType::get(PrimitiveType::Boolean));
    }

    if (expr->op() == BinaryOperator::LessEquals) {
        return std::make_shared<BoundBinaryExpression>(expr->location(),
            std::make_shared<BoundBinaryExpression>(expr->location(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            BinaryOperator::LogicalOr,
            std::make_shared<BoundBinaryExpression>(expr->location(),
                lhs, BinaryOperator::Less, rhs, ObjectType::get(PrimitiveType::Boolean)),
            ObjectType::get(PrimitiveType::Boolean));
    }

    if (expr->op() == BinaryOperator::NotEquals) {
        return std::make_shared<BoundUnaryExpression>(expr->location(),
            std::make_shared<BoundBinaryExpression>(expr->location(),
                lhs, BinaryOperator::Equals, rhs, ObjectType::get(PrimitiveType::Boolean)),
            UnaryOperator::LogicalInvert,
            ObjectType::get(PrimitiveType::Boolean));
    }

    return expr;
}

NODE_PROCESSOR(BoundUnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, expr->operand(), ctx);
    if (expr->op() == UnaryOperator::UnaryIncrement || expr->op() == UnaryOperator::UnaryDecrement) {
        auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(operand);
        auto new_rhs = std::make_shared<BoundBinaryExpression>(expr->location(),
            identifier,
            (expr->op() == UnaryOperator::UnaryIncrement) ? BinaryOperator::Add : BinaryOperator::Subtract,
            std::make_shared<BoundIntLiteral>(expr->location(), 1l, identifier->type()),
            identifier->type());
        return std::make_shared<BoundAssignment>(expr->location(), identifier, new_rhs);
    }
    return tree;
}

ProcessResult& lower(Config const& config, ProcessResult& result)
{
    if (result.is_error())
        return result;
    LowerContext ctx { config };
    process<LowerContext>(result.value(), ctx, result);
    if (result.is_error())
        return result;
    resolve_operators(result);
    return result;
}

}
