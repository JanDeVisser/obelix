/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Indexed
#include "controllertest.h"

namespace Obelix::JV80::CPU {

class TESTNAME : public HarnessTest {
public:
    static constexpr byte mov_gp_si_idx_asm[] = {
        /* 8000 */ MOV_SI_IMM, 0x00, 0x20, // 6
        /* 0003 */ MOV_A_SI_IDX, 0x10,     //
        /* 8005 */ HLT                     // 3
    };

    void mov_gp_si_idx(byte opcode, std::shared_ptr<Register> const& reg)
    {
        mem->initialize(RAM_START, 6, mov_gp_si_idx_asm);
        check_memory(RAM_VECTOR, MOV_SI_IMM);
        ASSERT_FALSE(mem->poke(0x2003, opcode).is_error());
        ASSERT_FALSE(mem->poke(0x2010, 0xFE).is_error());
        ASSERT_FALSE(mem->poke(0x2011, 0xCA).is_error());

        pc->setValue(RAM_VECTOR);
        ASSERT_EQ(pc->getValue(), RAM_VECTOR);

        check_cycles(15);
        ASSERT_EQ(reg->getValue(), 0xFE);
    }

    static constexpr byte mov_si_idx_gp_asm[] = {
        /* 8000 */ MOV_SI_IMM, 0x00, 0x20, // 6
        /* 8003 */ MOV_A_IMM, 0x42,
        /* 0005 */ MOV_SI_IDX_A, 0x10, //
        /* 8007 */ HLT                 // 3
    };

