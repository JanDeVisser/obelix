/*
 * ListLiteral.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ParserTest.h"
#include <gtest/gtest.h>

namespace Obelix {

TEST_F(ParserTest, ListLiteral)
{
    auto scope = parse("[ 1, 2 ];");
    EXPECT_EQ(scope->result().return_value->type(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 2);
    EXPECT_EQ(list[0], make_typed<Integer>(1));
    EXPECT_EQ(list[1], make_typed<Integer>(2));
}

TEST_F(ParserTest, EmptyListLiteral)
{
    auto scope = parse("[ ];");
    EXPECT_EQ(scope->result().return_value->type(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 0);
}

TEST_F(ParserTest, ListLiteralNoClose)
{
    auto scope = parse("var a = [ 1, 2 ; var b = a;");
    EXPECT_EQ(scope->result().code, ExecutionResultCode::Error);
    EXPECT_EQ(scope->result().return_value->type(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 1);
    EXPECT_EQ(list[0]->type(), "exception");
}

}
