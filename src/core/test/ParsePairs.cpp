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

TEST(StringUtil, ParsePairs)
{
    std::string input = "foo=bar;quux=frob";
    auto pairs = Obelix::parse_pairs(input, ';', '=');
    EXPECT_EQ(pairs.size(), 2);
    EXPECT_EQ(pairs[0].first, "foo");
    EXPECT_EQ(pairs[0].second, "bar");
    EXPECT_EQ(pairs[1].first, "quux");
    EXPECT_EQ(pairs[1].second, "frob");
}

TEST(StringUtil, ParsePairsWithSpaces)
{
    std::string input = "  foo = bar ;   quux = frob   ";
    auto pairs = Obelix::parse_pairs(input, ';', '=');
    EXPECT_EQ(pairs.size(), 2);
    EXPECT_EQ(pairs[0].first, "foo");
    EXPECT_EQ(pairs[0].second, "bar");
    EXPECT_EQ(pairs[1].first, "quux");
    EXPECT_EQ(pairs[1].second, "frob");
}


TEST(StringUtil, ParsePairsWithEmptyElement)
{
    std::string input = "  foo = bar ;  ; quux = frob   ";
    auto pairs = Obelix::parse_pairs(input, ';', '=');
    EXPECT_EQ(pairs.size(), 2);
    EXPECT_EQ(pairs[0].first, "foo");
    EXPECT_EQ(pairs[0].second, "bar");
    EXPECT_EQ(pairs[1].first, "quux");
    EXPECT_EQ(pairs[1].second, "frob");
}
