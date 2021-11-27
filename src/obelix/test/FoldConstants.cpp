/*
* FoldConstants.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <cstdio>

#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <gtest/gtest.h>

namespace Obelix {

TEST(FoldConstants, Fold)
{
    auto expr = std::make_shared<BinaryExpression>(
        std::make_shared<Literal>(make_obj<Integer>(2)),
        Token(TokenCode::Plus, "+"),
        std::make_shared<Literal>(make_obj<Integer>(3)));
    auto folded_or_error = fold_constants(expr);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::Literal);
    auto literal = std::dynamic_pointer_cast<Literal>(folded);
    EXPECT_EQ(literal->literal()->type(), ObelixType::TypeInt);
    EXPECT_EQ(literal->literal()->to_long().value(), 5);
}

TEST(FoldConstants, DontFold)
{
    Expressions params;
    params.push_back(
        std::make_shared<Literal>(make_obj<Integer>(3))
    );
    auto expr = std::make_shared<BinaryExpression>(
        std::make_shared<Literal>(make_obj<Integer>(2)),
        Token(TokenCode::Plus, "+"),
        std::make_shared<FunctionCall>(
            std::make_shared<Identifier>("foo"), params));
    auto folded_or_error = fold_constants(expr);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::BinaryExpression);
    auto binary = std::dynamic_pointer_cast<BinaryExpression>(folded);
    ASSERT_EQ(binary->lhs()->node_type(), SyntaxNodeType::Literal);
    ASSERT_EQ(binary->rhs()->node_type(), SyntaxNodeType::FunctionCall);
}

TEST(FoldConstants, FoldRight)
{
    auto rhs = std::make_shared<BinaryExpression>(
        std::make_shared<Literal>(make_obj<Integer>(2)),
        Token(TokenCode::Plus, "+"),
        std::make_shared<Identifier>("x"));
    auto expr = std::make_shared<BinaryExpression>(
        std::make_shared<Literal>(make_obj<Integer>(3)),
        Token(TokenCode::Plus, "+"),
        rhs);
    auto folded_or_error = fold_constants(expr);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::BinaryExpression);
    auto binary = std::dynamic_pointer_cast<BinaryExpression>(folded);
    ASSERT_EQ(binary->lhs()->node_type(), SyntaxNodeType::Literal);
    ASSERT_EQ(binary->rhs()->node_type(), SyntaxNodeType::Identifier);
    auto literal = std::dynamic_pointer_cast<Literal>(binary->lhs());
    EXPECT_EQ(literal->literal()->type(), ObelixType::TypeInt);
    EXPECT_EQ(literal->literal()->to_long().value(), 5);
    auto identifier = std::dynamic_pointer_cast<Identifier>(binary->rhs());
    EXPECT_EQ(identifier->name(), "x");
}


TEST(FoldConstants, FoldLeft)
{
    auto lhs = std::make_shared<BinaryExpression>(
        std::make_shared<Identifier>("x"),
        Token(TokenCode::Plus, "+"),
        std::make_shared<Literal>(make_obj<Integer>(2)));
    auto expr = std::make_shared<BinaryExpression>(
        lhs,
        Token(TokenCode::Plus, "+"),
        std::make_shared<Literal>(make_obj<Integer>(3)));
    auto folded_or_error = fold_constants(expr);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::BinaryExpression);
    auto binary = std::dynamic_pointer_cast<BinaryExpression>(folded);
    ASSERT_EQ(binary->rhs()->node_type(), SyntaxNodeType::Literal);
    ASSERT_EQ(binary->lhs()->node_type(), SyntaxNodeType::Identifier);
    auto literal = std::dynamic_pointer_cast<Literal>(binary->rhs());
    EXPECT_EQ(literal->literal()->type(), ObelixType::TypeInt);
    EXPECT_EQ(literal->literal()->to_long().value(), 5);
    auto identifier = std::dynamic_pointer_cast<Identifier>(binary->lhs());
    EXPECT_EQ(identifier->name(), "x");
}

TEST(FoldConstants, IncEquals)
{
    auto expr = std::make_shared<BinaryExpression>(
        std::make_shared<Identifier>("x"),
        Token(Parser::KeywordIncEquals, "+="),
        std::make_shared<Literal>(make_obj<Integer>(3)));

    auto folded_or_error = fold_constants(expr);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::BinaryExpression);

    auto binary = std::dynamic_pointer_cast<BinaryExpression>(folded);
    ASSERT_EQ(binary->lhs()->node_type(), SyntaxNodeType::Identifier);
    EXPECT_EQ(binary->op().code(), TokenCode::Equals);
    ASSERT_EQ(binary->rhs()->node_type(), SyntaxNodeType::BinaryExpression);

    auto identifier = std::dynamic_pointer_cast<Identifier>(binary->lhs());
    EXPECT_EQ(identifier->name(), "x");

    auto binary2 = std::dynamic_pointer_cast<BinaryExpression>(binary->rhs());
    ASSERT_EQ(binary2->lhs()->node_type(), SyntaxNodeType::Identifier);
    EXPECT_EQ(binary2->op().code(), TokenCode::Plus);
    ASSERT_EQ(binary2->rhs()->node_type(), SyntaxNodeType::Literal);

    identifier = std::dynamic_pointer_cast<Identifier>(binary2->lhs());
    EXPECT_EQ(identifier->name(), "x");

    auto literal = std::dynamic_pointer_cast<Literal>(binary2->rhs());
    EXPECT_EQ(literal->literal()->type(), ObelixType::TypeInt);
    EXPECT_EQ(literal->literal()->to_long().value(), 3);
}

TEST(FoldConstants, Unary)
{
    auto expr = std::make_shared<UnaryExpression>(
        Token(TokenCode::Minus, "-"),
        std::make_shared<Literal>(make_obj<Integer>(3)));
    auto folded_or_error = fold_constants(expr);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::Literal);
    auto literal = std::dynamic_pointer_cast<Literal>(folded);
    EXPECT_EQ(literal->literal()->type(), ObelixType::TypeInt);
    EXPECT_EQ(literal->literal()->to_long().value(), -3);
}

TEST(FoldConstants, BinaryWithUnary)
{
    auto expr = std::make_shared<BinaryExpression>(
        std::make_shared<Literal>(make_obj<Integer>(2)),
        Token(TokenCode::Plus, "+"),
        std::make_shared<UnaryExpression>(
            Token(TokenCode::Minus, "-"),
            std::make_shared<Literal>(make_obj<Integer>(3))));
    auto folded_or_error = fold_constants(expr);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::Literal);
    auto literal = std::dynamic_pointer_cast<Literal>(folded);
    EXPECT_EQ(literal->literal()->type(), ObelixType::TypeInt);
    EXPECT_EQ(literal->literal()->to_long().value(), -1);
}

TEST(FoldConstants, ConstVariable)
{
    Statements statements;
    auto var_decl = std::make_shared<VariableDeclaration>(
        Symbol { "x" },
        std::make_shared<Literal>(make_obj<Integer>(3)),
        true);
    auto lhs = std::make_shared<BinaryExpression>(
        std::make_shared<Identifier>("x"),
        Token(TokenCode::Plus, "+"),
        std::make_shared<Literal>(make_obj<Integer>(2)));
    auto expr = std::make_shared<BinaryExpression>(
        lhs,
        Token(TokenCode::Plus, "+"),
        std::make_shared<Literal>(make_obj<Integer>(3)));

    statements.push_back(var_decl);
    statements.push_back(std::make_shared<ExpressionStatement>(expr));
    auto block = std::make_shared<Block>(statements);
    auto folded_or_error = fold_constants(block);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::Block);
    block = std::dynamic_pointer_cast<Block>(folded);
    auto stmts = block->statements();
    ASSERT_EQ(stmts.size(), 2);
    ASSERT_EQ(stmts[1]->node_type(), SyntaxNodeType::ExpressionStatement);
    auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmts[1]);
    ASSERT_EQ(expr_stmt->expression()->node_type(), SyntaxNodeType::Literal);
    auto literal = std::dynamic_pointer_cast<Literal>(expr_stmt->expression());
    EXPECT_EQ(literal->literal()->to_long().value(), 8);
}

TEST(FoldConstants, NotConstVariable)
{
    Statements statements;
    auto var_decl = std::make_shared<VariableDeclaration>(
        Symbol { "x" },
        std::make_shared<Literal>(make_obj<Integer>(3)),
        false);
    auto lhs = std::make_shared<BinaryExpression>(
        std::make_shared<Identifier>("x"),
        Token(TokenCode::Plus, "+"),
        std::make_shared<Literal>(make_obj<Integer>(2)));
    auto expr = std::make_shared<BinaryExpression>(
        lhs,
        Token(TokenCode::Plus, "+"),
        std::make_shared<Literal>(make_obj<Integer>(3)));

    statements.push_back(var_decl);
    statements.push_back(std::make_shared<ExpressionStatement>(expr));
    auto block = std::make_shared<Block>(statements);
    auto folded_or_error = fold_constants(block);
    ASSERT_TRUE(folded_or_error.has_value());
    auto folded = folded_or_error.value();
    ASSERT_EQ(folded->node_type(), SyntaxNodeType::Block);
    block = std::dynamic_pointer_cast<Block>(folded);
    auto stmts = block->statements();
    ASSERT_EQ(stmts.size(), 2);
    ASSERT_EQ(stmts[1]->node_type(), SyntaxNodeType::ExpressionStatement);
    auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(stmts[1]);
    ASSERT_EQ(expr_stmt->expression()->node_type(), SyntaxNodeType::BinaryExpression);
}

}
