/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Arithmetic
#include "controllertest.h"

namespace Obelix::CPU {

class TESTNAME : public HarnessTest {
};

//
// A R I T H M E T I C
//

typedef std::function<byte(Harness&, byte, byte)> Expect;

static Expect expect[16] = {
    /* 0x0 ADD */ [](Harness& system, byte lhs, byte rhs) {
        return (byte)(lhs + rhs);
    },

    /* 0x1 ADC */ [](Harness& system, byte lhs, byte rhs) { return (byte)(lhs + rhs + +(byte)(system.bus().isSet(SystemBus::C))); },

    /* 0x2 SUB */ [](Harness& system, byte lhs, byte rhs) { return (byte)(lhs - rhs); },

    /* 0x3 SBB */ [](Harness& system, byte lhs, byte rhs) { return (byte)(lhs - rhs - (byte)(system.bus().isSet(SystemBus::C))); },

    /* 0x4 AND */ [](Harness& system, byte lhs, byte rhs) { return lhs & rhs; },

    /* 0x5 OR  */ [](Harness& system, byte lhs, byte rhs) { return lhs | rhs; },

    /* 0x6 XOR */ [](Harness& system, byte lhs, byte rhs) { return lhs ^ rhs; },

    /* 0x7 INC */ [](Harness& system, byte lhs, byte rhs) { return (byte)(lhs + 1); },

    /* 0x8 DEC */ [](Harness& system, byte lhs, byte rhs) { return (byte)(lhs - 1); },

    /* 0x9 NOT */ [](Harness& system, byte lhs, byte rhs) { return ~lhs; },

    /* 0xA SHL */ [](Harness& system, byte lhs, byte rhs) {
        byte ret = lhs << 1;
        if (system.bus().isSet(SystemBus::C)) {
          ret |= 0x01;
        }
        return ret; },

    /* 0xB SHR */ [](Harness& system, byte lhs, byte rhs) {
        byte ret = lhs >> 1;
        if (system.bus().isSet(SystemBus::C)) {
          ret |= 0x80;
        }
        return ret; },

    /* 0xC */ nullptr,
    /* 0xD */ nullptr,

    /* 0xE CLR */ [](Harness& system, byte lhs, byte rhs) { return (byte)0; },

    /* 0xF */ [](Harness& system, byte lhs, byte rhs) { return lhs; },
};

static byte reg2instr[4] {
    MOV_A_IMM,
    MOV_B_IMM,
    MOV_C_IMM,
    MOV_D_IMM,
};

struct OpTest {
    byte m_value = 0x1F;  // 31 dec
    byte m_value2 = 0xF8; // 248
    byte m_op_instr = NOT_A;
    int m_reg = GP_A;
    int m_reg2 = GP_B;
    ALU::Operations m_op = ALU::Operations::NOT;

    OpTest(int reg, byte op_instr, ALU::Operations op, int reg2 = GP_B)
        : m_op_instr(op_instr)
        , m_reg(reg)
        , m_reg2(reg2)
        , m_op(op)
    {
    }

    [[nodiscard]] virtual const byte* bytes() const = 0;
    [[nodiscard]] virtual int bytesSize() const = 0;
    [[nodiscard]] virtual int regs() const = 0;
    [[nodiscard]] virtual int cycleCount() const = 0;

    void value(byte val)
    {
        m_value = val;
    }

