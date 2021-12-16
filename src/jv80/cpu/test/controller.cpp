/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME ControllerTest
#include "controllertest.h"

namespace Obelix::JV80::CPU {

byte mov_a_direct[] = {
    /* 8000 */ MOV_A_IMM, 0x42,
    /* 8002 */ HLT
};

TEST_F(TESTNAME, movADirect)
{
    mem->initialize(ROM_START, 3, mov_a_direct);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov a, #42 takes 4 cycles. hlt takes 3.
    ASSERT_EQ(system->run(), 7);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, movADirectUsingRun)
{
    mem->initialize(ROM_START, 3, mov_a_direct);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    ASSERT_EQ(system->run(), 7);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

byte mov_a_absolute[] = {
    /* 8000 */ MOV_A_IMM_IND, 0x04, 0x80,
    /* 8003 */ HLT,
    /* 8004 */ 0x42
};

TEST_F(TESTNAME, movAAbsolute)
{
    mem->initialize(ROM_START, 5, mov_a_absolute);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM_IND);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov a, (8004) takes 8 cycles. hlt takes 3.
    system->cycles(11);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

byte mov_x_a[] = {
    MOV_A_IMM,
    0x42,
    MOV_B_A,
    MOV_C_A,
    MOV_D_A,
    HLT,
};

TEST_F(TESTNAME, movAToOtherGPRs)
{
    mem->initialize(ROM_START, 6, mov_x_a);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov a, #42  4 cycles
    // mov x, a    3 cycles x3
    // hlt         3 cycles
    // Total       16 cycles
    ASSERT_EQ(system->run(), 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_b->getValue(), 0x42);
    ASSERT_EQ(gp_c->getValue(), 0x42);
    ASSERT_EQ(gp_d->getValue(), 0x42);
}

byte mov_x_b[] = {
    MOV_B_IMM,
    0x42,
    MOV_A_B,
    MOV_C_B,
    MOV_D_B,
    HLT,
};

TEST_F(TESTNAME, movBToOtherGPRs)
{
    mem->initialize(ROM_START, 6, mov_x_b);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_B_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    ASSERT_EQ(system->run(), 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_b->getValue(), 0x42);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_c->getValue(), 0x42);
    ASSERT_EQ(gp_d->getValue(), 0x42);
}

byte mov_x_c[] = {
    MOV_C_IMM,
    0x42,
    MOV_A_C,
    MOV_B_C,
    MOV_D_C,
    HLT,
};

TEST_F(TESTNAME, movCToOtherGPRs)
{
    mem->initialize(ROM_START, 6, mov_x_c);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_C_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    ASSERT_EQ(system->run(), 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_c->getValue(), 0x42);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_b->getValue(), 0x42);
    ASSERT_EQ(gp_d->getValue(), 0x42);
}

byte mov_x_d[] = {
    MOV_D_IMM,
    0x42,
    MOV_A_D,
    MOV_B_D,
    MOV_C_D,
    HLT,
};

TEST_F(TESTNAME, movDToOtherGPRs)
{
    mem->initialize(ROM_START, 6, mov_x_d);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_D_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    ASSERT_EQ(system->run(), 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_d->getValue(), 0x42);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_b->getValue(), 0x42);
    ASSERT_EQ(gp_c->getValue(), 0x42);
}

byte mov_x_absolute[] = {
    /* 8000 */ MOV_A_IMM_IND, 0x0D, 0x80,
    /* 8003 */ MOV_B_IMM_IND, 0x0D, 0x80,
    /* 8006 */ MOV_C_IMM_IND, 0x0D, 0x80,
    /* 8009 */ MOV_D_IMM_IND, 0x0D, 0x80,
    /* 800C */ HLT,
    /* 800D */ 0x42
};

TEST_F(TESTNAME, movXAbsolute)
{
    mem->initialize(ROM_START, 14, mov_x_absolute);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM_IND);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov x, (800D) takes 8 cycles x4
    // hlt takes 3.
    ASSERT_EQ(system->run(), 35);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_b->getValue(), 0x42);
    ASSERT_EQ(gp_c->getValue(), 0x42);
    ASSERT_EQ(gp_d->getValue(), 0x42);
}

byte mov_addr_regs_direct[] = {
    /* 8000 */ MOV_SI_IMM,
    0x42,
    0x37,
    /* 8003 */ MOV_DI_IMM,
    0x42,
    0x37,
    /* 8006 */ MOV_SP_IMM,
    0x42,
    0x37,
    /* 8009 */ MOV_CD_IMM,
    0x42,
    0x37,
    /* 800C */ HLT,
};

