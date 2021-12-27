/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Jump
#include "controllertest.h"

namespace Obelix::JV80::CPU {

const byte jmp_immediate[] = {
    /* 2000 */ JMP,
    0x06,
    0x20,
    /* 2003 */ MOV_A_IMM,
    0x37,
    /* 2005 */ HLT,
    /* 2006 */ MOV_A_IMM,
    0x42,
    /* 2008 */ HLT,
};

const byte jmp_ind[] = {
    /* 2000 */ JMP_IND, 0x09, 0x20,
    /* 2003 */ MOV_A_IMM, 0x37,
    /* 2005 */ HLT,
    /* 2006 */ MOV_A_IMM, 0x42,
    /* 2008 */ HLT,
    /* 2009 */ 0x06, 0x20
};

class TESTNAME : public HarnessTest {
protected:
    void test_jump_immediate(byte opcode, bool ok)
    {
        auto mem = system.component<Memory>();
        mem->initialize(RAM_START, 9, jmp_immediate);
        check_memory(RAM_START, JMP);
        ASSERT_FALSE(mem->poke(RAM_START, opcode).is_error());

        auto pc = system.component<AddressRegister>(PC);
        pc->setValue(RAM_START);
        ASSERT_EQ(pc->getValue(), RAM_START);

        // jmp            7 cycles
        // mov a, #xx     4 cycles
        // hlt            3 cycles
        // total         14
        auto cycles_or_err = system.run();
        ASSERT_FALSE(cycles_or_err.is_error());
        ASSERT_EQ(cycles_or_err.value(), (ok) ? 14 : 13);
        ASSERT_EQ(system.bus().halt(), false);
    }

    void test_jump_ind(byte opcode, bool ok)
    {
        auto mem = system.component<Memory>();
        mem->initialize(RAM_START, 11, jmp_ind);
        check_memory(RAM_START, JMP_IND);
        ASSERT_FALSE(mem->poke(RAM_START, opcode).is_error());

        auto pc = system.component<AddressRegister>(PC);
        pc->setValue(RAM_START);
        ASSERT_EQ(pc->getValue(), RAM_START);

        // jmp_ind        6/10 cycles
        // mov a, #xx     4 cycles
        // hlt            3 cycles
        // total         14
        auto cycles_or_err = system.run();
        ASSERT_FALSE(cycles_or_err.is_error());
        ASSERT_EQ(cycles_or_err.value(), (ok) ? 16 : 13);
        ASSERT_EQ(system.bus().halt(), false);
    }
};

TEST_F(TESTNAME, jmp)
{
    test_jump_immediate(JMP, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jcCarrySet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::C);
    test_jump_immediate(JC, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jcCarryNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::C);
    test_jump_immediate(JC, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

TEST_F(TESTNAME, jnzZeroNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::Z);
    test_jump_immediate(JNZ, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jnzZeroSet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::Z);
    test_jump_immediate(JNZ, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

TEST_F(TESTNAME, jvCarrySet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::V);
    test_jump_immediate(JV, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jvOverflowNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::V);
    test_jump_immediate(JV, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

TEST_F(TESTNAME, jzZeroSet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::Z);
    test_jump_immediate(JZ, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jzZeroNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::Z);
    test_jump_immediate(JZ, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

const byte asm_call[] = {
    /* 8000 */ MOV_A_IMM, 0x37,  //  4 cycles
    /* 8002 */ CALL, 0x06, 0x80, // 11
    /* 8005 */ HLT,              //  3
    /* 8006 */ MOV_A_IMM, 0x42,  //  4
    /* 8008 */ RET,              //  6
};                                // Total: 28

TEST_F(TESTNAME, call)
{
    mem->initialize(ROM_START, 9, asm_call);
    check_memory(START_VECTOR, MOV_A_IMM);

    sp->setValue(RAM_START);
    ASSERT_EQ(sp->getValue(), RAM_START);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(28);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jmp_ind)
{
    test_jump_ind(JMP_IND, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jcAbsCarrySet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::C);
    test_jump_ind(JC_IND, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jcAbsCarryNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::C);
    test_jump_ind(JC_IND, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

TEST_F(TESTNAME, jnzAbsZeroNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::Z);
    test_jump_ind(JNZ_IND, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jnzAbsZeroSet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::Z);
    test_jump_ind(JNZ_IND, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

TEST_F(TESTNAME, jvAbsCarrySet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::V);
    test_jump_ind(JV_IND, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jvAbsOverflowNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::V);
    test_jump_ind(JV_IND, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

TEST_F(TESTNAME, jzAbsZeroSet)
{
    system.bus().setFlag(SystemBus::ProcessorFlags::Z);
    test_jump_ind(JZ_IND, true);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, jzAbsZeroNotSet)
{
    system.bus().clearFlag(SystemBus::ProcessorFlags::Z);
    test_jump_ind(JZ_IND, false);
    ASSERT_EQ(gp_a->getValue(), 0x37);
}

const byte asm_call_ind[] = {
    /* 8000 */ MOV_A_IMM, 0x37,      //  4 cycles
    /* 8002 */ CALL_IND, 0x09, 0x80, // 14
    /* 8005 */ HLT,                  //  3
    /* 8006 */ MOV_A_IMM, 0x42,      //  4
    /* 8008 */ RET,                  //  6
    /* 8009 */ 0x06, 0x80
}; // Total: 28

TEST_F(TESTNAME, call_ind)
{
    mem->initialize(ROM_START, 11, asm_call_ind);
    check_memory(START_VECTOR, MOV_A_IMM);

    sp->setValue(RAM_START);
    ASSERT_EQ(sp->getValue(), RAM_START);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(31);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

const byte asm_nmi[] = {
    /* 8000 */ NMIVEC, 0x13, 0x80,     //  4 cycles
    /* 8003 */ MOV_A_IMM, 0x30,        //  4 cycles
    /* 8005 */ MOV_B_IMM, 0x31,        //  4 cycles
    /* 8007 */ MOV_C_IMM, 0x32,        //  4 cycles
    /* 8009 */ MOV_D_IMM, 0x33,        //  4 cycles
    /* 800B */ MOV_SI_IMM, 0x34, 0x35, //  6 cycles
    /* 800E */ MOV_DI_IMM, 0x36, 0x37, //  6 cycles
    /* 8011 */ NOP,                    //  3
                                       // CALL NMI                         //  9
    /* 8012 */ HLT,                    //  6
    /* 8013 */ NOP,                    //  (NMI Service) 3 cycles
    /* 8014 */ RTI                     // 10
};                                     // Total: 58

TEST_F(TESTNAME, nmi)
{
    mem->initialize(ROM_START, 21, asm_nmi);
    check_memory(START_VECTOR, NMIVEC);

    sp->setValue(RAM_START);
    ASSERT_EQ(sp->getValue(), RAM_START);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    nmiAt = 0x8011;
    check_cycles(58);
    ASSERT_EQ(gp_a->getValue(), 0x30);
    ASSERT_EQ(gp_b->getValue(), 0x31);
    ASSERT_EQ(gp_c->getValue(), 0x32);
    ASSERT_EQ(gp_d->getValue(), 0x33);
    ASSERT_EQ(si->getValue(), 0x3534);
    ASSERT_EQ(di->getValue(), 0x3736);
}

}
