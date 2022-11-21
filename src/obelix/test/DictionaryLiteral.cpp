/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "ParserTest.h"
#include <gtest/gtest.h>

namespace Obelix {

TEST_F(ParserTest, DictionaryLiteral)
{
    auto scope = parse("var a = { foo: 12, bar: 42 };");
    EXPECT_EQ(scope->result().return_value->type(), TypeObject);
    auto dict = ptr_cast<Dictionary>(scope->result().return_value);
    EXPECT_EQ(dict->size(), 2);
    EXPECT_EQ(dict->get("foo"), make_typed<Integer>(12));
    EXPECT_EQ(dict->get("bar"), make_typed<Integer>(42));
}

TEST_F(ParserTest, EmptyDictionaryLiteral)
{
    auto scope = parse("var a = { };");
    EXPECT_EQ(scope->result().return_value->type(), TypeObject);
    auto dict = ptr_cast<Dictionary>(scope->result().return_value);
    EXPECT_EQ(dict->size(), 0);
}

TEST_F(ParserTest, DictionaryLiteralNoClose)
{
    auto scope = parse("var a = { foo: 12, bar: 42; var b = a;");
    EXPECT_EQ(scope->result().code, ExecutionResultCode::Error);
    EXPECT_EQ(scope->result().return_value->type(), TypeList);
    Ptr<List> list = ptr_cast<List>(scope->result().return_value);
    EXPECT_EQ(list->size(), 1);
    EXPECT_EQ(list[0]->type(), TypeException);
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