TEST_F(TESTNAME, movAddrRegsDirect)
{
    mem->initialize(ROM_START, 13, mov_addr_regs_direct);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_SI_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov si, #3742 takes 6 cycles x3.
    // mov cd, #3742 takes 8 cycles.
    // hlt takes 3.
    ASSERT_EQ(system->run(), 29);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(si->getValue(), 0x3742);
    ASSERT_EQ(di->getValue(), 0x3742);
    ASSERT_EQ(sp->getValue(), 0x3742);
    ASSERT_EQ(gp_c->getValue(), 0x42);
    ASSERT_EQ(gp_d->getValue(), 0x37);
}

byte mov_addr_regs_absolute[] = {
    /* 8000 */ MOV_SI_IMM_IND, 0x0A, 0x80,
    /* 8003 */ MOV_DI_IMM_IND, 0x0A, 0x80,
    /* 8006 */ MOV_SP_IMM_IND, 0x0A, 0x80,
    /* 8009 */ HLT,
    /* 800A */ 0x42, 0x37
};

TEST_F(TESTNAME, movAddrRegsAbsolute)
{
    mem->initialize(ROM_START, 12, mov_addr_regs_absolute);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_SI_IMM_IND);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov si, (8004) takes 10 cycles x3
    // hlt takes 3.
    ASSERT_EQ(system->run(), 33);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(si->getValue(), 0x3742);
    ASSERT_EQ(di->getValue(), 0x3742);
    ASSERT_EQ(sp->getValue(), 0x3742);
}

byte mov_addr_regs_from_other_regs[] = {
    /* 8000 */ MOV_C_IMM,
    0x42,
    /* 8002 */ MOV_D_IMM,
    0x37,
    /* 8004 */ MOV_SI_CD,
    /* 8005 */ MOV_DI_CD,
    /* 8006 */ MOV_SP_SI,
    /* 8007 */ HLT,
};

TEST_F(TESTNAME, movAddrRegsFromOtherRegs)
{
    mem->initialize(ROM_START, 8, mov_addr_regs_from_other_regs);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_C_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov c, #xx     4 cycles x2
    // mov xx, cd     4 cycles x2
    // mov sp, si     3 cycles
    // hlt            3 cycles
    // total          22
    ASSERT_EQ(system->run(), 22);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(si->getValue(), 0x3742);
    ASSERT_EQ(di->getValue(), 0x3742);
    ASSERT_EQ(sp->getValue(), 0x3742);
}

byte mov_gp_regs_from_si[] = {
    /* 8000 */ MOV_SI_IMM, 0x08, 0x80,
    /* 8003 */ MOV_A_SI_IND,
    /* 8004 */ MOV_B_SI_IND,
    /* 8005 */ MOV_C_SI_IND,
    /* 8006 */ MOV_D_SI_IND,
    /* 8007 */ HLT,
    /* 8008 */ 0x42,
    /* 8009 */ 0x43,
    /* 800A */ 0x44,
    /* 800B */ 0x45
};

TEST_F(TESTNAME, movGPRegsFromSI)
{
    mem->initialize(ROM_START, 12, mov_gp_regs_from_si);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_SI_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov si, #xxxx  6 cycles
    // mov xx, (si)   4 cycles x4
    // hlt            3 cycles
    // total          17
    ASSERT_EQ(system->run(), 25);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(si->getValue(), 0x800C);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_b->getValue(), 0x43);
    ASSERT_EQ(gp_c->getValue(), 0x44);
    ASSERT_EQ(gp_d->getValue(), 0x45);
}

byte mov_gp_regs_from_di[] = {
    /* 8000 */ MOV_DI_IMM, 0x08, 0x80,
    /* 8003 */ MOV_A_DI_IND,
    /* 8004 */ MOV_B_DI_IND,
    /* 8005 */ MOV_C_DI_IND,
    /* 8006 */ MOV_D_DI_IND,
    /* 8007 */ HLT,
    /* 8008 */ 0x42,
    /* 8009 */ 0x43,
    /* 800A */ 0x44,
    /* 800B */ 0x45
};

