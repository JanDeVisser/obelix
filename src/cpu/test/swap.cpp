/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Swap
#include "controllertest.h"

namespace Obelix::CPU {

class TESTNAME : public HarnessTest {
};

// SWAP

TEST_F(TESTNAME, swpAB)
{
    mem->initialize(RAM_START, 6, binary_op);
    check_memory(RAM_START, MOV_A_IMM);
    ASSERT_FALSE(mem->poke(0x2002, MOV_B_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2004, SWP_A_B).is_error());

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    check_cycles(16);
    ASSERT_EQ(gp_a->getValue(), 0xF8);
    ASSERT_EQ(gp_b->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpAC)
{
    mem->initialize(RAM_START, 6, binary_op);
    check_memory(RAM_START, MOV_A_IMM);
    ASSERT_FALSE(mem->poke(0x2002, MOV_C_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2004, SWP_A_C).is_error());

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    check_cycles(16);
    ASSERT_EQ(gp_a->getValue(), 0xF8);
    ASSERT_EQ(gp_c->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpAD)
{
    mem->initialize(RAM_START, 6, binary_op);
    check_memory(RAM_START, MOV_A_IMM);
    ASSERT_FALSE(mem->poke(0x2002, MOV_D_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2004, SWP_A_D).is_error());

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    check_cycles(16);
    ASSERT_EQ(gp_a->getValue(), 0xF8);
    ASSERT_EQ(gp_d->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpBC)
{
    mem->initialize(RAM_START, 6, binary_op);
    check_memory(RAM_START, MOV_A_IMM);
    ASSERT_FALSE(mem->poke(0x2000, MOV_B_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2002, MOV_C_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2004, SWP_B_C).is_error());

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    check_cycles(16);
    ASSERT_EQ(gp_b->getValue(), 0xF8);
    ASSERT_EQ(gp_c->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpBD)
{
    mem->initialize(RAM_START, 6, binary_op);
    check_memory(RAM_START, MOV_A_IMM);
    ASSERT_FALSE(mem->poke(0x2000, MOV_B_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2002, MOV_D_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2004, SWP_B_D).is_error());

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    check_cycles(16);
    ASSERT_EQ(gp_b->getValue(), 0xF8);
    ASSERT_EQ(gp_d->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpCD)
{
    mem->initialize(RAM_START, 6, binary_op);
    check_memory(RAM_START, MOV_A_IMM);
    ASSERT_FALSE(mem->poke(0x2000, MOV_C_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2002, MOV_D_IMM).is_error());
    ASSERT_FALSE(mem->poke(0x2004, SWP_C_D).is_error());

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    check_cycles(16);
    ASSERT_EQ(gp_c->getValue(), 0xF8);
    ASSERT_EQ(gp_d->getValue(), 0x1F);
}

}
