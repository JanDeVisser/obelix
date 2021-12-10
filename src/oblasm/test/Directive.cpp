/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <oblasm/Assembler.h>

namespace Obelix::Assembler {

TEST(AsmTest, ParseSegment)
{
    Image image;

    Assembler assembler(image);
    assembler.parse(
        ".segment $0100 "
        "mov a,b"
        );
    EXPECT_EQ(image.current_address(), 0x0101);
    auto bytes = image.assemble();
    ASSERT_GE(bytes.size(), 0x0101);
    EXPECT_EQ(bytes.size(), 0x0101);
    EXPECT_EQ(bytes[0x0100], 3);
}

TEST(AsmTest, ParseTwoSegments)
{
    Image image;

    Assembler assembler(image);
    assembler.parse(
        ".segment $0100 "
        "mov a,b "
        ".segment $0200 "
        "mov a,c "
    );
    EXPECT_EQ(image.current_address(), 0x0201);
    auto bytes = image.assemble();
    ASSERT_GE(bytes.size(), 0x0201);
    EXPECT_EQ(bytes.size(), 0x0201);
    EXPECT_EQ(bytes[0x0100], 3);
    EXPECT_EQ(bytes[0x0200], 4);
}

}