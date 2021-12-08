/*
* AsmTest.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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

#include <cstdio>
#include <gtest/gtest.h>

#include <oblasm/Assembler.h>

#include <oblasm/Assembler.h>

namespace Obelix::Assembler {

TEST(AsmTest, ParseNop)
{
    Image image;

    Assembler assembler(image);
    assembler.parse("nop");
    EXPECT_EQ(image.current_address(), 1);
    auto bytes = image.assemble();
    ASSERT_GE(bytes.size(), 1);
    EXPECT_EQ(bytes.size(), 1);
    EXPECT_EQ(bytes[0], 0);
}

TEST(AsmTest, ParseMovAImm)
{
    Image image;

    Assembler assembler(image);
    assembler.parse("mov a,#$55");
    EXPECT_EQ(image.current_address(), 2);
    auto bytes = image.assemble();
    ASSERT_GE(bytes.size(), 2);
    EXPECT_EQ(bytes.size(), 2);
    EXPECT_EQ(bytes[0], 1);
    EXPECT_EQ(bytes[1], 0x55);
}

TEST(AsmTest, ParseMovAB)
{
    Image image;

    Assembler assembler(image);
    assembler.parse("mov a,b");
    EXPECT_EQ(image.current_address(), 1);
    auto bytes = image.assemble();
    ASSERT_GE(bytes.size(), 1);
    EXPECT_EQ(bytes.size(), 1);
    EXPECT_EQ(bytes[0], 3);
}

TEST(AsmTest, ParseMovAImmBA)
{
    Image image;

    Assembler assembler(image);
    assembler.parse(
        "mov a,#$55 "
        "mov b,a"
        );
    EXPECT_EQ(image.current_address(), 3);
    auto bytes = image.assemble();
    ASSERT_GE(bytes.size(), 3);
    EXPECT_EQ(bytes.size(), 3);
    EXPECT_EQ(bytes[0], 1);
    EXPECT_EQ(bytes[1], 0x55);
    EXPECT_EQ(bytes[2], 3);
    image.list(true);
}



}
