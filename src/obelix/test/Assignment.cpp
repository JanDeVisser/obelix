/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "ParserTest.h"
#include <gtest/gtest.h>

namespace Obelix {

TEST_F(ParserTest, Assignment)
{
    auto root_scope = runtime->make_scope();
    root_scope->declare("x", make_obj<Integer>(0));
    auto scope = parse("x = 1;", root_scope);
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 1);
}

TEST_F(ParserTest, AssignExpression)
{
    auto root_scope = runtime->make_scope();
    root_scope->declare("x", make_obj<Integer>(1));
    auto scope = parse("x = x + 1;", root_scope);
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 2);
}

TEST_F(ParserTest, ChainedAssign)
{
    auto root_scope = runtime->make_scope();
    root_scope->declare("a", make_obj<Integer>(2));
    root_scope->declare("b", make_obj<Integer>(0));
    root_scope->declare("c", make_obj<Integer>(0));
    auto scope = parse("a = b = c = 2*a;", root_scope);
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto a_opt = scope->resolve("a");
    EXPECT_TRUE(a_opt.has_value());
    ASSERT_EQ(a_opt.value().type(), TypeInt);
    Ptr<Integer> a = ptr_cast<Integer>(a_opt.value());
    EXPECT_EQ(a->to_long().value(), 4);
    auto b_opt = scope->resolve("b");
    EXPECT_TRUE(b_opt.has_value());
    Ptr<Integer> b = ptr_cast<Integer>(b_opt.value());
    EXPECT_EQ(b->to_long().value(), 4);
    auto c_opt = scope->resolve("c");
    EXPECT_TRUE(c_opt.has_value());
    Ptr<Integer> c = ptr_cast<Integer>(c_opt.value());
    EXPECT_EQ(c->to_long().value(), 4);
}

TEST_F(ParserTest, AssignIncEquals)
{
    auto root_scope = runtime->make_scope();
    root_scope->declare("x", make_obj<Integer>(1));
    auto scope = parse("x += 2;", root_scope);
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 3);
}

TEST_F(ParserTest, LoopIncEquals)
{
    auto root_scope = runtime->make_scope();
    root_scope->declare("x", make_obj<Integer>(1));
    auto scope = parse("while (x < 10) { x += 2; }", root_scope);
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 11);
}

TEST_F(ParserTest, AssignDecEquals)
{
    runtime->declare("x", make_obj<Integer>(3));
    auto scope = parse("x -= 2;");
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto x_opt = runtime->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 1);
}

TEST_F(ParserTest, LoopDecEquals)
{
    runtime->declare("x", make_obj<Integer>(11));
    auto scope = parse("while (x > 0) { x -= 2; }");
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto x_opt = runtime->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), -1);
}

}
