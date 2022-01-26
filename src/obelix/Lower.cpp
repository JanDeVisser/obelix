/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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

    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        if (func_def->statement()) {
            auto statement = TRY_AND_CAST(Statement, lower_processor(func_def->statement(), ctx));
            switch (statement->node_type()) {
            case SyntaxNodeType::Block: {
                auto block = std::dynamic_pointer_cast<Block>(statement);
                return std::make_shared<FunctionDef>(func_def->declaration(), std::make_shared<FunctionBlock>(block->statements()));
            }
            case SyntaxNodeType::FunctionBlock: {
                auto block = std::dynamic_pointer_cast<FunctionBlock>(statement);
                return std::make_shared<FunctionDef>(func_def->declaration(), block);
            }
            default:
                return std::make_shared<FunctionDef>(func_def->declaration(), std::make_shared<FunctionBlock>(statement));
            }
        }
        return tree;
    }

    case SyntaxNodeType::SwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
        auto switch_expr = TRY_AND_CAST(Expression, lower_processor(switch_stmt->expression(), ctx));
        auto default_case = TRY_AND_CAST(CaseStatement, lower_processor(switch_stmt->default_case(), ctx));

        CaseStatements cases;
        for (auto& c : switch_stmt->cases()) {
            auto new_case = TRY_AND_CAST(CaseStatement, lower_processor(c, ctx));
            cases.push_back(new_case);
        }

        Branches branches;
        for (auto& c : cases) {
            branches.push_back(std::make_shared<Branch>(
                std::make_shared<BinaryExpression>(switch_expr, Token { TokenCode::EqualsTo, "==" }, c->condition()),
                c->statement()));
        }
        if (default_case)
            branches.push_back(std::make_shared<Branch>(nullptr, default_case->statement()));
        return std::make_shared<IfStatement>(branches);
    }

    case SyntaxNodeType::ForStatement: {
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
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        auto range_expr = TRY_AND_CAST(Expression, lower_processor(for_stmt->range(), ctx));
        auto stmt = TRY_AND_CAST(Statement, lower_processor(for_stmt->statement(), ctx));

        Statements for_block;

        if (range_expr->node_type() != SyntaxNodeType::BinaryExpression)
            return Error { ErrorCode::SyntaxError, "Invalid for-loop range" };
        auto range_binary_expr = std::dynamic_pointer_cast<BinaryExpression>(range_expr);
        if (range_binary_expr->op().code() != Parser::KeywordRange)
            return Error { ErrorCode::SyntaxError, "Invalid for-loop range" };

        for_block.push_back(
            std::make_shared<VariableDeclaration>(
                Symbol { for_stmt->variable(), ObelixType::TypeInt },
                range_binary_expr->lhs()));
        auto jump_past_loop = std::make_shared<Goto>();
        auto jump_back_label = std::make_shared<Label>();
        for_block.push_back(
            std::make_shared<IfStatement>(
                std::make_shared<BinaryExpression>(
                    std::make_shared<Identifier>(for_stmt->variable()),
                    Token(TokenCode::GreaterEqualThan, ">="),
                    range_binary_expr->rhs()),
                jump_past_loop));
        for_block.push_back(stmt);
        for_block.push_back(std::make_shared<ExpressionStatement>(
            std::make_shared<BinaryExpression>(
                std::make_shared<Identifier>(Symbol { for_stmt->variable(), ObelixType::TypeInt }),
                Token { TokenCode::Equals, "= " },
                std::make_shared<BinaryExpression>(
                    std::make_shared<Identifier>(Symbol { for_stmt->variable(), ObelixType::TypeInt }),
                    Token { TokenCode::Plus, "+" },
                    std::make_shared<Literal>(make_obj<Integer>(1))))));
        for_block.push_back(std::make_shared<Goto>(jump_back_label));
        for_block.push_back(std::make_shared<Label>(jump_past_loop));
        return std::make_shared<Block>(for_block);
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
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        auto condition = TRY_AND_CAST(Expression, lower_processor(while_stmt->condition(), ctx));
        auto stmt = TRY_AND_CAST(Statement, lower_processor(while_stmt->statement(), ctx));

        Statements while_block;
        auto start_of_loop = std::make_shared<Label>();
        auto jump_out_of_loop = std::make_shared<Goto>();
        while_block.push_back(start_of_loop);
        while_block.push_back(
            std::make_shared<IfStatement>(
                std::make_shared<UnaryExpression>(
                    Token(TokenCode::ExclamationPoint, "!"),
                    condition, condition->type()),
                jump_out_of_loop));
        while_block.push_back(stmt);
        while_block.push_back(std::make_shared<Goto>(start_of_loop));
        while_block.push_back(std::make_shared<Label>(jump_out_of_loop));
        return std::make_shared<Block>(while_block);
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto lhs = TRY_AND_CAST(Expression, lower_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, lower_processor(expr->rhs(), ctx));

        if (expr->op().code() == TokenCode::Equals && lhs->node_type() == SyntaxNodeType::Identifier) {
            auto identifier = std::dynamic_pointer_cast<Identifier>(lhs);
            return std::make_shared<Assignment>(identifier, rhs);
        }

        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        if (Parser::is_assignment_operator(expr->op().code()) && lhs->node_type() == SyntaxNodeType::Identifier) {
            auto identifier = std::dynamic_pointer_cast<Identifier>(lhs);
            auto new_rhs = std::make_shared<BinaryExpression>(
                std::make_shared<Identifier>(identifier->identifier()), Parser::operator_for_assignment_operator(expr->op().code()), rhs, expr->type());
            return std::make_shared<Assignment>(identifier, new_rhs);
        }

        if (expr->op().code() == TokenCode::GreaterEqualThan) {
            return std::make_shared<BinaryExpression>(
                std::make_shared<BinaryExpression>(lhs, Token { TokenCode::Equals, "==" }, rhs, ObelixType::TypeBoolean),
                Token { TokenCode::LogicalOr, "||" },
                std::make_shared<BinaryExpression>(lhs, Token { TokenCode::GreaterThan, ">" }, rhs, ObelixType::TypeBoolean),
                ObelixType::TypeBoolean);
        }

        if (expr->op().code() == TokenCode::LessEqualThan) {
            return std::make_shared<BinaryExpression>(
                std::make_shared<BinaryExpression>(lhs, Token { TokenCode::Equals, "==" }, rhs, ObelixType::TypeBoolean),
                Token { TokenCode::LogicalOr, "||" },
                std::make_shared<BinaryExpression>(lhs, Token { TokenCode::LessThan, "<" }, rhs, ObelixType::TypeBoolean),
                ObelixType::TypeBoolean);
        }

        if (expr->op().code() == TokenCode::NotEqualTo) {
            return std::make_shared<UnaryExpression>(Token { TokenCode::ExclamationPoint, "!" },
                std::make_shared<BinaryExpression>(lhs, Token { TokenCode::Equals, "==" }, rhs, ObelixType::TypeBoolean),
                ObelixType::TypeBoolean);
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
