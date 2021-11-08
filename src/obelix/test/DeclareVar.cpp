/*
 * DeclareVar.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix2.
 *
 * obelix2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix2.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ParserTest.h"
#include <gtest/gtest.h>

namespace Obelix {

TEST_F(ParserTest, DeclareVar)
{
    auto scope = parse("var x = 0;");
    EXPECT_NE(scope->result().code, ExecutionResultCode::Error);
    auto errors = ptr_cast<List>(scope->result().return_value);
    auto x_opt = scope->resolve("x");
    EXPECT_TRUE(x_opt.has_value());
    Ptr<Integer> x = ptr_cast<Integer>(x_opt.value());
    EXPECT_EQ(x->to_long().value(), 0);
}

}