TEST_F(TESTNAME, movGPRegsFromDI)
{
    mem->initialize(ROM_START, 12, mov_gp_regs_from_di);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_DI_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov di, #xxxx  6 cycles
    // mov xx, (si)   4 cycles x4
    // hlt            3 cycles
    // total          17
    ASSERT_EQ(system->run(), 25);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(di->getValue(), 0x800C);
    ASSERT_EQ(gp_a->getValue(), 0x42);
    ASSERT_EQ(gp_b->getValue(), 0x43);
    ASSERT_EQ(gp_c->getValue(), 0x44);
    ASSERT_EQ(gp_d->getValue(), 0x45);
}

byte mov_di_from_si[] = {
    /* 8000 */ MOV_SI_IMM, 0x0B, 0x80,
    /* 8003 */ MOV_DI_IMM, 0x00, 0x20,
    /* 8006 */ MOV_DI_IND_SI_IND,
    /* 8007 */ MOV_DI_IND_SI_IND,
    /* 8008 */ MOV_DI_IND_SI_IND,
    /* 8009 */ MOV_DI_IND_SI_IND,
    /* 800A */ HLT,
    /* 800B */ 0x42,
    /* 800C */ 0x43,
    /* 800D */ 0x44,
    /* 800E */ 0x45
};

TEST_F(TESTNAME, movDIFromSI)
{
    mem->initialize(ROM_START, 16, mov_di_from_si);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_SI_IMM);
    ASSERT_EQ((*mem)[0x800B], 0x42);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov si, #xxxx  6 cycles x2
    // mov (di), (si) 6 cycles x4
    // hlt            3 cycles
    // total          39
    ASSERT_EQ(system->run(), 39);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(si->getValue(), 0x800F);
    ASSERT_EQ(di->getValue(), 0x2004);
    ASSERT_EQ((*mem)[0x2000], 0x42);
    ASSERT_EQ((*mem)[0x2001], 0x43);
    ASSERT_EQ((*mem)[0x2002], 0x44);
    ASSERT_EQ((*mem)[0x2003], 0x45);
}

TEST_F(TESTNAME, busFlagManip)
{
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::ProcessorFlags::C);
    system->bus().setFlag(SystemBus::ProcessorFlags::Z);

    ASSERT_TRUE(system->bus().isSet(SystemBus::ProcessorFlags::C));
    ASSERT_TRUE(system->bus().isSet(SystemBus::ProcessorFlags::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::ProcessorFlags::V));

    system->bus().clearFlag(SystemBus::ProcessorFlags::C);

    ASSERT_FALSE(system->bus().isSet(SystemBus::ProcessorFlags::C));
    ASSERT_TRUE(system->bus().isSet(SystemBus::ProcessorFlags::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::ProcessorFlags::V));
}

// MOV_IMM_IND_A      = 0x39,
// MOV_IMM_IND_B      = 0x3B,
// MOV_IMM_IND_C      = 0x3D,
// MOV_IMM_IND_D      = 0x3F,

const byte gp_to_absolute_mem[] = {
    MOV_A_IMM,
    0x42,
    MOV_B_IMM,
    0x43,
    MOV_C_IMM,
    0x44,
    MOV_D_IMM,
    0x45,
    MOV_IMM_IND_A,
    0x00,
    0x20,
    MOV_IMM_IND_B,
    0x01,
    0x20,
    MOV_IMM_IND_C,
    0x02,
    0x20,
    MOV_IMM_IND_D,
    0x03,
    0x20,
    HLT,
};

TEST_F(TESTNAME, movGPRegToMem)
{
    mem->initialize(ROM_START, 21, gp_to_absolute_mem);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov a, #xx     4 cycles x4  16
    // mov (xxxx), a  8 cycles x4  32
    // hlt            3 cycles      3
    // total                       51
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 51);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2000], 0x42);
    ASSERT_EQ((*mem)[0x2001], 0x43);
    ASSERT_EQ((*mem)[0x2002], 0x44);
    ASSERT_EQ((*mem)[0x2003], 0x45);
}

const byte gp_to_rom[] = {
    /* 8000 */ MOV_A_IMM,
    0x42,
    /* 8002 */ MOV_IMM_IND_A,
    0x06,
    0x80, // mov (8006), a
    /* 8005 */ HLT,
};

TEST_F(TESTNAME, cantMovGPRegToROM)
{
    mem->initialize(ROM_START, 21, gp_to_rom);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    ASSERT_NE((*mem)[0x8006], 0x42);
    system->run();
    ASSERT_EQ(system->error(), ProtectedMemory);
    ASSERT_NE((*mem)[0x8006], 0x42);
}

