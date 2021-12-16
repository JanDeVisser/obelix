/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <gtest/gtest.h>

#include <jv80/cpu/alu.h>
#include <jv80/cpu/harness.h>

static int LHS = 0x4;
static int RHS = 0x5;

namespace Obelix::JV80::CPU {

class ALUTest : public ::testing::Test {
protected:
    Harness* system = nullptr;
    Register* lhs = new Register(LHS);
    ALU* alu = new ALU(0x5, lhs);

    void SetUp() override
    {
        system = new Harness();
        system->insert(lhs);
        system->insert(alu);
    }

    void TearDown() override
    {
        delete system;
    }
};

TEST_F(ALUTest, add)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0x03);
    ASSERT_EQ(lhs->getValue(), 0x03);
    system->cycle(false, true, true, 1, RHS, ALU::ADD, 0x02);
    ASSERT_EQ(alu->getValue(), 0x02);
    ASSERT_EQ(lhs->getValue(), 0x05);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, addSetZero)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0x00);
    ASSERT_EQ(lhs->getValue(), 0x00);
    system->cycle(false, true, true, 1, RHS, ALU::ADD, 0x00);
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, addSetCarry)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0xFE);
    ASSERT_EQ(lhs->getValue(), 0xFE);
    system->cycle(false, true, true, 1, RHS, ALU::ADD, 0x03);
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x01);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
}

TEST_F(ALUTest, addSetOverflowPosPos)
{
    system->cycle(false, true, true, 1, LHS, 0x0, (byte)80);
    ASSERT_EQ(lhs->getValue(), 0x50);
    system->cycle(false, true, true, 1, RHS, ALU::ADD, (byte)80);
    ASSERT_EQ(alu->getValue(), 0x50);
    ASSERT_EQ(lhs->getValue(), 0xA0);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, addSetOverflowNegNeg)
{
    system->cycle(false, true, true, 1, LHS, 0x0, (byte)-80);
    ASSERT_EQ(lhs->getValue(), 0xB0);
    system->cycle(false, true, true, 1, RHS, ALU::ADD, (byte)-80);
    ASSERT_EQ(alu->getValue(), 0xB0);
    ASSERT_EQ(lhs->getValue(), 0x60);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adc)
{
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    system->cycle(false, true, true, 1, LHS, 0x0, 0x03);
    ASSERT_EQ(lhs->getValue(), 0x03);
    system->cycle(false, true, true, 1, RHS, ALU::ADC, 0x02);
    ASSERT_EQ(alu->getValue(), 0x02);
    ASSERT_EQ(lhs->getValue(), 0x06);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcNoCarry)
{
    system->bus().clearFlags();
    system->cycle(false, true, true, 1, LHS, 0x0, 0x03);
    ASSERT_EQ(lhs->getValue(), 0x03);
    system->cycle(false, true, true, 1, RHS, ALU::ADC, 0x02);
    ASSERT_EQ(alu->getValue(), 0x02);
    ASSERT_EQ(lhs->getValue(), 0x05);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcNoCarrySetCarry)
{
    system->bus().clearFlags();
    system->cycle(false, true, true, 1, LHS, 0x0, 0xFE);
    ASSERT_EQ(lhs->getValue(), 0xFE);
    system->cycle(false, true, true, 1, RHS, ALU::ADC, 0x03);
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x01);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcSetZeroAndCarry)
{
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    system->cycle(false, true, true, 1, LHS, 0x0, 0xFF);
    ASSERT_EQ(lhs->getValue(), 0xFF);
    system->cycle(false, true, true, 1, RHS, ALU::ADC, 0x00);
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, adcSetCarry)
{
    system->bus().clearFlags();
    system->bus().setFlag(SystemBus::C);
    system->cycle(false, true, true, 1, LHS, 0x0, 0xFE);
    ASSERT_EQ(lhs->getValue(), 0xFE);
    system->cycle(false, true, true, 1, RHS, ALU::ADC, 0x03);
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x02);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, sub)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0x14);
    ASSERT_EQ(lhs->getValue(), 0x14);
    system->cycle(false, true, true, 1, RHS, ALU::SUB, 0x0F);
    ASSERT_EQ(alu->getValue(), 0x0F);
    ASSERT_EQ(lhs->getValue(), 0x05);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, subSetOverflowPosNeg)
{
    system->cycle(false, true, true, 1, LHS, 0x0, (byte)100);
    ASSERT_EQ(lhs->getValue(), 0x64);
    system->cycle(false, true, true, 1, RHS, ALU::SUB, (byte)-33);
    ASSERT_EQ(alu->getValue(), 0xDF);
    ASSERT_EQ(lhs->getValue(), 0x85);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, subSetOverflowNegPos)
{
    system->cycle(false, true, true, 1, LHS, 0x0, (byte)-100);
    ASSERT_EQ(lhs->getValue(), 0x9C);
    system->cycle(false, true, true, 1, RHS, ALU::SUB, (byte)33);
    ASSERT_EQ(alu->getValue(), 0x21);
    ASSERT_EQ(lhs->getValue(), 0x7B);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, sbbNoCarry)
{
    system->bus().setFlag(SystemBus::C, false);
    system->cycle(false, true, true, 1, LHS, 0x0, 0x14);
    ASSERT_EQ(lhs->getValue(), 0x14);
    system->cycle(false, true, true, 1, RHS, ALU::SBB, 0x0F);
    ASSERT_EQ(alu->getValue(), 0x0F);
    ASSERT_EQ(lhs->getValue(), 0x14 - 0x0F);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, sbbWithCarry)
{
    system->bus().setFlag(SystemBus::C, true);
    system->cycle(false, true, true, 1, LHS, 0x0, 0x14);
    ASSERT_EQ(lhs->getValue(), 0x14);
    system->cycle(false, true, true, 1, RHS, ALU::SBB, 0x0F);
    ASSERT_EQ(alu->getValue(), 0x0F);
    ASSERT_EQ(lhs->getValue(), 0x14 - 0x0F - 1);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, inc)
{
    system->bus().clearFlags();
    system->cycle(false, true, true, 1, RHS, ALU::INC, 0x03);
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x04);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, incSetZero)
{
    system->bus().clearFlags();
    system->cycle(false, true, true, 1, RHS, ALU::INC, 0xFF);
    ASSERT_EQ(alu->getValue(), 0xFF);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, dec)
{
    system->bus().clearFlags();
    system->cycle(false, true, true, 1, RHS, ALU::DEC, 0x03);
    ASSERT_EQ(alu->getValue(), 0x03);
    ASSERT_EQ(lhs->getValue(), 0x02);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, decSetZero)
{
    system->bus().clearFlags();
    system->cycle(false, true, true, 1, RHS, ALU::DEC, 0x01);
    ASSERT_EQ(alu->getValue(), 0x01);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseAnd)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0b00011111);
    ASSERT_EQ(lhs->getValue(), 0x1F);
    system->cycle(false, true, true, 1, RHS, ALU::AND, 0b11111000);
    ASSERT_EQ(alu->getValue(), 0xF8);
    ASSERT_EQ(lhs->getValue(), 0x18);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseAndSelf)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0x55);
    ASSERT_EQ(lhs->getValue(), 0x55);
    system->cycle(false, true, true, 1, RHS, ALU::AND, 0x55);
    ASSERT_EQ(alu->getValue(), 0x55);
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseAndZero)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0x55);
    ASSERT_EQ(lhs->getValue(), 0x55);
    system->cycle(false, true, true, 1, RHS, ALU::AND, 0x00);
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseOr)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0b00101010);
    ASSERT_EQ(lhs->getValue(), 0x2A);
    system->cycle(false, true, true, 1, RHS, ALU::OR, 0b00011100);
    ASSERT_EQ(alu->getValue(), 0x1C);
    ASSERT_EQ(lhs->getValue(), 0b00111110);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseOrZero)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0x55);
    ASSERT_EQ(lhs->getValue(), 0x55);
    system->cycle(false, true, true, 1, RHS, ALU::OR, 0x00);
    ASSERT_EQ(alu->getValue(), 0x00);
    ASSERT_EQ(lhs->getValue(), 0x55);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseXor)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0b00101010);
    ASSERT_EQ(lhs->getValue(), 0x2A);
    system->cycle(false, true, true, 1, RHS, ALU::XOR, 0b00011100);
    ASSERT_EQ(alu->getValue(), 0x1C);
    ASSERT_EQ(lhs->getValue(), 0b00110110 /* 0x36 */);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, bitwiseXorSelf)
{
    system->cycle(false, true, true, 1, LHS, 0x0, 0x55);
    ASSERT_EQ(lhs->getValue(), 0x55);
    system->cycle(false, true, true, 1, RHS, ALU::XOR, 0x55);
    ASSERT_EQ(alu->getValue(), 0x55);
    ASSERT_EQ(lhs->getValue(), 0x00);
    ASSERT_TRUE(system->bus().isSet(SystemBus::Z));
}

