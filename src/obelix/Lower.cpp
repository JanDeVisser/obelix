/*
 * Lower.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

ErrorOrNode lower(std::shared_ptr<SyntaxNode> const& tree)
{
    using LowerContext = Context<int>;
    LowerContext::ProcessorMap lower_map;

    //
    // for (x in 1..5) {
    //   foo(x);
    // }
    // ==>
    // {
    //    var $range = 1..5;
    //    if (!$range.has_next()) goto label_0; -> ! * $range
    // label_1:
    //    x = $range.next();  -> x = @ $range;
    //    foo(x);
    //    if ($range.has_next()) goto label_1;
    // label_0:
    // }
    //
    lower_map[SyntaxNodeType::ForStatement] = [](std::shared_ptr<SyntaxNode> const& tree, LowerContext &ctx) -> ErrorOrNode {
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        Statements for_block;
        for_block.push_back(
            std::make_shared<VariableDeclaration>(
                Symbol { "$range", TypeUnknown },
                for_stmt->range(),
                false));
        for_block.push_back(
            std::make_shared<VariableDeclaration>(
                Symbol { for_stmt->variable(), TypeUnknown }));
        auto jump_past_loop = std::make_shared<Goto>();
        for_block.push_back(
            std::make_shared<IfStatement>(
                std::make_shared<UnaryExpression>(
                    Token(TokenCode::ExclamationPoint),
                    std::make_shared<UnaryExpression>(
                        Token(TokenCode::Asterisk, "*"),
                        std::make_shared<Identifier>("$range"))),
                jump_past_loop));
        auto jump_back_label = std::make_shared<Label>();
        for_block.push_back(jump_back_label);
        for_block.push_back(
            std::make_shared<ExpressionStatement>(
                std::make_shared<BinaryExpression>(
                    std::make_shared<Identifier>(for_stmt->variable()),
                    Token(TokenCode::Equals, "="),
                    std::make_shared<UnaryExpression>(
                        Token(TokenCode::AtSign),
                        std::make_shared<Identifier>("$range")))));
        for_block.push_back(for_stmt->statement());
        for_block.push_back(
            std::make_shared<IfStatement>(
                std::make_shared<UnaryExpression>(
                    Token(TokenCode::Asterisk, "*"),
                    std::make_shared<Identifier>("$range")),
                std::make_shared<Goto>(jump_back_label)));
        for_block.push_back(std::make_shared<Label>(jump_past_loop));
        return std::make_shared<Block>(for_block);
    };

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
    lower_map[SyntaxNodeType::WhileStatement] = [](std::shared_ptr<SyntaxNode> const& tree, LowerContext &ctx) -> ErrorOrNode {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        Statements while_block;
        auto start_of_loop = std::make_shared<Label>();
        auto jump_out_of_loop = std::make_shared<Goto>();
        while_block.push_back(start_of_loop);
        while_block.push_back(
            std::make_shared<IfStatement>(
                std::make_shared<UnaryExpression>(
                    Token(TokenCode::ExclamationPoint, "!"),
                    while_stmt->condition()
                    ),
                jump_out_of_loop
                )
            );
        while_block.push_back(while_stmt->statement());
        while_block.push_back(std::make_shared<Goto>(start_of_loop));
        while_block.push_back(std::make_shared<Label>(jump_out_of_loop));
        return std::make_shared<Block>(while_block);
    };

    lower_map[SyntaxNodeType::BinaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, LowerContext &ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        if (Parser::is_assignment_operator(expr->op().code()) && expr->lhs()->node_type() == SyntaxNodeType::Identifier) {
            auto id = std::dynamic_pointer_cast<Identifier>(expr->lhs());
            auto tok = expr->op().value();
            auto new_rhs = std::make_shared<BinaryExpression>(std::make_shared<Identifier>(id->name()), Parser::operator_for_assignment_operator(expr->op().code()), expr->rhs());
            return std::make_shared<BinaryExpression>(id, Token(TokenCode::Equals, "="), new_rhs);
        }
        return tree;
    };

    LowerContext ctx(lower_map);
    return process_tree(tree, ctx);
}

}
