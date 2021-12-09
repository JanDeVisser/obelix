/*
 * AsmTest.h - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

namespace Obelix::Assembler {

#define CHECK_SIMPLE_INSTR(s, code)              \
    TEST(AsmTest, ParseCode##code)               \
    {                                            \
        Image image;                             \
        Assembler assembler(image);              \
        assembler.parse((s));                    \
        ASSERT_TRUE(assembler.was_successful()); \
        EXPECT_EQ(image.current_address(), 1);   \
        auto bytes = image.assemble();           \
        ASSERT_GE(bytes.size(), 1);              \
        EXPECT_EQ(bytes.size(), 1);              \
        EXPECT_EQ(bytes[0], code);               \
    }

#define CHECK_8BIT_IMM(s, code)                  \
    TEST(AsmTest, ParseCode##code)               \
    {                                            \
        Image image;                             \
                                                 \
        Assembler assembler(image);              \
        assembler.parse(format((s), "#$55"));    \
        ASSERT_TRUE(assembler.was_successful()); \
        EXPECT_EQ(image.current_address(), 2);   \
        auto bytes = image.assemble();           \
        ASSERT_GE(bytes.size(), 2);              \
        EXPECT_EQ(bytes.size(), 2);              \
        EXPECT_EQ(bytes[0], (code));             \
        EXPECT_EQ(bytes[1], 0x55);               \
    }

#define CHECK_16BIT_IMM(s, code)                 \
    TEST(AsmTest, ParseCode##code)               \
    {                                            \
        Image image;                             \
                                                 \
        Assembler assembler(image);              \
        assembler.parse(format((s), "#$c0de"));  \
        ASSERT_TRUE(assembler.was_successful()); \
        EXPECT_EQ(image.current_address(), 3);   \
        auto bytes = image.assemble();           \
        ASSERT_GE(bytes.size(), 3);              \
        EXPECT_EQ(bytes.size(), 3);              \
        EXPECT_EQ(bytes[0], (code));             \
        EXPECT_EQ(bytes[1], 0xde);               \
        EXPECT_EQ(bytes[2], 0xc0);               \
    }

#define CHECK_16BIT_IMM_IND(s, code)             \
    TEST(AsmTest, ParseCode##code)               \
    {                                            \
        Image image;                             \
                                                 \
        Assembler assembler(image);              \
        assembler.parse(format((s), "*$c0de"));  \
        ASSERT_TRUE(assembler.was_successful()); \
        EXPECT_EQ(image.current_address(), 3);   \
        auto bytes = image.assemble();           \
        ASSERT_GE(bytes.size(), 3);              \
        EXPECT_EQ(bytes.size(), 3);              \
        EXPECT_EQ(bytes[0], (code));             \
        EXPECT_EQ(bytes[1], 0xde);               \
        EXPECT_EQ(bytes[2], 0xc0);               \
    }

#define CHECK_INSTR(name, s, num, ...)             \
    TEST(AsmTest, Parse##name)                     \
    {                                              \
        uint8_t out[] = { __VA_ARGS__ };           \
        Image image;                               \
        Assembler assembler(image);                \
        assembler.parse((s));                      \
        ASSERT_TRUE(assembler.was_successful());   \
        EXPECT_EQ(image.current_address(), (num)); \
        auto bytes = image.assemble();             \
        ASSERT_GE(bytes.size(), (num));            \
        EXPECT_EQ(bytes.size(), (num));            \
        for (auto ix = 0; ix < num; ++ix) {        \
            EXPECT_EQ(bytes[ix], out[ix]);         \
        }                                          \
    }

#define CHECK_ERROR(name, s)                      \
    TEST(AsmTest, ParseError##name)               \
    {                                             \
        Image image;                              \
        Assembler assembler(image);               \
        assembler.parse((s));                     \
        ASSERT_FALSE(assembler.was_successful()); \
    }

#define CHECK_ASSEMBLY_ERROR(name, s, num)         \
    TEST(AsmTest, ParseError##name)                \
    {                                              \
        Image image;                               \
        Assembler assembler(image);                \
        assembler.parse((s));                      \
        EXPECT_TRUE(assembler.was_successful());   \
        ASSERT_TRUE(assembler.was_successful());   \
        EXPECT_EQ(image.current_address(), (num)); \
        auto bytes = image.assemble();             \
        ASSERT_FALSE(image.errors().empty());      \
    }

}

