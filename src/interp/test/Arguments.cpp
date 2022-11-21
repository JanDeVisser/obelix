/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-09-28.
//

#include <core/Arguments.h>
#include <core/Dictionary.h>
#include <core/List.h>
#include <core/Object.h>

#include <gtest/gtest.h>

namespace Obelix {

TEST(Arguments, Instantiate)
{
    auto args = make_typed<Obelix::Arguments>();
    EXPECT_EQ(args->size(), 0);
}

TEST(Arguments, OneObj)
{
    auto args = make_typed<Arguments>(make_obj<Integer>(42));
    EXPECT_EQ(args->size(), 1);
    auto obj = args->get(0);
    EXPECT_TRUE(obj);
    auto i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 42);
}

TEST(Arguments, OneInt)
{
    auto args = make_typed<Arguments>(make_typed<Integer>(42));
    EXPECT_EQ(args->size(), 1);
    auto obj = args->get(0);
    EXPECT_TRUE(obj);
    auto i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 42);
}

TEST(Arguments, TwoInts)
{
    auto args = make_typed<Arguments>(make_typed<Integer>(42), make_typed<Integer>(12));
    EXPECT_EQ(args->size(), 2);
    auto obj = args->get(0);
    EXPECT_TRUE(obj);
    auto i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 42);
    obj = args->get(1);
    EXPECT_TRUE(obj);
    i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 12);
}

TEST(Arguments, TwoIntsOneString)
{
    auto args = make_typed<Arguments>(make_typed<Integer>(42), make_typed<Integer>(12), make_typed<String>("foo"));
    EXPECT_EQ(args->size(), 3);
    auto obj = args->get(0);
    EXPECT_TRUE(obj);
    auto i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 42);
    obj = args->get(1);
    EXPECT_TRUE(obj);
    i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 12);
    obj = args->get(2);
    EXPECT_EQ(obj->type(), TypeString);
    auto s = ptr_cast<String>(obj);
    EXPECT_EQ(s->to_string(), "foo");
}

TEST(Arguments, TwoLongsOneStdString)
{
    auto args = make_typed<Arguments>(42, 12, std::string("foo"));
    EXPECT_EQ(args->size(), 3);
    auto obj = args->get(0);
    EXPECT_TRUE(obj);
    auto i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 42);
    obj = args->get(1);
    EXPECT_TRUE(obj);
    i = ptr_cast<Integer>(obj);
    EXPECT_EQ(i->to_long().value(), 12);
    obj = args->get(2);
    EXPECT_EQ(obj->type(), TypeString);
    auto s = ptr_cast<String>(obj);
    EXPECT_EQ(s->to_string(), "foo");
}

TEST(Arguments, NVP)
{
    auto args = make_typed<Arguments>(make_typed<NVP>("foo", make_obj<Integer>(42)));
    EXPECT_EQ(args->size(), 0);
    EXPECT_EQ(args->kwsize(), 1);
    auto foo_maybe = args->get("foo");
    EXPECT_TRUE(foo_maybe.has_value());
    EXPECT_TRUE(foo_maybe.value()->to_long().has_value());
    EXPECT_EQ(foo_maybe.value()->to_long().value(), 42);
}


TEST(Arguments, StdStringNVP)
{
    auto args = make_typed<Arguments>("foo", make_typed<NVP>("foo", make_obj<Integer>(42)));
    EXPECT_EQ(args->size(), 1);
    EXPECT_EQ(args->kwsize(), 1);
    auto foo_maybe = args->get("foo");
    EXPECT_TRUE(foo_maybe.has_value());
    EXPECT_TRUE(foo_maybe.value()->to_long().has_value());
    EXPECT_EQ(foo_maybe.value()->to_long().value(), 42);
}
}
