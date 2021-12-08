/*
 * Directive.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
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
    image.list(true);
    image.dump();
}

}