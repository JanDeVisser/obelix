/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME InputOutput
#include "controllertest.h"

namespace Obelix::CPU {

constexpr byte out[] = {
    /* 2000 */ MOV_A_IMM,
    0x42,
    /* 2002 */ OUT_A,
    CHANNEL_OUT,
    /* 2004 */ HLT,
};

class TESTNAME : public HarnessTest {
protected:
    void test_io(byte opcode_init, byte opcode_io, byte channel = CHANNEL_OUT)
    {
        auto mem = system.component<Memory>();
        mem->initialize(RAM_START, 5, out);
        check_memory(RAM_START + 2, OUT_A);
        ASSERT_FALSE(mem->poke(RAM_START, opcode_init).is_error());
        ASSERT_FALSE(mem->poke(RAM_START + 2, opcode_io).is_error());
        ASSERT_FALSE(mem->poke(RAM_START + 3, channel).is_error());

        auto pc = system.component<AddressRegister>(PC);
        pc->setValue(RAM_START);
        ASSERT_EQ(pc->getValue(), RAM_START);

        // mov a, #xx     4 cycles
        // out #xx, a     5 cycles
        // hlt            3 cycles
        // total         12
        auto cycles_or_err = system.run();
        ASSERT_FALSE(cycles_or_err.is_error());
        ASSERT_EQ(cycles_or_err.value(), 12);
        ASSERT_EQ(system.bus().halt(), false);
    }
};

TEST_F(TESTNAME, outA)
{
    outValue = 0x39;
    test_io(MOV_A_IMM, OUT_A);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, outB)
{
    outValue = 0x39;
    test_io(MOV_B_IMM, OUT_B);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, outC)
{
    outValue = 0x39;
    test_io(MOV_C_IMM, OUT_C);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, outD)
{
    outValue = 0x39;
    test_io(MOV_D_IMM, OUT_D);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(TESTNAME, inA)
{
    inValue = 0x39;
    test_io(MOV_A_IMM, IN_A, CHANNEL_IN);
    ASSERT_EQ(gp_a->getValue(), 0x39);
}

TEST_F(TESTNAME, inB)
{
    inValue = 0x39;
    test_io(MOV_B_IMM, IN_B, CHANNEL_IN);
    ASSERT_EQ(gp_b->getValue(), 0x39);
}

TEST_F(TESTNAME, inC)
{
    inValue = 0x39;
    test_io(MOV_C_IMM, IN_C, CHANNEL_IN);
    ASSERT_EQ(gp_c->getValue(), 0x39);
}

TEST_F(TESTNAME, inD)
{
    inValue = 0x39;
    test_io(MOV_D_IMM, IN_D, CHANNEL_IN);
    ASSERT_EQ(gp_d->getValue(), 0x39);
}

}
