/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "ParserTest.h"
#include <gtest/gtest.h>

namespace Obelix {

typedef ParserTest ListComprehensionTest;

TEST_F(ListComprehensionTest, ListComprehension)
{
    auto scope = parse("[ 2*x for x in [1,2] ];");
    EXPECT_EQ(scope->result().return_value->type_name(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 2);
    EXPECT_EQ(list[0], make_typed<Integer>(2));
    EXPECT_EQ(list[1], make_typed<Integer>(4));
}

TEST_F(ListComprehensionTest, ListComprehensionWhere)
{
    auto scope = parse("[ 2*x for x in [1,2,3] where x != 2];");
    EXPECT_EQ(scope->result().return_value->type_name(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 2);
    EXPECT_EQ(list[0], make_typed<Integer>(2));
    EXPECT_EQ(list[1], make_typed<Integer>(6));
}

TEST_F(ListComprehensionTest, NoGenerator)
{
    auto scope = parse("[ 2*x for ];");
    EXPECT_EQ(scope->result().code, ExecutionResultCode::Error);
    EXPECT_EQ(scope->result().return_value->type_name(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 1);
    EXPECT_EQ(list[0]->type_name(), "exception");
}

}