    void execute(Harness& system, int cycles = -1) const
    {
        auto mem = system.component<Memory>();
        const byte* b = bytes();
        mem->initialize(RAM_START, bytesSize(), b);
        ASSERT_FALSE(mem->poke(RAM_START, reg2instr[m_reg]).is_error());
        ASSERT_FALSE(mem->poke(RAM_START + 1, m_value).is_error());
        word instrAddr = RAM_START + 2;
        if (regs() > 1) {
            ASSERT_FALSE(mem->poke(RAM_START + 2, reg2instr[m_reg2]).is_error());
            ASSERT_FALSE(mem->poke(RAM_START + 3, m_value2).is_error());
            instrAddr = RAM_START + 4;
        } else if (regs() == -2) {
            ASSERT_FALSE(mem->poke(RAM_START + 3, m_value2).is_error());
        }
        ASSERT_FALSE(mem->poke(instrAddr, m_op_instr).is_error());

        auto pc = system.component<AddressRegister>(PC);
        pc->setValue(RAM_START);
        ASSERT_EQ(pc->getValue(), RAM_START);

        byte e = expect[m_op](system, m_value, m_value2);
        auto cycles_used_or_error = system.run();
        ASSERT_FALSE(cycles_used_or_error.is_error());
        ASSERT_EQ(cycles_used_or_error.value(), (cycles < 0) ? cycleCount() : cycles);
        ASSERT_EQ(system.bus().halt(), false);

        auto r = system.component<Register>(m_reg);
        ASSERT_EQ(r->getValue(), e);
    }
};

struct UnaryOpTest : public OpTest {
    UnaryOpTest(int reg, byte op_instr, ALU::Operations op)
        : OpTest(reg, op_instr, op)
    {
    }

    [[nodiscard]] const byte* bytes() const override
    {
        return unary_op;
    }

    [[nodiscard]] int bytesSize() const override
    {
        return 4;
    }

    [[nodiscard]] int regs() const override
    {
        return 1;
    }

    [[nodiscard]] int cycleCount() const override
    {
        return 11;
    }
};

struct BinaryOpTest : public OpTest {
    BinaryOpTest(int reg, int reg2, byte op_instr, ALU::Operations op)
        : OpTest(reg, op_instr, op, reg2)
    {
    }

    [[nodiscard]] const byte* bytes() const override
    {
        return binary_op;
    }

    [[nodiscard]] int bytesSize() const override
    {
        return 6;
    }

    [[nodiscard]] int regs() const override
    {
        return 2;
    }

    int cycles = 16;
    [[nodiscard]] int cycleCount() const override
    {
        return cycles;
    }

    void values(byte val1, byte val2)
    {
        m_value = val1;
        m_value2 = val2;
    }
};

// mov a, #xx      4
// cmp a, #xx      5/6
// hlt             3
// total          12/13
//
// cmp 5 cycles, others 6.
const byte binary_op_const[] = {
    /* 2000 */ MOV_A_IMM,
    0x1F,
    /* 2002 */ CMP_A_IMM,
    0x42,
    /* 2004 */ HLT,
};

struct BinaryOpConstTest : public OpTest {
    BinaryOpConstTest(int reg, byte op_instr, ALU::Operations op)
        : OpTest(reg, op_instr, op)
    {
    }

    [[nodiscard]] const byte* bytes() const override
    {
        return binary_op_const;
    }

    [[nodiscard]] int bytesSize() const override
    {
        return 5;
    }

    [[nodiscard]] int regs() const override
    {
        return -2;
    }

    int cycles = 13;
    [[nodiscard]] int cycleCount() const override
    {
        return cycles;
    }

