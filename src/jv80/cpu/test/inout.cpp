/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME InputOutput
#include "controllertest.h"

namespace Obelix::JV80::CPU {

const byte out[] = {
    /* 2000 */ MOV_A_IMM,
    0x42,
    /* 2002 */ OUT_A,
    CHANNEL_OUT,
    /* 2004 */ HLT,
};

void test_io(Harness* system, byte opcode_init, byte opcode_io, byte channel = CHANNEL_OUT)
{
    auto* mem = dynamic_cast<Memory*>(system->component(MEMADDR));
    mem->initialize(RAM_START, 5, out);
    ASSERT_EQ((*mem)[RAM_START + 2], OUT_A);
    (*mem)[RAM_START] = opcode_init;
    (*mem)[RAM_START + 2] = opcode_io;
    (*mem)[RAM_START + 3] = channel;

    auto* pc = dynamic_cast<AddressRegister*>(system->component(PC));
    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    // mov a, #xx     4 cycles
    // out #xx, a     5 cycles
    // hlt            3 cycles
    // total         12
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 12);
    ASSERT_EQ(system->bus().halt(), false);
}

TEST_F(TESTNAME, outA)
{
    outValue = 0x39;
    test_io(system, MOV_A_IMM, OUT_A);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, outB)
{
    outValue = 0x39;
    test_io(system, MOV_B_IMM, OUT_B);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, outC)
{
    outValue = 0x39;
    test_io(system, MOV_C_IMM, OUT_C);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, outD)
{
    outValue = 0x39;
    test_io(system, MOV_D_IMM, OUT_D);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, inA)
{
    inValue = 0x39;
    test_io(system, MOV_A_IMM, IN_A, CHANNEL_IN);
    ASSERT_EQ(gp_a->getValue(), 0x39);
}

TEST_F(TESTNAME, inB)
{
    inValue = 0x39;
    test_io(system, MOV_B_IMM, IN_B, CHANNEL_IN);
    ASSERT_EQ(gp_b->getValue(), 0x39);
}

TEST_F(TESTNAME, inC)
{
    inValue = 0x39;
    test_io(system, MOV_C_IMM, IN_C, CHANNEL_IN);
    ASSERT_EQ(gp_c->getValue(), 0x39);
}

TEST_F(TESTNAME, inD)
{
    inValue = 0x39;
    test_io(system, MOV_D_IMM, IN_D, CHANNEL_IN);
    ASSERT_EQ(gp_d->getValue(), 0x39);
}

}
