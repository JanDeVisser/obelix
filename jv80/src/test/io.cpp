/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cpu/harness.h"
#include "cpu/iochannel.h"
#include "cpu/register.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>

static int REGID = 0xC;
static int CHANNEL_IN = 0x3;
static int CHANNEL_OUT = 0x5;

class IOTest : public ::testing::Test {
protected:
    Harness* system = nullptr;
    Register* reg;
    IOChannel* channelIn = nullptr;
    IOChannel* channelOut = nullptr;
    byte inValue;
    byte outValue;

    void SetUp() override
    {
        channelIn = new IOChannel(CHANNEL_IN, "IN", [this]() {
            return inValue;
        });
        channelOut = new IOChannel(CHANNEL_OUT, "OUT", [this](byte v) {
            outValue = v;
        });
        reg = new Register(REGID, "REG");
        system = new Harness(reg);
        system->insertIO(channelIn);
        system->insertIO(channelOut);
    }

    void TearDown() override
    {
        delete system;
    }
};

TEST_F(IOTest, canSend)
{
    reg->setValue(0x42);
    system->cycle(true, true, false, REGID, CHANNEL_OUT, SystemBus::IOOut, 0x37);
    ASSERT_EQ(outValue, 0x42);
}

TEST_F(IOTest, canReceive)
{
    inValue = 0x42;
    reg->setValue(0x37);
    system->cycle(false, true, true, REGID, CHANNEL_IN, SystemBus::IOIn, 0x39);
}

//
// TEST_F(RegisterTest, dontPutWhenOtherRegAddressed) {
//  reg -> setValue(0x37);
//  system -> cycle(false, true, true, 1, 2, 0, 0x42);
//  ASSERT_EQ(reg -> getValue(), 0x37);
//}
//
// TEST_F(RegisterTest, dontGetWhenOtherRegAddressed) {
//  reg -> setValue(0x42);
//  system -> cycle(false, true, true, 2, 1, 0, 0x37);
//  ASSERT_EQ(system->bus().readDataBus(), 0x37);
//}