const byte gp_to_unmapped[] = {
    /* 8000 */ MOV_A_IMM,
    0x42,
    /* 8002 */ MOV_IMM_IND_A,
    0x06,
    0x10, // mov (1006), a
    /* 8005 */ HLT,
};

TEST_F(TESTNAME, cantMovGPRegToUnmappedMem)
{
    mem->initialize(ROM_START, 21, gp_to_unmapped);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    system->run();
    ASSERT_EQ(system->error(), ProtectedMemory);
}

const byte gp_to_di_indirect[] = {
    MOV_A_IMM,
    0x42,
    MOV_B_IMM,
    0x43,
    MOV_C_IMM,
    0x44,
    MOV_D_IMM,
    0x45,
    MOV_DI_IMM,
    0x00,
    0x20,
    MOV_DI_IND_A,
    MOV_DI_IND_B,
    MOV_DI_IND_C,
    MOV_DI_IND_D,
    HLT,
};

TEST_F(TESTNAME, movGPRegToDiIndirect)
{
    mem->initialize(ROM_START, 16, gp_to_di_indirect);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov a, #xx     4 cycles x4  16
    // mov di, #xxxx  6 cycles      6
    // mov (di), a    4 cycles x4  16
    // hlt            3 cycles      3
    // total                       41
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 41);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2000], 0x42);
    ASSERT_EQ((*mem)[0x2001], 0x43);
    ASSERT_EQ((*mem)[0x2002], 0x44);
    ASSERT_EQ((*mem)[0x2003], 0x45);
}

const byte addr_reg_to_absolute_mem[] = {
    MOV_SI_IMM,
    0x22,
    0x11,
    MOV_DI_IMM,
    0x44,
    0x33,
    MOV_C_IMM,
    0x66,
    MOV_D_IMM,
    0x55,
    MOV_IMM_IND_SI,
    0x00,
    0x20,
    MOV_IMM_IND_DI,
    0x02,
    0x20,
    MOV_IMM_IND_CD,
    0x04,
    0x20,
    HLT,
};

TEST_F(TESTNAME, movAddrRegToMem)
{
    mem->initialize(ROM_START, 21, addr_reg_to_absolute_mem);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_SI_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov si, #xxxx   6 cycles x2  12
    // mov c, #xx      4        x2   8
    // mov (xxxx), si 10        x3  30
    // hlt             3             3
    // total                        53
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 53);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2000], 0x22);
    ASSERT_EQ((*mem)[0x2001], 0x11);
    ASSERT_EQ((*mem)[0x2002], 0x44);
    ASSERT_EQ((*mem)[0x2003], 0x33);
    ASSERT_EQ((*mem)[0x2004], 0x66);
    ASSERT_EQ((*mem)[0x2005], 0x55);
}

const byte cd_to_sidi_indirect[] = {
    MOV_SI_IMM,
    0x00,
    0x20,
    MOV_DI_IMM,
    0x10,
    0x20,
    MOV_C_IMM,
    0x42,
    MOV_D_IMM,
    0x37,
    MOV_SI_IND_CD,
    MOV_DI_IND_CD,
    HLT,
};

TEST_F(TESTNAME, movCDRegToMemViaSiDiIndirect)
{
    mem->initialize(ROM_START, 21, cd_to_sidi_indirect);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_SI_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov si, #xxxx   6 cycles x2  12
    // mov c, #xx      4        x2   8
    // mov (si), cd    6        x2  12
    // hlt             3             3
    // total                        35
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 35);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2000], 0x42);
    ASSERT_EQ((*mem)[0x2001], 0x37);
    ASSERT_EQ((*mem)[0x2010], 0x42);
    ASSERT_EQ((*mem)[0x2011], 0x37);
}

const byte a_to_cd_indirect[] = {
    MOV_A_IMM,
    0x42,
    MOV_C_IMM,
    0x10,
    MOV_D_IMM,
    0x20,
    MOV_CD_IND_A,
    HLT,
};

TEST_F(TESTNAME, movARegToMemViaCDIndirect)
{
    mem->initialize(ROM_START, 8, a_to_cd_indirect);
    ASSERT_EQ((*mem)[START_VECTOR], MOV_A_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    // mov a, #xx      4        x3  12
    // mov *cd, a      5             5
    // hlt             3             3
    // total                        20
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 20);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2010], 0x42);
}

