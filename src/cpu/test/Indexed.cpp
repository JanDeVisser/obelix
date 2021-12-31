/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Indexed
#include "controllertest.h"

namespace Obelix::CPU {

class TESTNAME : public HarnessTest {
public:
    void mov_gp_bp_idx(byte opcode, std::shared_ptr<Register> const& reg)
    {
        static constexpr byte code[] = {
            MOV_SP_IMM, 0x00, 0x20,
            MOV_BP_SP,
            MOV_A_BP_IDX, 0x10,
            HLT
        };

        mem->initialize(RAM_START, 7, code);
        check_memory(RAM_VECTOR, MOV_SP_IMM);
        ASSERT_FALSE(mem->poke(0x2004, opcode).is_error());
        ASSERT_FALSE(mem->poke(0x2010, 0xFE).is_error());
        ASSERT_FALSE(mem->poke(0x2011, 0xCA).is_error());

        pc->setValue(RAM_VECTOR);
        ASSERT_EQ(pc->getValue(), RAM_VECTOR);

        check_cycles(19);
        ASSERT_EQ(reg->getValue(), 0xFE);
    }

    void mov_bp_idx_gp(byte load_gp, byte idx_opcode)
    {
        static constexpr byte code[] = {
            MOV_SP_IMM, 0x00, 0x20,
            MOV_BP_SP,
            MOV_A_IMM, 0x42,
            MOV_BP_IDX_A, 0x10,
            HLT
        };

        mem->initialize(RAM_START, 9, code);
        check_memory(RAM_VECTOR, MOV_SP_IMM);
        ASSERT_FALSE(mem->poke(0x2004, load_gp).is_error());
        ASSERT_FALSE(mem->poke(0x2006, idx_opcode).is_error());

        pc->setValue(RAM_VECTOR);
        ASSERT_EQ(pc->getValue(), RAM_VECTOR);

        check_cycles(23);
        auto peek = mem->peek(0x2010);
        ASSERT_FALSE(peek.is_error());
        ASSERT_EQ(peek.value(), 0x42);
    }
};

TEST_F(TESTNAME, MOV_BP_SP)
{
    constexpr static byte mov_bp_sp[] = {
        /* 8000 */ MOV_SP_IMM, 0x42, 0x55, // 6
        /* 0002 */ MOV_BP_SP,              // 3
        /* 8003 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 5, mov_bp_sp);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(12);
    ASSERT_EQ(bp->getValue(), 0x5542);
}

TEST_F(TESTNAME, MOV_SP_BP)
{
    constexpr static byte mov_sp_bp[] = {
        /* 8000 */ MOV_SP_IMM, 0x42, 0x55, // 6
        /* 0003 */ MOV_BP_SP,              // 3
        /* 8004 */ MOV_SP_IMM, 0xFE, 0xCA, // 6
        /* 0007 */ MOV_SP_BP,              // 3
        /* 8008 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 9, mov_sp_bp);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(21);
    ASSERT_EQ(bp->getValue(), 0x5542);
}

TEST_F(TESTNAME, MOV_SI_BP_IDX)
{
    constexpr static byte mov_si_bp_idx[] = {
        /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
        /* 0003 */ MOV_BP_SP,              // 3
        /* 0004 */ MOV_SI_BP_IDX, 0x02,    //
        /* 8006 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 7, mov_si_bp_idx);
    check_memory(START_VECTOR, MOV_SP_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(20);
    ASSERT_EQ(si->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, MOV_SI_BP_IDX_negative_index)
{
    constexpr static byte code[] = {
        /* 8000 */ MOV_SP_IMM, 0x04, 0x20,  // 6
        /* 0003 */ MOV_BP_SP,               // 3
        /* 0004 */ MOV_SI_BP_IDX, (byte)-2, //
        /* 8006 */ HLT                      // 3
    };

    mem->initialize(ROM_START, 7, code);
    check_memory(START_VECTOR, MOV_SP_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(20);
    ASSERT_EQ(si->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, MOV_DI_BP_IDX)
{
    constexpr static byte code[] = {
        /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
        /* 0003 */ MOV_BP_SP,              // 3
        /* 0004 */ MOV_DI_BP_IDX, 0x02,    //
        /* 8006 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 7, code);
    check_memory(START_VECTOR, MOV_SP_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(20);
    ASSERT_EQ(di->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, MOV_DI_SI_IDX)
{
    constexpr static byte code[] = {
        /* 8000 */ MOV_SI_IMM, 0x00, 0x20, // 6
        /* 0003 */ MOV_DI_SI_IDX, 0x02,    //
        /* 8005 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 6, code);
    check_memory(START_VECTOR, MOV_SI_IMM);
    ASSERT_FALSE(mem->poke(0x2002, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2003, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(17);
    ASSERT_EQ(di->getValue(), 0xCAFE);
}

TEST_F(TESTNAME, MOV_A_BP_IDX)
{
    mov_gp_bp_idx(MOV_A_BP_IDX, gp_a);
}

TEST_F(TESTNAME, MOV_B_BP_IDX)
{
    mov_gp_bp_idx(MOV_B_BP_IDX, gp_b);
}

TEST_F(TESTNAME, MOV_C_BP_IDX)
{
    mov_gp_bp_idx(MOV_C_BP_IDX, gp_c);
}

TEST_F(TESTNAME, MOV_D_BP_IDX)
{
    mov_gp_bp_idx(MOV_D_BP_IDX, gp_d);
}

TEST_F(TESTNAME, MOV_BP_IDX_SI)
{
    constexpr static byte code[] = {
        /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
        /* 0003 */ MOV_BP_SP,              // 3
        /* 8000 */ MOV_SI_IMM, 0xFE, 0xCA, // 6
        /* 0004 */ MOV_BP_IDX_SI, 0x02,    //
        /* 8006 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 10, code);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(26);
    ASSERT_EQ(mem->peek(0x2002).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2003).value(), 0xCA);
}

TEST_F(TESTNAME, MOV_BP_IDX_DI)
{
    constexpr static byte code[] = {
        /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
        /* 0003 */ MOV_BP_SP,              // 3
        /* 8000 */ MOV_DI_IMM, 0xFE, 0xCA, // 6
        /* 0004 */ MOV_BP_IDX_DI, 0x02,    //
        /* 8006 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 10, code);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(26);
    ASSERT_EQ(mem->peek(0x2002).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2003).value(), 0xCA);
}

TEST_F(TESTNAME, MOV_SI_IDX_DI)
{
    constexpr static byte code[] = {
        /* 8000 */ MOV_SI_IMM, 0x00, 0x20, // 6
        /* 8000 */ MOV_DI_IMM, 0xFE, 0xCA, // 6
        /* 0003 */ MOV_SI_IDX_DI, 0x02,    //
        /* 8005 */ HLT                     // 3
    };

    mem->initialize(ROM_START, 9, code);
    check_memory(START_VECTOR, MOV_SI_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(23);
    ASSERT_EQ(mem->peek(0x2002).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2003).value(), 0xCA);
}

TEST_F(TESTNAME, MOV_BP_IDX_A)
{
    mov_bp_idx_gp(MOV_A_IMM, MOV_BP_IDX_A);
}

TEST_F(TESTNAME, MOV_BP_IDX_B)
{
    mov_bp_idx_gp(MOV_B_IMM, MOV_BP_IDX_B);
}

TEST_F(TESTNAME, MOV_BP_IDX_C)
{
    mov_bp_idx_gp(MOV_C_IMM, MOV_BP_IDX_C);
}

TEST_F(TESTNAME, MOV_BP_IDX_D)
{
    mov_bp_idx_gp(MOV_D_IMM, MOV_BP_IDX_D);
}

TEST_F(TESTNAME, PUSH_BP_IDX)
{
    constexpr static byte code[] = {
        /* 8000 */ MOV_SP_IMM, 0x00, 0x20, // 6
        /* 8000 */ MOV_BP_SP,
        /* 0003 */ PUSH_BP_IDX, 0x10, //
        /* 8005 */ HLT                // 3
    };

    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, MOV_SP_IMM);
    ASSERT_FALSE(mem->poke(0x2010, 0xFE).is_error());
    ASSERT_FALSE(mem->poke(0x2011, 0xCA).is_error());

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(24);
    ASSERT_EQ(mem->peek(0x2000).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2001).value(), 0xCA);
}

TEST_F(TESTNAME, POP_BP_IDX)
{
    constexpr static byte code[] = {
        MOV_SP_IMM, 0x00, 0x20,
        MOV_BP_SP,
        MOV_SI_IMM, 0xFE, 0xCA,
        PUSH_SI,
        POP_BP_IDX, 0x10,
        HLT
    };

    mem->initialize(ROM_START, sizeof(code), code);
    check_memory(START_VECTOR, MOV_SP_IMM);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(38);
    ASSERT_EQ(mem->peek(0x2010).value(), 0xFE);
    ASSERT_EQ(mem->peek(0x2011).value(), 0xCA);
}

}
