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

TEST(StringUtil, Strip)
{
    std::string input = " \t abcdef  \n";
    auto stripped = Obelix::strip(input);
    EXPECT_EQ(stripped, "abcdef");
}

TEST(StringUtil, StripEmpty)
{
    std::string input = "";
    auto stripped = Obelix::strip(input);
    EXPECT_EQ(stripped, "");
}

TEST(StringUtil, StripAllSpaces)
{
    std::string input = " \t \r    \n";
    auto stripped = Obelix::strip(input);
    EXPECT_EQ(stripped, "");
}

TEST(StringUtil, RStrip)
{
    std::string input = "  abcdef  \n";
    auto stripped = Obelix::rstrip(input);
    EXPECT_EQ(stripped, "  abcdef");
}

TEST(StringUtil, LStrip)
{
    std::string input = "\n  abcdef  \n";
    auto stripped = Obelix::lstrip(input);
    EXPECT_EQ(stripped, "abcdef  \n");
}

TEST(StringUtil, RStripEmpty)
{
    std::string input = "";
    auto stripped = Obelix::rstrip(input);
    EXPECT_EQ(stripped, "");
}

TEST(StringUtil, LStripEmpty)
{
    std::string input = "";
    auto stripped = Obelix::lstrip(input);
    EXPECT_EQ(stripped, "");
}

TEST(StringUtil, RStripAllSpaces)
{
    std::string input = " \t \r    \n";
    auto stripped = Obelix::rstrip(input);
    EXPECT_EQ(stripped, "");
}

TEST(StringUtil, LStripAllSpaces)
{
    std::string input = " \t \r    \n";
    auto stripped = Obelix::lstrip(input);
    EXPECT_EQ(stripped, "");
}
