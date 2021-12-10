/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ParserTest.h"
#include <gtest/gtest.h>

namespace Obelix {

TEST_F(ParserTest, DeclareVar)
{
    auto scope = parse("var x;");
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto errors = ptr_cast<List>(scope->result().return_value);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 0);
}

TEST_F(ParserTest, DeclareVarAndAssignConstant)
{
    auto scope = parse("var x = \"test\";");
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto errors = ptr_cast<List>(scope->result().return_value);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    auto x = ptr_cast<String>(x_opt.value());
    EXPECT_EQ(x->to_string(), "test");
}

TEST_F(ParserTest, DeclareVarAndAssignExpression)
{
    auto scope = parse("var x = 3 + 4;");
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto errors = ptr_cast<List>(scope->result().return_value);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 7);
}

}
