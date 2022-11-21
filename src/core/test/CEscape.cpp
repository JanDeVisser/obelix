/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <core/StringUtil.h>
#include <gtest/gtest.h>

TEST(StringUtil, c_escape_DQuote)
{
    std::string input = "ab\"cd\"ef";
    EXPECT_EQ("ab\\\"cd\\\"ef", Obelix::c_escape(input));
}

TEST(StringUtil, c_escape_SQuote) {
    std::string input = "ab\'cd\'ef";
    EXPECT_EQ("ab\\\'cd\\\'ef", Obelix::c_escape(input));
}

TEST(StringUtil, c_escape_Backslash) {
    std::string input = "ab\\cdef";
    EXPECT_EQ("ab\\\\cdef", Obelix::c_escape(input));
}

TEST(StringUtil, c_escape_Leading) {
    std::string input = "\\abcdef";
    EXPECT_EQ("\\\\abcdef", Obelix::c_escape(input));
}

TEST(StringUtil, c_escape_Trailing) {
    std::string input = "abcdef\\";
    EXPECT_EQ("abcdef\\\\", Obelix::c_escape(input));
}

TEST(StringUtil, c_escape_QuotedString) {
    std::string input = "\"abcdef\'";
    EXPECT_EQ("\\\"abcdef\\\'", Obelix::c_escape(input));
}
