/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define TESTNAME Arithmetic
#include "controllertest.h"

//
// A R I T H M E T I C
//

typedef std::function<byte(Harness*, byte, byte)> Expect;

static Expect expect[16] = {
    /* 0x0 ADD */ [](Harness* system, byte lhs, byte rhs) {
        return (byte)(lhs + rhs);
    },

    /* 0x1 ADC */ [](Harness* system, byte lhs, byte rhs) { return (byte)(lhs + rhs + +(byte)(system->bus().isSet(SystemBus::C))); },

    /* 0x2 SUB */ [](Harness* system, byte lhs, byte rhs) { return (byte)(lhs - rhs); },

    /* 0x3 SBB */ [](Harness* system, byte lhs, byte rhs) { return (byte)(lhs - rhs
                                                                - (byte)(system->bus().isSet(SystemBus::C))); },

    /* 0x4 AND */ [](Harness* system, byte lhs, byte rhs) { return lhs & rhs; },

    /* 0x5 OR  */ [](Harness* system, byte lhs, byte rhs) { return lhs | rhs; },

    /* 0x6 XOR */ [](Harness* system, byte lhs, byte rhs) { return lhs ^ rhs; },

    /* 0x7 INC */ [](Harness* system, byte lhs, byte rhs) { return (byte)(lhs + 1); },

    /* 0x8 DEC */ [](Harness* system, byte lhs, byte rhs) { return (byte)(lhs - 1); },

    /* 0x9 NOT */ [](Harness* system, byte lhs, byte rhs) { return ~lhs; },

    /* 0xA SHL */ [](Harness* system, byte lhs, byte rhs) {
    byte ret = lhs << 1;
    if (system -> bus().isSet(SystemBus::C)) {
      ret |= 0x01;
    }
    return ret; },

    /* 0xB SHR */ [](Harness* system, byte lhs, byte rhs) {
    byte ret = lhs >> 1;
    if (system -> bus().isSet(SystemBus::C)) {
      ret |= 0x80;
    }
    return ret; },

    /* 0xC */ nullptr,
    /* 0xD */ nullptr,

    /* 0xE CLR */ [](Harness* system, byte lhs, byte rhs) { return (byte)0; },

    /* 0xF */ [](Harness* system, byte lhs, byte rhs) { return lhs; },
};

static byte reg2instr[4] {
    MOV_A_CONST,
    MOV_B_CONST,
    MOV_C_CONST,
    MOV_D_CONST,
};

struct OpTest {
    byte m_value = 0x1F;  // 31 dec
    byte m_value2 = 0xF8; // 248
    byte m_op_instr = NOT_A;
    int m_reg = GP_A;
    int m_reg2 = GP_B;
    ALU::Operations m_op = ALU::Operations::NOT;

    OpTest(int reg, byte op_instr, ALU::Operations op, int reg2 = GP_B)
        : m_reg(reg)
        , m_op_instr(op_instr)
        , m_op(op)
        , m_reg2(reg2)
    {
    }

    virtual const byte* bytes() const = 0;
    virtual int bytesSize() const = 0;
    virtual int regs() const = 0;
    virtual int cycleCount() const = 0;

    void value(byte val)
    {
        m_value = val;
    }

    virtual void execute(Harness* system, int cycles = -1)
    {
        auto* mem = dynamic_cast<Memory*>(system->component(MEMADDR));
        const byte* b = bytes();
        mem->initialize(RAM_START, bytesSize(), b);
        ASSERT_EQ((*mem)[RAM_START], b[0]);
        (*mem)[RAM_START] = reg2instr[m_reg];
        (*mem)[RAM_START + 1] = m_value;
        word instrAddr = RAM_START + 2;
        if (regs() > 1) {
            (*mem)[RAM_START + 2] = reg2instr[m_reg2];
            (*mem)[RAM_START + 3] = m_value2;
            instrAddr = RAM_START + 4;
        } else if (regs() == -2) {
            (*mem)[RAM_START + 3] = m_value2;
        }
        (*mem)[instrAddr] = m_op_instr;

        auto* pc = dynamic_cast<AddressRegister*>(system->component(PC));
        pc->setValue(RAM_START);
        ASSERT_EQ(pc->getValue(), RAM_START);

        byte e = expect[m_op](system, m_value, m_value2);
        auto cyclesUsed = system->run();
        ASSERT_EQ(system->error(), NoError);
        ASSERT_EQ(cyclesUsed, (cycles < 0) ? cycleCount() : cycles);
        ASSERT_EQ(system->bus().halt(), false);

        auto* r = dynamic_cast<Register*>(system->component(m_reg));
        ASSERT_EQ(r->getValue(), e);
    }
};