TEST_F(TESTNAME, movBRegToMemViaCDIndirect)
{
    mem->initialize(RAM_START, 8, a_to_cd_indirect);
    ASSERT_EQ((*mem)[RAM_VECTOR], MOV_A_IMM);
    (*mem)[RAM_VECTOR + 0] = MOV_B_IMM;
    (*mem)[RAM_VECTOR + 6] = MOV_CD_IND_B;

    pc->setValue(RAM_VECTOR);
    ASSERT_EQ(pc->getValue(), RAM_VECTOR);

    // mov a, #xx      4        x3  12
    // mov *cd, a      5             5
    // hlt             3             3
    // total                        20
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 20);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2010], 0x42);
}

const byte cd_indirect_to_a[] = {
    MOV_C_IMM,
    0x06,
    MOV_D_IMM,
    0x20,
    MOV_A_CD_IND,
    HLT,
    0x42,
};

TEST_F(TESTNAME, movMemToARegViaCDIndirect)
{
    mem->initialize(RAM_START, 7, cd_indirect_to_a);
    ASSERT_EQ((*mem)[RAM_VECTOR], MOV_C_IMM);

    pc->setValue(RAM_VECTOR);
    ASSERT_EQ(pc->getValue(), RAM_VECTOR);

    // mov c, #xx      4        x2   8
    // mov a, *cd      5             5
    // hlt             3             3
    // total                        16
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_a->getValue(), 0x42);
}

TEST_F(TESTNAME, movMemToBRegViaCDIndirect)
{
    mem->initialize(RAM_START, 7, cd_indirect_to_a);
    ASSERT_EQ((*mem)[RAM_VECTOR], MOV_C_IMM);
    (*mem)[RAM_VECTOR + 4] = MOV_B_CD_IND;

    pc->setValue(RAM_VECTOR);
    ASSERT_EQ(pc->getValue(), RAM_VECTOR);

    // mov c, #xx      4        x2   8
    // mov a, *cd      5             5
    // hlt             3             3
    // total                        16
    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ(gp_b->getValue(), 0x42);
}

const byte mov_const_to_addr_reg_indirect[] = {
    /* 2000 */ MOV_SI_IMM,
    0x06,
    0x20,
    /* 2003 */ MOV_SI_IND_IMM,
    0x42,
    /* 2005 */ HLT,
    /* 2006 */ 0x37,
};

TEST_F(TESTNAME, movConstToSiIndirect)
{
    mem->initialize(RAM_START, 7, mov_const_to_addr_reg_indirect);
    ASSERT_EQ((*mem)[RAM_VECTOR], MOV_SI_IMM);

    pc->setValue(RAM_VECTOR);
    ASSERT_EQ(pc->getValue(), RAM_VECTOR);

    [[maybe_unused]] auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    //  ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2006], 0x42);
}

TEST_F(TESTNAME, movConstToDiIndirect)
{
    mem->initialize(RAM_START, 7, mov_const_to_addr_reg_indirect);
    ASSERT_EQ((*mem)[RAM_VECTOR], MOV_SI_IMM);
    (*mem)[RAM_VECTOR] = MOV_DI_IMM;
    (*mem)[RAM_VECTOR + 3] = MOV_DI_IND_IMM;

    pc->setValue(RAM_VECTOR);
    ASSERT_EQ(pc->getValue(), RAM_VECTOR);

    [[maybe_unused]] auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    //  ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2006], 0x42);
}

const byte mov_const_to_cd_indirect[] = {
    /* 2000 */ MOV_C_IMM,
    0x07,
    /* 2002 */ MOV_D_IMM,
    0x20,
    /* 2004 */ MOV_CD_IND_IMM,
    0x42,
    /* 2006 */ HLT,
    /* 2007 */ 0x37,
};

TEST_F(TESTNAME, movConstToCDIndirect)
{
    mem->initialize(RAM_START, 8, mov_const_to_cd_indirect);
    ASSERT_EQ((*mem)[RAM_VECTOR], MOV_C_IMM);

    pc->setValue(RAM_VECTOR);
    ASSERT_EQ(pc->getValue(), RAM_VECTOR);

    [[maybe_unused]] auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    //  ASSERT_EQ(cycles, 16);
    ASSERT_EQ(system->bus().halt(), false);
    ASSERT_EQ((*mem)[0x2007], 0x42);
}

}