    void values(byte val1, byte val2)
    {
        m_value = val1;
        m_value2 = val2;
    }
};

#define TEST_ADD(r1, r2)                                                         \
    TEST_F(TESTNAME, ADD_##r1##_##r2)                                            \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, ADD_##r1##_##r2, ALU::Operations::ADD); \
        t.execute(system);                                                       \
    }                                                                            \
    TEST_F(TESTNAME, ADD_##r1##_##r2##_set_carry)                                \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, ADD_##r1##_##r2, ALU::Operations::ADD); \
        t.values(0xC0, 0xC0);                                                    \
        t.execute(system);                                                       \
        ASSERT_TRUE(system.bus().isSet(SystemBus::C));                           \
    }                                                                            \
    TEST_F(TESTNAME, ADD_##r1##_##r2##_set_overflow)                             \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, ADD_##r1##_##r2, ALU::Operations::ADD); \
        t.values(100, 50);                                                       \
        t.execute(system);                                                       \
        ASSERT_TRUE(system.bus().isSet(SystemBus::V));                           \
    }                                                                            \
    TEST_F(TESTNAME, ADD_##r1##_##r2##_set_zero)                                 \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, ADD_##r1##_##r2, ALU::Operations::ADD); \
        t.values(-20, 20);                                                       \
        t.execute(system);                                                       \
        ASSERT_TRUE(system.bus().isSet(SystemBus::Z));                           \
        ASSERT_TRUE(system.bus().isSet(SystemBus::C));                           \
    }

#define TEST_ADC(r1, r2)                                                         \
    TEST_F(TESTNAME, ADC_##r1##_##r2##_no_carry)                                 \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, ADC_##r1##_##r2, ALU::Operations::ADC); \
        system.bus().clearFlags();                                               \
        t.execute(system);                                                       \
    }                                                                            \
    TEST_F(TESTNAME, ADC_##r1##_##r2##_carry)                                    \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, ADC_##r1##_##r2, ALU::Operations::ADC); \
        system.bus().setFlag(SystemBus::C);                                      \
        t.execute(system);                                                       \
    }

#define TEST_SUB(r1, r2)                                                         \
    TEST_F(TESTNAME, SUB_##r1##_##r2)                                            \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, SUB_##r1##_##r2, ALU::Operations::SUB); \
        t.execute(system);                                                       \
    }

#define TEST_SBB(r1, r2)                                                         \
    TEST_F(TESTNAME, SBB_##r1##_##r2##_no_carry)                                 \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, SBB_##r1##_##r2, ALU::Operations::SBB); \
        system.bus().clearFlags();                                               \
        t.execute(system);                                                       \
    }                                                                            \
    TEST_F(TESTNAME, SBB_##r1##_##r2##_carry)                                    \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, SBB_##r1##_##r2, ALU::Operations::SBB); \
        system.bus().setFlag(SystemBus::C);                                      \
        t.execute(system);                                                       \
    }

#define TEST_AND(r1, r2)                                                         \
    TEST_F(TESTNAME, AND_##r1##_##r2)                                            \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, AND_##r1##_##r2, ALU::Operations::AND); \
        t.execute(system);                                                       \
    }

#define TEST_OR(r1, r2)                                                        \
    TEST_F(TESTNAME, OR_##r1##_##r2)                                           \
    {                                                                          \
        BinaryOpTest t(GP_##r1, GP_##r2, OR_##r1##_##r2, ALU::Operations::OR); \
        t.execute(system);                                                     \
    }

#define TEST_XOR(r1, r2)                                                         \
    TEST_F(TESTNAME, XOR_##r1##_##r2)                                            \
    {                                                                            \
        BinaryOpTest t(GP_##r1, GP_##r2, XOR_##r1##_##r2, ALU::Operations::XOR); \
        t.execute(system);                                                       \
    }

#define TEST_REGISTER_PERMUTATION(r1, r2) \
    TEST_ADD(r1, r2)                      \
    TEST_ADC(r1, r2)                      \
    TEST_SUB(r1, r2)                      \
    TEST_SBB(r1, r2)                      \
    TEST_AND(r1, r2)                      \
    TEST_OR(r1, r2)                       \
    TEST_XOR(r1, r2)

TEST_REGISTER_PERMUTATION(A, B)
TEST_REGISTER_PERMUTATION(A, C)
TEST_REGISTER_PERMUTATION(A, D)
TEST_REGISTER_PERMUTATION(B, C)
TEST_REGISTER_PERMUTATION(B, D)
TEST_REGISTER_PERMUTATION(C, A)
TEST_REGISTER_PERMUTATION(C, B)
TEST_REGISTER_PERMUTATION(C, D)
TEST_REGISTER_PERMUTATION(D, A)
TEST_REGISTER_PERMUTATION(D, B)

#define TEST_UNARY(reg)                                           \
    TEST_F(TESTNAME, NOT_##reg)                                   \
    {                                                             \
        UnaryOpTest t(GP_##reg, NOT_##reg, ALU::Operations::NOT); \
        t.execute(system);                                        \
    }                                                             \
    TEST_F(TESTNAME, SHL_##reg)                                   \
    {                                                             \
        UnaryOpTest t(GP_##reg, SHL_##reg, ALU::Operations::SHL); \
        t.execute(system);                                        \
    }                                                             \
    TEST_F(TESTNAME, SHR_##reg)                                   \
    {                                                             \
        UnaryOpTest t(GP_##reg, SHR_##reg, ALU::Operations::SHR); \
        t.execute(system);                                        \
    }                                                             \
    TEST_F(TESTNAME, CLR_##reg)                                   \
    {                                                             \
        UnaryOpTest t(GP_##reg, CLR_##reg, ALU::Operations::CLR); \
        t.execute(system, 12);                                    \
        ASSERT_TRUE(system.bus().isSet(SystemBus::Z));            \
    }                                                             \
    TEST_F(TESTNAME, INC_##reg)                                   \
    {                                                             \
        UnaryOpTest t(GP_##reg, INC_##reg, ALU::Operations::INC); \
        t.execute(system);                                        \
    }                                                             \
    TEST_F(TESTNAME, DEC_##reg)                                   \
    {                                                             \
        UnaryOpTest t(GP_##reg, DEC_##reg, ALU::Operations::DEC); \
        t.execute(system);                                        \
    }

TEST_UNARY(A)
TEST_UNARY(B)
TEST_UNARY(C)
TEST_UNARY(D)

#define TEST_CMP(r1, r2)                                             \
    TEST_F(TESTNAME, CMP_##r1##_##r2##_not_equal)                    \
    {                                                                \
        BinaryOpTest t(GP_##r1, GP_##r2, CMP_##r1##_##r2, ALU::CMP); \
        t.cycles = 15;                                               \
        t.execute(system);                                           \
        ASSERT_FALSE(system.bus().isSet(SystemBus::Z));              \
    }                                                                \
    TEST_F(TESTNAME, CMP_##r1##_##r2##_equal)                        \
    {                                                                \
        BinaryOpTest t(GP_##r1, GP_##r2, CMP_##r1##_##r2, ALU::CMP); \
        t.values(0x42, 0x42);                                        \
        t.cycles = 15;                                               \
        t.execute(system);                                           \
        ASSERT_TRUE(system.bus().isSet(SystemBus::Z));               \
    }

TEST_CMP(A, B)
TEST_CMP(A, C)
TEST_CMP(A, D)
TEST_CMP(B, C)
TEST_CMP(B, D)
TEST_CMP(C, D)

#define TEST_CMP_IMM(reg)                                         \
    TEST_F(TESTNAME, CMP_##reg##_IMM_not_equal)                   \
    {                                                             \
        BinaryOpConstTest t(GP_##reg, CMP_##reg##_IMM, ALU::CMP); \
        t.cycles = 12;                                            \
        t.execute(system);                                        \
        ASSERT_FALSE(system.bus().isSet(SystemBus::Z));           \
    }                                                             \
    TEST_F(TESTNAME, CMP_##reg##_IMM_equal)                       \
    {                                                             \
        BinaryOpConstTest t(GP_##reg, CMP_##reg##_IMM, ALU::CMP); \
        t.values(0x42, 0x42);                                     \
        t.cycles = 12;                                            \
        t.execute(system);                                        \
        ASSERT_TRUE(system.bus().isSet(SystemBus::Z));            \
    }

#define TEST_AND_IMM(reg)                                         \
    TEST_F(TESTNAME, AND_##reg##_IMM_not_equal)                   \
    {                                                             \
        BinaryOpConstTest t(GP_##reg, AND_##reg##_IMM, ALU::AND); \
        t.execute(system);                                        \
    }                                                             \
    TEST_F(TESTNAME, AND_##reg##_IMM_equal)                       \
    {                                                             \
        BinaryOpConstTest t(GP_##reg, AND_##reg##_IMM, ALU::AND); \
        t.execute(system);                                        \
    }

#define TEST_OR_IMM(reg)                                        \
    TEST_F(TESTNAME, OR_##reg##_IMM_not_equal)                  \
    {                                                           \
        BinaryOpConstTest t(GP_##reg, OR_##reg##_IMM, ALU::OR); \
        t.execute(system);                                      \
    }                                                           \
    TEST_F(TESTNAME, OR_##reg##_IMM_equal)                      \
    {                                                           \
        BinaryOpConstTest t(GP_##reg, OR_##reg##_IMM, ALU::OR); \
        t.execute(system);                                      \
    }

#define TEST_IMMEDIATE_OPS(reg) \
    TEST_CMP_IMM(reg)           \
    TEST_AND_IMM(reg)           \
    TEST_OR_IMM(reg)

TEST_IMMEDIATE_OPS(A)
TEST_IMMEDIATE_OPS(B)
TEST_IMMEDIATE_OPS(C)
TEST_IMMEDIATE_OPS(D)

// mov a, #xx      4        x4  16
// add ab,cd       8             8
// hlt             3             3
// total                        27
const byte wide_binary_op[] = {
    /* 2000 */ MOV_A_IMM,
    0x1F,
    /* 2002 */ MOV_B_IMM,
    0xF8,
    /* 2004 */ MOV_C_IMM,
    0x36,
    /* 2006 */ MOV_D_IMM,
    0xA7,
    /* 2008 */ NOP,
    /* 2009 */ HLT,
};

void test_wide_op(Harness& system, byte opcode)
{
    auto mem = system.component<Memory>();
    mem->initialize(RAM_START, 10, wide_binary_op);
    ASSERT_FALSE(mem->poke(0x2008, opcode).is_error());

    auto pc = system.component<AddressRegister>(PC);
    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles_or_error = system.run();
    ASSERT_FALSE(cycles_or_error.is_error());
    ASSERT_EQ(cycles_or_error.value(), 27);
    ASSERT_EQ(system.bus().halt(), false);
}

TEST_F(TESTNAME, add_AB_CD)
{
    test_wide_op(system, ADD_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F + 0xA736) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F + 0xA736) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, adc_AB_CD_NoCarry)
{
    system.bus().clearFlags();
    test_wide_op(system, ADC_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F + 0xA736) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F + 0xA736) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, adc_AB_CD_CarrySet)
{
    system.bus().setFlag(SystemBus::C);
    test_wide_op(system, ADC_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F + 0xA736 + 1) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F + 0xA736 + 1) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, sub_AB_CD)
{
    test_wide_op(system, SUB_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F - 0xA736) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F - 0xA736) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, sbb_AB_CD_NoCarry)
{
    system.bus().clearFlags();
    test_wide_op(system, SBB_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F - 0xA736) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F - 0xA736) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, sbb_AB_CD_CarrySet)
{
    system.bus().setFlag(SystemBus::C);
    test_wide_op(system, SBB_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F - 0xA736 - 1) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F - 0xA736 - 1) & 0xFF00) >> 8);
}

// -- I N C / D E C  SI / DI ---------------------------------------------

// mov si, #xxxx   6  x2 = 12
// inc si                   3
// hlt                      3
// total                   18
const byte wide_unary_op[] = {
    /* 2000 */ MOV_SI_IMM,
    0x67,
    0x04,
    /* 2003 */ MOV_DI_IMM,
    0x67,
    0x05,
    /* 2006 */ NOP,
    /* 2007 */ HLT,
};

void test_wide_unary_op(Harness& system, byte opcode)
{
    auto mem = system.component<Memory>();
    mem->initialize(RAM_START, 8, wide_unary_op);
    ASSERT_FALSE(mem->poke(0x2006, opcode).is_error());

    auto pc = system.component<AddressRegister>(PC);
    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles_or_error = system.run();
    ASSERT_FALSE(cycles_or_error.is_error());
    ASSERT_EQ(cycles_or_error.value(), 18);
    ASSERT_EQ(system.bus().halt(), false);
}

TEST_F(TESTNAME, incSi)
{
    test_wide_unary_op(system, INC_SI);
    ASSERT_EQ(si->getValue(), 0x0468);
}

TEST_F(TESTNAME, incDi)
{
    test_wide_unary_op(system, INC_DI);
    ASSERT_EQ(di->getValue(), 0x0568);
}

TEST_F(TESTNAME, decSi)
{
    test_wide_unary_op(system, DEC_SI);
    ASSERT_EQ(si->getValue(), 0x0466);
}

TEST_F(TESTNAME, decDi)
{
    test_wide_unary_op(system, DEC_DI);
    ASSERT_EQ(di->getValue(), 0x0566);
}

}
