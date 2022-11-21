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

TEST(StringUtil, Join)
{
    std::vector<std::string> collection;
    collection.emplace_back("ab");
    collection.emplace_back("cd");
    collection.emplace_back("ef");
    auto joined = Obelix::join(collection, ";");
    EXPECT_EQ(joined, "ab;cd;ef");
}

TEST(StringUtil, JoinEmptyElement)
{
    std::vector<std::string> collection;
    collection.emplace_back("ab");
    collection.emplace_back("");
    collection.emplace_back("ef");
    auto joined = Obelix::join(collection, ";");
    EXPECT_EQ(joined, "ab;;ef");
}

TEST(StringUtil, JoinEmptyCollection)
{
    std::vector<std::string> collection;
    auto joined = Obelix::join(collection, ";");
    EXPECT_EQ(joined, "");
}

TEST(StringUtil, JoinOneElement)
{
    std::vector<std::string> collection;
    collection.emplace_back("ab");
    auto joined = Obelix::join(collection, ";");
    EXPECT_EQ(joined, "ab");
}

TEST(StringUtil, JoinEmptySep)
{
    std::vector<std::string> collection;
    collection.emplace_back("ab");
    collection.emplace_back("cd");
    collection.emplace_back("ef");
    auto joined = Obelix::join(collection, "");
    EXPECT_EQ(joined, "abcdef");
}

TEST(StringUtil, JoinSepChar)
{
    std::vector<std::string> collection;
    collection.emplace_back("ab");
    collection.emplace_back("cd");
    collection.emplace_back("ef");
    auto joined = Obelix::join(collection, ';');
    EXPECT_EQ(joined, "ab;cd;ef");
}
