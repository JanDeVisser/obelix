/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

//
// Created by Jan de Visser on 2021-09-22.
//

#include <core/StringUtil.h>
#include <gtest/gtest.h>

TEST(StringUtil, Split)
{
    std::string input = "ab;cd;ef";
    auto splitted = Obelix::split(input, ';');
    EXPECT_EQ(3, splitted.size());
    EXPECT_EQ(splitted[0], "ab");
    EXPECT_EQ(splitted[1], "cd");
    EXPECT_EQ(splitted[2], "ef");
}

TEST(StringUtil, SplitEmptyString)
{
    std::string input = "";
    auto splitted = Obelix::split(input, ';');
    EXPECT_EQ(1, splitted.size());
    EXPECT_EQ(splitted[0], "");
}

TEST(StringUtil, SplitEmptyElement)
{
    std::string input = "ab;;ef";
    auto splitted = Obelix::split(input, ';');
    EXPECT_EQ(3, splitted.size());
    EXPECT_EQ(splitted[0], "ab");
    EXPECT_EQ(splitted[1], "");
    EXPECT_EQ(splitted[2], "ef");
}

TEST(StringUtil, SplitLeadingEmpty)
{
    std::string input = ";ab;ef";
    auto splitted = Obelix::split(input, ';');
    EXPECT_EQ(3, splitted.size());
    EXPECT_EQ(splitted[0], "");
    EXPECT_EQ(splitted[1], "ab");
    EXPECT_EQ(splitted[2], "ef");
}

TEST(StringUtil, SplitTrailingEmpty)
{
    std::string input = "ab;ef;";
    auto splitted = Obelix::split(input, ';');
    EXPECT_EQ(3, splitted.size());
    EXPECT_EQ(splitted[0], "ab");
    EXPECT_EQ(splitted[1], "ef");
    EXPECT_EQ(splitted[2], "");
}