// mov a, #xx      4
// not a           4
// hlt             3
// total          11
const byte unary_op[] = {
    /* 2000 */ MOV_A_CONST,
    0x1F,
    /* 2002 */ NOP,
    /* 2003 */ HLT,
};

struct UnaryOpTest : public OpTest {
    UnaryOpTest(int reg, byte op_instr, ALU::Operations op)
        : OpTest(reg, op_instr, op)
    {
    }

    const byte* bytes() const override
    {
        return unary_op;
    }

    int bytesSize() const override
    {
        return 4;
    }

    int regs() const override
    {
        return 1;
    }

    int cycleCount() const override
    {
        return 11;
    }
};

// mov a, #xx      4        x2   8
// add a, b        5             5
// hlt             3             3
// total                        16
const byte binary_op[] = {
    /* 2000 */ MOV_A_CONST,
    0x1F,
    /* 2002 */ MOV_B_CONST,
    0xF8,
    /* 2004 */ NOP,
    /* 2005 */ HLT,
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
    /* 2000 */ MOV_A_CONST,
    0x1F,
    /* 2002 */ CMP_A_CONST,
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

TEST_F(TESTNAME, addAB)
{
    BinaryOpTest t(GP_A, GP_B, ADD_A_B, ALU::Operations::ADD);
    t.execute(system);
}

TEST_F(TESTNAME, addABSetCarry)
{
    BinaryOpTest t(GP_A, GP_B, ADD_A_B, ALU::Operations::ADD);
    t.values(0xC0, 0xC0);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, addABSetOverflow)
{
    BinaryOpTest t(GP_A, GP_B, ADD_A_B, ALU::Operations::ADD);
    t.values(100, 50);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(TESTNAME, addABSetZero)
{
    BinaryOpTest t(GP_A, GP_B, ADD_A_B, ALU::Operations::ADD);
    t.values(-20, 20);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, adcABCarrySet)
{
    BinaryOpTest t(GP_A, GP_B, ADC_A_B, ALU::Operations::ADC);
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, adcABCarryNotSet)
{
    BinaryOpTest t(GP_A, GP_B, ADC_A_B, ALU::Operations::ADC);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, subAB)
{
    BinaryOpTest t(GP_A, GP_B, SUB_A_B, ALU::Operations::SUB);
    t.execute(system);
}

TEST_F(TESTNAME, sbbABNoCarry)
{
    BinaryOpTest t(GP_A, GP_B, SBB_A_B, ALU::Operations::SBB);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, sbbABWithCarry)
{
    BinaryOpTest t(GP_A, GP_B, SBB_A_B, ALU::Operations::SBB);
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, andAB)
{
    BinaryOpTest t(GP_A, GP_B, AND_A_B, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, orAB)
{
    BinaryOpTest t(GP_A, GP_B, OR_A_B, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, xorAB)
{
    BinaryOpTest t(GP_A, GP_B, XOR_A_B, ALU::Operations::XOR);
    t.execute(system);
}

// Register A Unary Operations

TEST_F(TESTNAME, notA)
{
    UnaryOpTest t(GP_A, NOT_A, ALU::Operations::NOT);
    t.execute(system);
}

TEST_F(TESTNAME, shlA)
{
    UnaryOpTest t(GP_A, SHL_A, ALU::Operations::SHL);
    t.execute(system);
}

TEST_F(TESTNAME, shrA)
{
    UnaryOpTest t(GP_A, SHR_A, ALU::Operations::SHR);
    t.execute(system);
}

TEST_F(TESTNAME, clrA)
{
    UnaryOpTest t(GP_A, CLR_A, ALU::CLR);
    t.execute(system, 12);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

// Arithmetic A, C.

TEST_F(TESTNAME, addAC)
{
    BinaryOpTest t(GP_A, GP_C, ADD_A_C, ALU::Operations::ADD);
    t.execute(system);
}

TEST_F(TESTNAME, addACSetCarry)
{
    BinaryOpTest t(GP_A, GP_C, ADD_A_C, ALU::Operations::ADD);
    t.values(0xC0, 0xC0);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, addACSetOverflow)
{
    BinaryOpTest t(GP_A, GP_C, ADD_A_C, ALU::Operations::ADD);
    t.values(100, 50);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(TESTNAME, addACSetZero)
{
    BinaryOpTest t(GP_A, GP_C, ADD_A_C, ALU::Operations::ADD);
    t.values(-20, 20);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, adcACCarrySet)
{
    BinaryOpTest t(GP_A, GP_C, ADC_A_C, ALU::Operations::ADC);
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, adcACCarryNotSet)
{
    BinaryOpTest t(GP_A, GP_C, ADC_A_C, ALU::Operations::ADC);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, subAC)
{
    BinaryOpTest t(GP_A, GP_C, SUB_A_C, ALU::Operations::SUB);
    t.execute(system);
}

TEST_F(TESTNAME, sbbACNoCarry)
{
    BinaryOpTest t(GP_A, GP_C, SBB_A_C, ALU::Operations::SBB);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, sbbACWithCarry)
{
    BinaryOpTest t(GP_A, GP_C, SBB_A_C, ALU::Operations::SBB);
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, andAC)
{
    BinaryOpTest t(GP_A, GP_C, AND_A_C, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, orAC)
{
    BinaryOpTest t(GP_A, GP_C, OR_A_C, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, xorAC)
{
    BinaryOpTest t(GP_A, GP_C, XOR_A_C, ALU::Operations::XOR);
    t.execute(system);
}

// Arithmetic A, D.

TEST_F(TESTNAME, addAD)
{
    BinaryOpTest t(GP_A, GP_D, ADD_A_D, ALU::Operations::ADD);
    t.execute(system);
}

TEST_F(TESTNAME, addADSetCarry)
{
    BinaryOpTest t(GP_A, GP_D, ADD_A_D, ALU::Operations::ADD);
    t.values(0xC0, 0xC0);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, addADSetOverflow)
{
    BinaryOpTest t(GP_A, GP_D, ADD_A_D, ALU::Operations::ADD);
    t.values(100, 50);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(TESTNAME, addADSetZero)
{
    BinaryOpTest t(GP_A, GP_D, ADD_A_D, ALU::Operations::ADD);
    t.values(-20, 20);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, adcADCarrySet)
{
    BinaryOpTest t(GP_A, GP_D, ADC_A_D, ALU::Operations::ADC);
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, adcADCarryNotSet)
{
    BinaryOpTest t(GP_A, GP_D, ADC_A_D, ALU::Operations::ADC);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, subAD)
{
    BinaryOpTest t(GP_A, GP_D, SUB_A_D, ALU::Operations::SUB);
    t.execute(system);
}

TEST_F(TESTNAME, sbbADNoCarry)
{
    BinaryOpTest t(GP_A, GP_D, SBB_A_D, ALU::Operations::SBB);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, sbbADWithCarry)
{
    BinaryOpTest t(GP_A, GP_D, SBB_A_D, ALU::Operations::SBB);
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, andAD)
{
    BinaryOpTest t(GP_A, GP_D, AND_A_D, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, orAD)
{
    BinaryOpTest t(GP_A, GP_D, OR_A_D, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, xorAD)
{
    BinaryOpTest t(GP_A, GP_D, XOR_A_D, ALU::Operations::XOR);
    t.execute(system);
}

// Register B Unary Operations

TEST_F(TESTNAME, notB)
{
    UnaryOpTest t(GP_B, NOT_B, ALU::Operations::NOT);
    t.execute(system);
}

TEST_F(TESTNAME, shlB)
{
    UnaryOpTest t(GP_B, SHL_B, ALU::Operations::SHL);
    t.execute(system);
}

TEST_F(TESTNAME, shrB)
{
    UnaryOpTest t(GP_B, SHR_B, ALU::Operations::SHR);
    t.execute(system);
}

TEST_F(TESTNAME, clrB)
{
    UnaryOpTest t(GP_B, CLR_B, ALU::CLR);
    t.execute(system, 12);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

// Arithmetic B, C.

TEST_F(TESTNAME, addBC)
{
    BinaryOpTest t(GP_B, GP_C, ADD_B_C, ALU::Operations::ADD);
    t.execute(system);
}

TEST_F(TESTNAME, addBCSetCarry)
{
    BinaryOpTest t(GP_B, GP_C, ADD_B_C, ALU::Operations::ADD);
    t.values(0xC0, 0xC0);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, addBCSetOverflow)
{
    BinaryOpTest t(GP_B, GP_C, ADD_B_C, ALU::Operations::ADD);
    t.values(100, 50);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(TESTNAME, addBCSetZero)
{
    BinaryOpTest t(GP_B, GP_C, ADD_B_C, ALU::Operations::ADD);
    t.values(-20, 20);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, adcBCCarrySet)
{
    BinaryOpTest t(GP_B, GP_C, ADC_B_C, ALU::Operations::ADC);
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, adcBCCarryNotSet)
{
    BinaryOpTest t(GP_B, GP_C, ADC_B_C, ALU::Operations::ADC);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, subBC)
{
    BinaryOpTest t(GP_B, GP_C, SUB_B_C, ALU::Operations::SUB);
    t.execute(system);
}

TEST_F(TESTNAME, sbbBCNoCarry)
{
    BinaryOpTest t(GP_B, GP_C, SBB_B_C, ALU::Operations::SBB);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, sbbBCWithCarry)
{
    BinaryOpTest t(GP_B, GP_C, SBB_B_C, ALU::Operations::SBB);
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, andBC)
{
    BinaryOpTest t(GP_B, GP_C, AND_B_C, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, orBC)
{
    BinaryOpTest t(GP_B, GP_C, OR_B_C, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, xorBC)
{
    BinaryOpTest t(GP_B, GP_C, XOR_B_C, ALU::Operations::XOR);
    t.execute(system);
}

// Arithmetic B, D.

TEST_F(TESTNAME, addBD)
{
    BinaryOpTest t(GP_B, GP_D, ADD_B_D, ALU::Operations::ADD);
    t.execute(system);
}

TEST_F(TESTNAME, addBDSetCarry)
{
    BinaryOpTest t(GP_B, GP_D, ADD_B_D, ALU::Operations::ADD);
    t.values(0xC0, 0xC0);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, addBDSetOverflow)
{
    BinaryOpTest t(GP_B, GP_D, ADD_B_D, ALU::Operations::ADD);
    t.values(100, 50);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(TESTNAME, addBDSetZero)
{
    BinaryOpTest t(GP_B, GP_D, ADD_B_D, ALU::Operations::ADD);
    t.values(-20, 20);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, adcBDCarrySet)
{
    BinaryOpTest t(GP_B, GP_D, ADC_B_D, ALU::Operations::ADC);
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, adcBDCarryNotSet)
{
    BinaryOpTest t(GP_B, GP_D, ADC_B_D, ALU::Operations::ADC);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, subBD)
{
    BinaryOpTest t(GP_B, GP_D, SUB_B_D, ALU::Operations::SUB);
    t.execute(system);
}

TEST_F(TESTNAME, sbbBDNoCarry)
{
    BinaryOpTest t(GP_B, GP_D, SBB_B_D, ALU::Operations::SBB);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, sbbBDWithCarry)
{
    BinaryOpTest t(GP_B, GP_D, SBB_B_D, ALU::Operations::SBB);
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, andBD)
{
    BinaryOpTest t(GP_B, GP_D, AND_B_D, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, orBD)
{
    BinaryOpTest t(GP_B, GP_D, OR_B_D, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, xorBD)
{
    BinaryOpTest t(GP_B, GP_D, XOR_B_D, ALU::Operations::XOR);
    t.execute(system);
}

// Register C Unary Operations

TEST_F(TESTNAME, notC)
{
    UnaryOpTest t(GP_C, NOT_C, ALU::Operations::NOT);
    t.execute(system);
}

TEST_F(TESTNAME, shlC)
{
    UnaryOpTest t(GP_C, SHL_C, ALU::Operations::SHL);
    t.execute(system);
}

TEST_F(TESTNAME, shrC)
{
    UnaryOpTest t(GP_C, SHR_C, ALU::Operations::SHR);
    t.execute(system);
}

TEST_F(TESTNAME, clrC)
{
    UnaryOpTest t(GP_C, CLR_C, ALU::CLR);
    t.execute(system, 12);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

// Arithmetic C, D.

TEST_F(TESTNAME, addCD)
{
    BinaryOpTest t(GP_C, GP_D, ADD_C_D, ALU::Operations::ADD);
    t.execute(system);
}

TEST_F(TESTNAME, addCDSetCarry)
{
    BinaryOpTest t(GP_C, GP_D, ADD_C_D, ALU::Operations::ADD);
    t.values(0xC0, 0xC0);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, addCDSetOverflow)
{
    BinaryOpTest t(GP_C, GP_D, ADD_C_D, ALU::Operations::ADD);
    t.values(100, 50);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(TESTNAME, addCDSetZero)
{
    BinaryOpTest t(GP_C, GP_D, ADD_C_D, ALU::Operations::ADD);
    t.values(-20, 20);
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(TESTNAME, adcCDCarrySet)
{
    BinaryOpTest t(GP_C, GP_D, ADC_C_D, ALU::Operations::ADC);
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, adcCDCarryNotSet)
{
    BinaryOpTest t(GP_C, GP_D, ADC_C_D, ALU::Operations::ADC);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, subCD)
{
    BinaryOpTest t(GP_C, GP_D, SUB_C_D, ALU::Operations::SUB);
    t.execute(system);
}

TEST_F(TESTNAME, sbbCDNoCarry)
{
    BinaryOpTest t(GP_C, GP_D, SBB_C_D, ALU::Operations::SBB);
    system->bus().clearFlags();
    t.execute(system);
}

TEST_F(TESTNAME, sbbCDWithCarry)
{
    BinaryOpTest t(GP_C, GP_D, SBB_C_D, ALU::Operations::SBB);
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    t.execute(system);
}

TEST_F(TESTNAME, andCD)
{
    BinaryOpTest t(GP_C, GP_D, AND_C_D, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, orCD)
{
    BinaryOpTest t(GP_C, GP_D, OR_C_D, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, xorCD)
{
    BinaryOpTest t(GP_C, GP_D, XOR_C_D, ALU::Operations::XOR);
    t.execute(system);
}

// Register D Unary Operations

TEST_F(TESTNAME, notD)
{
    UnaryOpTest t(GP_D, NOT_D, ALU::Operations::NOT);
    t.execute(system);
}

TEST_F(TESTNAME, shlD)
{
    UnaryOpTest t(GP_D, SHL_D, ALU::Operations::SHL);
    t.execute(system);
}

TEST_F(TESTNAME, shrD)
{
    UnaryOpTest t(GP_D, SHR_D, ALU::Operations::SHR);
    t.execute(system);
}

TEST_F(TESTNAME, clrD)
{
    UnaryOpTest t(GP_D, CLR_D, ALU::CLR);
    t.execute(system, 12);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

// mov a, #xx      4        x4  16
// add ab,cd       8             8
// hlt             3             3
// total                        27
const byte wide_binary_op[] = {
    /* 2000 */ MOV_A_CONST,
    0x1F,
    /* 2002 */ MOV_B_CONST,
    0xF8,
    /* 2004 */ MOV_C_CONST,
    0x36,
    /* 2006 */ MOV_D_CONST,
    0xA7,
    /* 2008 */ NOP,
    /* 2009 */ HLT,
};

void test_wide_op(Harness* system, byte opcode)
{
    auto* mem = dynamic_cast<Memory*>(system->component(MEMADDR));
    mem->initialize(RAM_START, 10, wide_binary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_A_CONST);
    (*mem)[0x2008] = opcode;

    auto* pc = dynamic_cast<AddressRegister*>(system->component(PC));
    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 27);
    ASSERT_EQ(system->bus().halt(), false);
}

TEST_F(TESTNAME, add_AB_CD)
{
    test_wide_op(system, ADD_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F + 0xA736) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F + 0xA736) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, adc_AB_CD_NoCarry)
{
    system->bus().clearFlags();
    test_wide_op(system, ADC_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F + 0xA736) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F + 0xA736) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, adc_AB_CD_CarrySet)
{
    system->bus().setFlag(SystemBus::C);
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
    system->bus().clearFlags();
    test_wide_op(system, SBB_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F - 0xA736) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F - 0xA736) & 0xFF00) >> 8);
}

TEST_F(TESTNAME, sbb_AB_CD_CarrySet)
{
    system->bus().setFlag(SystemBus::C);
    test_wide_op(system, SBB_AB_CD);
    ASSERT_EQ(gp_a->getValue(), (0xF81F - 0xA736 - 1) & 0x00FF);
    ASSERT_EQ(gp_b->getValue(), ((0xF81F - 0xA736 - 1) & 0xFF00) >> 8);
}

// CMP X,Y

TEST_F(TESTNAME, cmpABNotEqual)
{
    BinaryOpTest t(GP_A, GP_B, CMP_A_B, ALU::CMP);
    t.cycles = 15;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpABEqual)
{
    BinaryOpTest t(GP_A, GP_B, CMP_A_B, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 15;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpACNotEqual)
{
    BinaryOpTest t(GP_A, GP_C, CMP_A_C, ALU::CMP);
    t.cycles = 15;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpACEqual)
{
    BinaryOpTest t(GP_A, GP_C, CMP_A_C, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 15;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpADNotEqual)
{
    BinaryOpTest t(GP_A, GP_D, CMP_A_D, ALU::CMP);
    t.cycles = 15;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpADEqual)
{
    BinaryOpTest t(GP_A, GP_D, CMP_A_D, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 15;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpBCNotEqual)
{
    BinaryOpTest t(GP_B, GP_C, CMP_B_C, ALU::CMP);
    t.cycles = 15;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpBCEqual)
{
    BinaryOpTest t(GP_B, GP_C, CMP_B_C, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 15;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpBDNotEqual)
{
    BinaryOpTest t(GP_B, GP_D, CMP_B_D, ALU::CMP);
    t.cycles = 15;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmpBDEqual)
{
    BinaryOpTest t(GP_B, GP_D, CMP_B_D, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 15;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

// -- INC / DEC ----------------------------------------------------------

TEST_F(TESTNAME, incA)
{
    UnaryOpTest t(GP_A, INC_A, ALU::Operations::INC);
    t.execute(system);
}

TEST_F(TESTNAME, incB)
{
    UnaryOpTest t(GP_B, INC_B, ALU::Operations::INC);
    t.execute(system);
}

TEST_F(TESTNAME, incC)
{
    UnaryOpTest t(GP_C, INC_C, ALU::Operations::INC);
    t.execute(system);
}

TEST_F(TESTNAME, incD)
{
    UnaryOpTest t(GP_D, INC_D, ALU::Operations::INC);
    t.execute(system);
}

TEST_F(TESTNAME, decA)
{
    UnaryOpTest t(GP_A, DEC_A, ALU::Operations::DEC);
    t.execute(system);
}

TEST_F(TESTNAME, decB)
{
    UnaryOpTest t(GP_B, DEC_B, ALU::Operations::DEC);
    t.execute(system);
}

TEST_F(TESTNAME, decC)
{
    UnaryOpTest t(GP_C, DEC_C, ALU::Operations::DEC);
    t.execute(system);
}

TEST_F(TESTNAME, decD)
{
    UnaryOpTest t(GP_D, DEC_D, ALU::Operations::DEC);
    t.execute(system);
}

// -- I N C / D E C  SI / DI ---------------------------------------------

// mov si, #xxxx   6  x2 = 12
// inc si                   3
// hlt                      3
// total                   18
const byte wide_unary_op[] = {
    /* 2000 */ MOV_SI_CONST,
    0x67,
    0x04,
    /* 2003 */ MOV_DI_CONST,
    0x67,
    0x05,
    /* 2006 */ NOP,
    /* 2007 */ HLT,
};

void test_wide_unary_op(Harness* system, byte opcode)
{
    auto* mem = dynamic_cast<Memory*>(system->component(MEMADDR));
    mem->initialize(RAM_START, 8, wide_unary_op);
    ASSERT_EQ((*mem)[RAM_START], MOV_SI_CONST);
    (*mem)[0x2006] = opcode;

    auto* pc = dynamic_cast<AddressRegister*>(system->component(PC));
    pc->setValue(RAM_START);
    ASSERT_EQ(pc->getValue(), RAM_START);

    auto cycles = system->run();
    ASSERT_EQ(system->error(), NoError);
    ASSERT_EQ(cycles, 18);
    ASSERT_EQ(system->bus().halt(), false);
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

// -- CMP X,#0x00

TEST_F(TESTNAME, cmp_A_0x00_NotEqual)
{
    BinaryOpConstTest t(GP_A, CMP_A_CONST, ALU::CMP);
    t.cycles = 12;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmp_A_0x00_Equal)
{
    BinaryOpConstTest t(GP_A, CMP_A_CONST, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 12;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmp_B_0x00_NotEqual)
{
    BinaryOpConstTest t(GP_B, CMP_B_CONST, ALU::CMP);
    t.cycles = 12;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmp_B_0x00_Equal)
{
    BinaryOpConstTest t(GP_B, CMP_B_CONST, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 12;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmp_C_0x00_NotEqual)
{
    BinaryOpConstTest t(GP_C, CMP_C_CONST, ALU::CMP);
    t.cycles = 12;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmp_C_0x00_Equal)
{
    BinaryOpConstTest t(GP_C, CMP_C_CONST, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 12;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmp_D_0x00_NotEqual)
{
    BinaryOpConstTest t(GP_D, CMP_D_CONST, ALU::CMP);
    t.cycles = 12;
    t.execute(system);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(TESTNAME, cmp_D_0x00_Equal)
{
    BinaryOpConstTest t(GP_D, CMP_D_CONST, ALU::CMP);
    t.values(0x42, 0x42);
    t.cycles = 12;
    t.execute(system);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

// -- OR X,#0x00

TEST_F(TESTNAME, or_A_0x00)
{
    BinaryOpConstTest t(GP_A, OR_A_CONST, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, or_B_0x00)
{
    BinaryOpConstTest t(GP_B, OR_B_CONST, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, or_C_0x00)
{
    BinaryOpConstTest t(GP_C, OR_C_CONST, ALU::Operations::OR);
    t.execute(system);
}

TEST_F(TESTNAME, or_D_0x00)
{
    BinaryOpConstTest t(GP_D, OR_D_CONST, ALU::Operations::OR);
    t.execute(system);
}

// -- AND X,#0x00

TEST_F(TESTNAME, and_A_0x00)
{
    BinaryOpConstTest t(GP_A, AND_A_CONST, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, and_B_0x00)
{
    BinaryOpConstTest t(GP_B, AND_B_CONST, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, and_C_0x00)
{
    BinaryOpConstTest t(GP_C, AND_C_CONST, ALU::Operations::AND);
    t.execute(system);
}

TEST_F(TESTNAME, and_D_0x00)
{
    BinaryOpConstTest t(GP_D, AND_D_CONST, ALU::Operations::AND);
    t.execute(system);
}