    void mov_si_idx_gp(byte load_gp, byte idx_opcode)
    {
        mem->initialize(RAM_START, 8, mov_si_idx_gp_asm);
        check_memory(RAM_VECTOR, MOV_SI_IMM);
        ASSERT_FALSE(mem->poke(0x2003, load_gp).is_error());
        ASSERT_FALSE(mem->poke(0x2005, idx_opcode).is_error());

        pc->setValue(RAM_VECTOR);
        ASSERT_EQ(pc->getValue(), RAM_VECTOR);

        check_cycles(19);
        auto peek = mem->peek(0x2010);
        ASSERT_FALSE(peek.is_error());
        ASSERT_EQ(peek.value(), 0x42);
    }
};

byte mov_bp_sp[] = {
    /* 8000 */ MOV_SP_IMM, 0x42, 0x55, // 6
    /* 0002 */ MOV_BP_SP,              // 3
    /* 8003 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_BP_SP)
{
    mem->initialize(ROM_START, 5, mov_bp_sp);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(12);
    ASSERT_EQ(bp->getValue(), 0x5542);
}

byte mov_sp_bp[] = {
    /* 8000 */ MOV_SP_IMM, 0x42, 0x55, // 6
    /* 0003 */ MOV_BP_SP,              // 3
    /* 8004 */ MOV_SP_IMM, 0xFE, 0xCA, // 6
    /* 0007 */ MOV_SP_BP,              // 3
    /* 8008 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_SP_BP)
{
    mem->initialize(ROM_START, 9, mov_sp_bp);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(21);
    ASSERT_EQ(bp->getValue(), 0x5542);
}

byte mov_si_bp_idx[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
    /* 0003 */ MOV_BP_SP,              // 3
    /* 0004 */ MOV_SI_BP_IDX, 0x02,    //
    /* 8006 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_SI_BP_IDX)
{
    mem->initialize(ROM_START, 7, mov_si_bp_idx);
    check_memory(START_VECTOR, MOV_SP_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(19);
    ASSERT_EQ(si->getValue(), 0xCAFE);
}

byte mov_si_bp_idx_negative_index[] = {
    /* 8000 */ MOV_SP_IMM, 0x04, 0x20,  // 6
    /* 0003 */ MOV_BP_SP,               // 3
    /* 0004 */ MOV_SI_BP_IDX, (byte)-2, //
    /* 8006 */ HLT                      // 3
};

TEST_F(TESTNAME, MOV_SI_BP_IDX_negative_index)
{
    mem->initialize(ROM_START, 7, mov_si_bp_idx_negative_index);
    check_memory(START_VECTOR, MOV_SP_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(19);
    ASSERT_EQ(si->getValue(), 0xCAFE);
}

byte mov_di_bp_idx[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
    /* 0003 */ MOV_BP_SP,              // 3
    /* 0004 */ MOV_DI_BP_IDX, 0x02,    //
    /* 8006 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_DI_BP_IDX)
{
    mem->initialize(ROM_START, 7, mov_di_bp_idx);
    check_memory(START_VECTOR, MOV_SP_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(19);
    ASSERT_EQ(di->getValue(), 0xCAFE);
}

byte mov_di_si_idx[] = {
    /* 8000 */ MOV_SI_IMM, 0x00, 0x20, // 6
    /* 0003 */ MOV_DI_SI_IDX, 0x02,    //
    /* 8005 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_DI_SI_IDX)
{
    mem->initialize(ROM_START, 6, mov_di_si_idx);
    check_memory(START_VECTOR, MOV_SI_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(16);
    ASSERT_EQ(di->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, MOV_A_SI_IDX)
{
    mov_gp_si_idx(MOV_A_SI_IDX, gp_a);
}

TEST_F(TESTNAME, MOV_B_SI_IDX)
{
    mov_gp_si_idx(MOV_B_SI_IDX, gp_b);
}

TEST_F(TESTNAME, MOV_C_SI_IDX)
{
    mov_gp_si_idx(MOV_C_SI_IDX, gp_c);
}

TEST_F(TESTNAME, MOV_D_SI_IDX)
{
    mov_gp_si_idx(MOV_D_SI_IDX, gp_d);
}

byte mov_bp_idx_si[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
    /* 0003 */ MOV_BP_SP,              // 3
    /* 8000 */ MOV_SI_IMM, 0xFE, 0xCA, // 6
    /* 0004 */ MOV_BP_IDX_SI, 0x02,    //
    /* 8006 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_BP_IDX_SI)
{
    mem->initialize(ROM_START, 10, mov_bp_idx_si);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(25);
    ASSERT_EQ(mem->peek(0x2002).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2003).value(), 0xCA);
}

byte mov_bp_idx_di[] = {
    /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
    /* 0003 */ MOV_BP_SP,              // 3
    /* 8000 */ MOV_DI_IMM, 0xFE, 0xCA, // 6
    /* 0004 */ MOV_BP_IDX_DI, 0x02,    //
    /* 8006 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_BP_IDX_DI)
{
    mem->initialize(ROM_START, 10, mov_bp_idx_di);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(25);
    ASSERT_EQ(mem->peek(0x2002).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2003).value(), 0xCA);
}

byte mov_si_idx_di[] = {
    /* 8000 */ MOV_SI_IMM, 0x00, 0x20, // 6
    /* 8000 */ MOV_DI_IMM, 0xFE, 0xCA, // 6
    /* 0003 */ MOV_SI_IDX_DI, 0x02,    //
    /* 8005 */ HLT                     // 3
};

TEST_F(TESTNAME, MOV_SI_IDX_DI)
{
    mem->initialize(ROM_START, 9, mov_si_idx_di);
    check_memory(START_VECTOR, MOV_SI_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(22);
    ASSERT_EQ(mem->peek(0x2002).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2003).value(), 0xCA);
}

TEST_F(TESTNAME, MOV_SI_IDX_A)
{
    mov_si_idx_gp(MOV_A_IMM, MOV_SI_IDX_A);
}

TEST_F(TESTNAME, MOV_SI_IDX_B)
{
    mov_si_idx_gp(MOV_B_IMM, MOV_SI_IDX_B);
}

TEST_F(TESTNAME, MOV_SI_IDX_C)
{
    mov_si_idx_gp(MOV_C_IMM, MOV_SI_IDX_C);
}

TEST_F(TESTNAME, MOV_SI_IDX_D)
{
    mov_si_idx_gp(MOV_D_IMM, MOV_SI_IDX_D);
}

}
