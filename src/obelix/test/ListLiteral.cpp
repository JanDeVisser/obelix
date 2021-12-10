/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ParserTest.h"
#include <gtest/gtest.h>

namespace Obelix {

TEST_F(ParserTest, ListLiteral)
{
    auto scope = parse("[ 1, 2 ];");
    EXPECT_EQ(scope->result().return_value->type_name(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 2);
    EXPECT_EQ(list[0], make_typed<Integer>(1));
    EXPECT_EQ(list[1], make_typed<Integer>(2));
}

TEST_F(ParserTest, EmptyListLiteral)
{
    auto scope = parse("[ ];");
    EXPECT_EQ(scope->result().return_value->type_name(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 0);
}

TEST_F(ParserTest, ListLiteralNoClose)
{
    auto scope = parse("var a = [ 1, 2 ; var b = a;");
    EXPECT_EQ(scope->result().code, ExecutionResultCode::Error);
    EXPECT_EQ(scope->result().return_value->type_name(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 1);
    EXPECT_EQ(list[0]->type_name(), "exception");
}

TEST_F(ParserTest, ListLiteralIterate)
{
    auto scope = parse("var x = 0; for (i in [1, 2]) { x = x + i; }");
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 3);
}

}
