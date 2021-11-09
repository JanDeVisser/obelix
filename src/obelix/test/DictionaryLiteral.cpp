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

TEST_F(ParserTest, DictionaryLiteral)
{
    auto scope = parse("var a = { foo: 12, bar: 42 };");
    EXPECT_EQ(scope->result().return_value->type(), "dictionary");
    auto dict = ptr_cast<Dictionary>(scope->result().return_value);
    EXPECT_EQ(dict->size(), 2);
    EXPECT_EQ(dict->get("foo"), make_typed<Integer>(12));
    EXPECT_EQ(dict->get("bar"), make_typed<Integer>(42));
}

TEST_F(ParserTest, EmptyDictionaryLiteral)
{
    auto scope = parse("var a = { };");
    EXPECT_EQ(scope->result().return_value->type(), "dictionary");
    auto dict = ptr_cast<Dictionary>(scope->result().return_value);
    EXPECT_EQ(dict->size(), 0);
}

TEST_F(ParserTest, DictionaryLiteralNoClose)
{
    auto scope = parse("var a = { foo: 12, bar: 42; var b = a;");
    EXPECT_EQ(scope->result().code, ExecutionResultCode::Error);
    EXPECT_EQ(scope->result().return_value->type(), "list");
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 1);
    EXPECT_EQ(list[0]->type(), "exception");
}

TEST_F(ParserTest, DictionaryLiteralIterate)
{
    auto scope = parse("var x = 0; for (i in { foo: 12, bar: 42 }) { x = x + i.value; }");
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 54);
}

}