TEST_F(ALUTest, bitwiseNot)
{
    system->cycle(false, true, true, 1, RHS, ALU::NOT, 0b00011100);
    ASSERT_EQ(alu->getValue(), 0x1C);
    ASSERT_EQ(lhs->getValue(), 0b11100011 /* 0x36 */);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shl)
{
    system->cycle(false, true, true, 1, RHS, ALU::SHL, 0b01010101);
    ASSERT_EQ(alu->getValue(), 0x55);
    ASSERT_EQ(lhs->getValue(), 0b10101010 /* 0xAA */);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shlSetCarry)
{
    system->cycle(false, true, true, 1, RHS, ALU::SHL, 0b10101010);
    ASSERT_EQ(alu->getValue(), 0b10101010);
    ASSERT_EQ(lhs->getValue(), 0b01010100);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shr)
{
    system->cycle(false, true, true, 1, RHS, ALU::SHR, 0b10101010);
    ASSERT_EQ(alu->getValue(), 0xAA);
    ASSERT_EQ(lhs->getValue(), 0b01010101 /* 0x55 */);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_FALSE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
}

TEST_F(ALUTest, shrSetCarry)
{
    system->cycle(false, true, true, 1, RHS, ALU::SHR, 0b01010101);
    ASSERT_EQ(alu->getValue(), 0b01010101);
    ASSERT_EQ(lhs->getValue(), 0b00101010);
    ASSERT_FALSE(system->bus().isSet(SystemBus::Z));
    ASSERT_TRUE(system->bus().isSet(SystemBus::C));
    ASSERT_FALSE(system->bus().isSet(SystemBus::V));
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
