/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Swap
#include "controllertest.h"

// SWAP

TEST_F(TESTNAME, swpAB)
{
    mem->initialize(RAM_START, 6, binary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_A_CONST);
    (*mem)[0x2002] = MOV_B_CONST;
    (*mem)[0x2004] = SWP_A_B;

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0xF8);
    ASSERT_EQ(gp_b->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpAC)
{
    mem->initialize(RAM_START, 6, binary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_A_CONST);
    (*mem)[0x2002] = MOV_C_CONST;
    (*mem)[0x2004] = SWP_A_C;

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0xF8);
    ASSERT_EQ(gp_c->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpAD)
{
    mem->initialize(RAM_START, 6, binary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_A_CONST);
    (*mem)[0x2002] = MOV_D_CONST;
    (*mem)[0x2004] = SWP_A_D;

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0xF8);
    ASSERT_EQ(gp_d->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpBC)
{
    mem->initialize(RAM_START, 6, binary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_A_CONST);
    (*mem)[0x2000] = MOV_B_CONST;
    (*mem)[0x2002] = MOV_C_CONST;
    (*mem)[0x2004] = SWP_B_C;

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_b->getValue(), 0xF8);
    ASSERT_EQ(gp_c->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpBD)
{
    mem->initialize(RAM_START, 6, binary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_A_CONST);
    (*mem)[0x2000] = MOV_B_CONST;
    (*mem)[0x2002] = MOV_D_CONST;
    (*mem)[0x2004] = SWP_B_D;

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_b->getValue(), 0xF8);
    ASSERT_EQ(gp_d->getValue(), 0x1F);
}

TEST_F(TESTNAME, swpCD)
{
    mem->initialize(RAM_START, 6, binary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_A_CONST);
    (*mem)[0x2000] = MOV_C_CONST;
    (*mem)[0x2002] = MOV_D_CONST;
    (*mem)[0x2004] = SWP_C_D;

    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_c->getValue(), 0xF8);
    ASSERT_EQ(gp_d->getValue(), 0x1F);
}
