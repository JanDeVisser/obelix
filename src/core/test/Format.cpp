//
// Created by Jan de Visser on 2021-09-22.
//

#include <core/Format.h>
#include <core/StringUtil.h>
#include <gtest/gtest.h>

TEST(Format, format_int) {
    std::string formatted = Obelix::format("{}", 42);
    EXPECT_EQ(formatted, "42");
}

TEST(Format, format_string) {
    std::string formatted = Obelix::format("{}", std::string("Hello World!"));
    EXPECT_EQ(formatted, "Hello World!");
}

TEST(Format, format_char)
{
    std::string formatted = Obelix::format("{}","Hello World!");
    EXPECT_EQ(formatted, "Hello World!");
}

TEST(Format, format_char_and_int)
{
    std::string formatted = Obelix::format("String: '{}' int: {}-", "Hello World!", 42);
    EXPECT_EQ(formatted, "String: 'Hello World!' int: {}-");
}
