/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <gtest/gtest.h>

#include "cpu/alu.h"
#include "cpu/harness.h"

static int LHS = 0x4;
static int RHS = 0x5;

namespace Obelix::CPU {

class ALUTest : public ::testing::Test {
protected:
    Harness system;
    std::shared_ptr<Register> lhs = std::make_shared<Register>(LHS);
    std::shared_ptr<ALU> alu = std::make_shared<ALU>(0x5, lhs);

    void SetUp() override
    {
        system.insert(lhs);
        system.insert(alu);
    }
};

TEST_F(ALUTest, add)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x03).is_error());
    ASSERT_EQ(lhs->getValue(), 0x03);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADD, 0x02).is_error());
    ASSERT_EQ(alu->getValue(), 0x02);
    ASSERT_EQ(lhs->getValue(), 0x05);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, addSetZero)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x00).is_error());
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADD, 0x00).is_error());
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, addSetCarry)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0xFE).is_error());
    ASSERT_EQ(lhs->getValue(), 0xFE);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADD, 0x03).is_error());
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x01);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::C));
}

TEST_F(ALUTest, addSetOverflowPosPos)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, (byte)80).is_error());
    ASSERT_EQ(lhs->getValue(), 0x50);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADD, (byte)80).is_error());
    ASSERT_EQ(alu->getValue(), 0x50);
    ASSERT_EQ(lhs->getValue(), 0xA0);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, addSetOverflowNegNeg)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, (byte)-80).is_error());
    ASSERT_EQ(lhs->getValue(), 0xB0);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADD, (byte)-80).is_error());
    ASSERT_EQ(alu->getValue(), 0xB0);
    ASSERT_EQ(lhs->getValue(), 0x60);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adc)
{
    system.bus().clearFlags();
    system.bus().setFlag(SystemBus::C);
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x03).is_error());
    ASSERT_EQ(lhs->getValue(), 0x03);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADC, 0x02).is_error());
    ASSERT_EQ(alu->getValue(), 0x02);
    ASSERT_EQ(lhs->getValue(), 0x06);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcNoCarry)
{
    system.bus().clearFlags();
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x03).is_error());
    ASSERT_EQ(lhs->getValue(), 0x03);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADC, 0x02).is_error());
    ASSERT_EQ(alu->getValue(), 0x02);
    ASSERT_EQ(lhs->getValue(), 0x05);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcNoCarrySetCarry)
{
    system.bus().clearFlags();
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0xFE).is_error());
    ASSERT_EQ(lhs->getValue(), 0xFE);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADC, 0x03).is_error());
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x01);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcSetZeroAndCarry)
{
    system.bus().clearFlags();
    system.bus().setFlag(SystemBus::C);
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0xFF).is_error());
    ASSERT_EQ(lhs->getValue(), 0xFF);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADC, 0x00).is_error());
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcSetCarry)
{
    system.bus().clearFlags();
    system.bus().setFlag(SystemBus::C);
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0xFE).is_error());
    ASSERT_EQ(lhs->getValue(), 0xFE);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::ADC, 0x03).is_error());
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x02);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, sub)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x14).is_error());
    ASSERT_EQ(lhs->getValue(), 0x14);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SUB, 0x0F).is_error());
    ASSERT_EQ(alu->getValue(), 0x0F);
    ASSERT_EQ(lhs->getValue(), 0x05);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, subSetOverflowPosNeg)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, (byte)100).is_error());
    ASSERT_EQ(lhs->getValue(), 0x64);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SUB, (byte)-33).is_error());
    ASSERT_EQ(alu->getValue(), 0xDF);
    ASSERT_EQ(lhs->getValue(), 0x85);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, subSetOverflowNegPos)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, (byte)-100).is_error());
    ASSERT_EQ(lhs->getValue(), 0x9C);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SUB, (byte)33).is_error());
    ASSERT_EQ(alu->getValue(), 0x21);
    ASSERT_EQ(lhs->getValue(), 0x7B);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, sbbNoCarry)
{
    system.bus().setFlag(SystemBus::C, false);
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x14).is_error());
    ASSERT_EQ(lhs->getValue(), 0x14);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SBB, 0x0F).is_error());
    ASSERT_EQ(alu->getValue(), 0x0F);
    ASSERT_EQ(lhs->getValue(), 0x14 - 0x0F);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, sbbWithCarry)
{
    system.bus().setFlag(SystemBus::C, true);
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x14).is_error());
    ASSERT_EQ(lhs->getValue(), 0x14);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SBB, 0x0F).is_error());
    ASSERT_EQ(alu->getValue(), 0x0F);
    ASSERT_EQ(lhs->getValue(), 0x14 - 0x0F - 1);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, inc)
{
    system.bus().clearFlags();
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::INC, 0x03).is_error());
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x04);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, incSetZero)
{
    system.bus().clearFlags();
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::INC, 0xFF).is_error());
    ASSERT_EQ(alu->getValue(), 0xFF);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, dec)
{
    system.bus().clearFlags();
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::DEC, 0x03).is_error());
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x02);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, decSetZero)
{
    system.bus().clearFlags();
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::DEC, 0x01).is_error());
    ASSERT_EQ(alu->getValue(), 0x01);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseAnd)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0b00011111).is_error());
    ASSERT_EQ(lhs->getValue(), 0x1F);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::AND, 0b11111000).is_error());
    ASSERT_EQ(alu->getValue(), 0xF8);
    ASSERT_EQ(lhs->getValue(), 0x18);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseAndSelf)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x55).is_error());
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::AND, 0x55).is_error());
    ASSERT_EQ(alu->getValue(), 0x55);
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseAndZero)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x55).is_error());
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::AND, 0x00).is_error());
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system.bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseOr)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0b00101010).is_error());
    ASSERT_EQ(lhs->getValue(), 0x2A);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::OR, 0b00011100).is_error());
    ASSERT_EQ(alu->getValue(), 0x1C);
    ASSERT_EQ(lhs->getValue(), 0b00111110);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseOrZero)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x55).is_error());
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::OR, 0x00).is_error());
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseXor)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0b00101010).is_error());
    ASSERT_EQ(lhs->getValue(), 0x2A);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::XOR, 0b00011100).is_error());
    ASSERT_EQ(alu->getValue(), 0x1C);
    ASSERT_EQ(lhs->getValue(), 0b00110110 /* 0x36 */);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseXorSelf)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, LHS, 0x0, 0x55).is_error());
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::XOR, 0x55).is_error());
    ASSERT_EQ(alu->getValue(), 0x55);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system.bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseNot)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::NOT, 0b00011100).is_error());
    ASSERT_EQ(alu->getValue(), 0x1C);
    ASSERT_EQ(lhs->getValue(), 0b11100011 /* 0x36 */);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shl)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SHL, 0b01010101).is_error());
    ASSERT_EQ(alu->getValue(), 0x55);
    ASSERT_EQ(lhs->getValue(), 0b10101010 /* 0xAA */);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shlSetCarry)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SHL, 0b10101010).is_error());
    ASSERT_EQ(alu->getValue(), 0b10101010);
    ASSERT_EQ(lhs->getValue(), 0b01010100);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shr)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SHR, 0b10101010).is_error());
    ASSERT_EQ(alu->getValue(), 0xAA);
    ASSERT_EQ(lhs->getValue(), 0b01010101 /* 0x55 */);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shrSetCarry)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, RHS, ALU::SHR, 0b01010101).is_error());
    ASSERT_EQ(alu->getValue(), 0b01010101);
    ASSERT_EQ(lhs->getValue(), 0b00101010);
    ASSERT_FALSE(system.bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system.bus().isSet(SystemBus::C));
    ASSERT_FALSE(system.bus().isSet(SystemBus::V));
}

// TEST_F(ALUTest, clr) {
//   system -> cycle(false, true, true,1, RHS, 0xA, 0b10101010);
//   ASSERT_EQ(alu -> getValue(), 0xAA);
//   ASSERT_EQ(lhs -> getValue(), 0x00);
//   ASSERT_TRUE(system -> bus().isSet(SystemBus::Z));
//   ASSERT_FALSE(system -> bus().isSet(SystemBus::C));
//   ASSERT_FALSE(system -> bus().isSet(SystemBus::V));
// }

}
