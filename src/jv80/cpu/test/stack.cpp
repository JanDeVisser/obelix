/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Stack
#include "controllertest.h"

namespace Obelix::JV80::CPU {

class TESTNAME : public HarnessTest {
};

const byte push_a[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20,
    /* 8003 */ MOV_A_IMM, 0x42,
    /* 8005 */ PUSH_A,
    /* 8006 */ HLT
};

TEST_F(TESTNAME, pushA)
{
    mem->initialize(ROM_START, 7, push_a);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov sp, #xxxx  6 cycles
    // mov a, #xx     4 cycles
    // push a         4 cycles
    // hlt            3 cycles
    // total          17
    check_cycles(17);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(sp->getValue(), 0x2001);
    check_memory(0x2000, 0x42);
}

const byte push_pop_a[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20,
    /* 8003 */ MOV_A_IMM, 0x42,
    /* 8005 */ PUSH_A,
    /* 8006 */ MOV_A_IMM, 0x37,
    /* 8008 */ POP_A,
    /* 8009 */ HLT
};

TEST_F(TESTNAME, pushPopA)
{
    mem->initialize(ROM_START, 10, push_pop_a);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov sp, #xxxx  6 cycles
    // mov a, #xx     4 cycles
    // push a         4 cycles
    // mov a, #xx     4 cycles
    // pop a          4 cycles
    // hlt            3 cycles
    // total          25
    check_cycles(25);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(sp->getValue(), 0x2000);
    check_memory(0x2000, 0x42);
}

const byte push_pop_gp_regs[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20,
    /* 8003 */ MOV_A_IMM, 0x42,
    /* 8005 */ MOV_B_IMM, 0x43,
    /* 8007 */ MOV_C_IMM, 0x44,
    /* 8009 */ MOV_D_IMM, 0x45,
    /* 800B */ PUSH_A,
    /* 800C */ PUSH_B,
    /* 800D */ PUSH_C,
    /* 800E */ PUSH_D,
    /* 800F */ MOV_A_IMM, 0x37,
    /* 8011 */ MOV_B_IMM, 0x36,
    /* 8013 */ MOV_C_IMM, 0x35,
    /* 8015 */ MOV_D_IMM, 0x34,
    /* 8017 */ POP_D,
    /* 8018 */ POP_C,
    /* 8019 */ POP_B,
    /* 801A */ POP_A,
    /* 801B */ HLT
};

TEST_F(TESTNAME, pushPopGPRegs)
{
    mem->initialize(ROM_START, 29, push_pop_gp_regs);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov sp, #xxxx  6 cycles      6
    // mov x, #xx     4 cycles 4x  16
    // push a         4 cycles 4x  16
    // mov a, #xx     4 cycles 4x  16
    // pop a          4 cycles 4x  16
    // hlt            3 cycles      3
    // total                       73
    check_cycles(73);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_c->getValue(), 0x44);
    ASSERT_EQ(gp_d->getValue(), 0x45);
    ASSERT_EQ(sp->getValue(), 0x2000);
    check_memory(0x2000, 0x42);
    check_memory(0x2001, 0x43);
    check_memory(0x2002, 0x44);
    check_memory(0x2003, 0x45);
}

const byte push_pop_addr_regs[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20,
    /* 8003 */ MOV_BP_SP,
    /* 8004 */ MOV_SI_IMM, 0x34, 0x12,
    /* 8007 */ MOV_DI_IMM, 0x78, 0x56,
    /* 800A */ PUSH_SI,
    /* 800B */ PUSH_DI,
    /* 800C */ PUSH_BP,
    /* 800D */ MOV_SI_IMM, 0x55, 0x44,
    /* 8010 */ MOV_DI_IMM, 0x77, 0x66,
    /* 8013 */ MOV_BP_SP,
    /* 8014 */ POP_BP,
    /* 8015 */ POP_DI,
    /* 8016 */ POP_SI,
    /* 8017 */ HLT
};

TEST_F(TESTNAME, pushPopAddrRegs)
{
    mem->initialize(ROM_START, 24, push_pop_addr_regs);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(75);
    ASSERT_EQ(si->getValue(), 0x1234);
    ASSERT_EQ(di->getValue(), 0x5678);
    ASSERT_EQ(bp->getValue(), 0x2000);
    ASSERT_EQ(sp->getValue(), 0x2000);
    check_memory(0x2000, 0x34);
    check_memory(0x2001, 0x12);
    check_memory(0x2002, 0x78);
    check_memory(0x2003, 0x56);
    check_memory(0x2004, 0x00);
    check_memory(0x2005, 0x20);
}

TEST_F(TESTNAME, PUSH_IMM)
{
    static constexpr byte code[] = {
        MOV_SP_IMM, 0x00, 0x20,
        PUSH_IMM, 0x42,
        POP_A,
        HLT
    };
    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, code[0]);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(19);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, PUSHW_IMM)
{
    static constexpr byte code[] = {
        MOV_SP_IMM, 0x00, 0x20,
        PUSHW_IMM, 0xFE, 0xCA,
        POP_SI,
        HLT
    };
    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, code[0]);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(25);
    ASSERT_EQ(si->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, PUSH_AB)
{
    static constexpr byte code[] = {
        MOV_SP_IMM, 0x00, 0x20,
        MOV_A_IMM, 0xFE,
        MOV_B_IMM, 0xCA,
        PUSH_AB,
        POP_SI,
        HLT
    };
    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, code[0]);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(29);
    ASSERT_EQ(si->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, PUSH_CD)
{
    static constexpr byte code[] = {
        MOV_SP_IMM, 0x00, 0x20,
        MOV_C_IMM, 0xFE,
        MOV_D_IMM, 0xCA,
        PUSH_CD,
        POP_SI,
        HLT
    };
    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, code[0]);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(29);
    ASSERT_EQ(si->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, POP_AB)
{
    static constexpr byte code[] = {
        MOV_SP_IMM, 0x00, 0x20,
        MOV_SI_IMM, 0xFE, 0xCA,
        PUSH_SI,
        POP_AB,
        HLT
    };
    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, code[0]);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(27);
    ASSERT_EQ(gp_a->getValue(), 0xFE);
    ASSERT_EQ(gp_b->getValue(), 0xCA);
}

TEST_F(TESTNAME, POP_CD)
{
    static constexpr byte code[] = {
        MOV_SP_IMM, 0x00, 0x20,
        MOV_SI_IMM, 0xFE, 0xCA,
        PUSH_SI,
        POP_CD,
        HLT
    };
    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, code[0]);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(27);
    ASSERT_EQ(gp_c->getValue(), 0xFE);
    ASSERT_EQ(gp_d->getValue(), 0xCA);
}

}
