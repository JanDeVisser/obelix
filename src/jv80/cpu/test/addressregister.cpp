/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <gtest/gtest.h>

#include <jv80/cpu/addressregister.h>
#include <jv80/cpu/harness.h>

namespace Obelix::JV80::CPU {

static int REGID = 0xC;

class AddressRegisterTest : public ::testing::Test {
protected:
    Harness* system = nullptr;
    AddressRegister* reg = nullptr;

    void SetUp() override
    {
        reg = new AddressRegister(REGID, "TEST");
        system = new Harness(reg);
    }

    void TearDown() override
    {
        delete system;
    }
};

TEST_F(AddressRegisterTest, canPutLSB)
{
    reg->setValue(0x5555);
    system->cycle(false, true, true, 1, REGID, 0, 0x42);
    ASSERT_EQ(reg->getValue(), 0x5542);
}

TEST_F(AddressRegisterTest, canPutMSB)
{
    reg->setValue(0x5555);
    system->cycle(false, true, true, 1, REGID, SystemBus::MSB, 0x42);
    ASSERT_EQ(reg->getValue(), 0x4255);
}

TEST_F(AddressRegisterTest, canPutLSBThenMSB)
{
    reg->setValue(0x5555);
    system->cycle(false, true, true, 1, REGID, 0, 0x37);
    system->cycle(false, true, true, 1, REGID, SystemBus::MSB, 0x42);
    ASSERT_EQ(reg->getValue(), 0x4237);
}

TEST_F(AddressRegisterTest, canPutAddr)
{
    reg->setValue(0x5555);
    system->cycle(true, false, true, 1, REGID, 0, 0x42, 0x37);
    ASSERT_EQ(reg->getValue(), 0x3742);
}

TEST_F(AddressRegisterTest, canGetAddr)
{
    reg->setValue(0x4237);
    system->cycle(true, false, true, REGID, 1, 0, 0x72);
    ASSERT_EQ(system->bus().readDataBus(), 0x37);
    ASSERT_EQ(system->bus().readAddrBus(), 0x42);
}

TEST_F(AddressRegisterTest, canGetLSB)
{
    reg->setValue(0x4237);
    system->cycle(false, true, true, REGID, 1, 0, 0x72);
    ASSERT_EQ(system->bus().readDataBus(), 0x37);
}

TEST_F(AddressRegisterTest, canGetMSB)
{
    reg->setValue(0x4237);
    system->cycle(false, true, true, REGID, 1, SystemBus::MSB, 0x72);
    ASSERT_EQ(system->bus().readDataBus(), 0x42);
}

TEST_F(AddressRegisterTest, dontPutWhenOtherRegAddressed)
{
    reg->setValue(0x5555);
    system->cycle(false, true, true, 1, 2, 0, 0x42);
    ASSERT_EQ(reg->getValue(), 0x5555);
}

TEST_F(AddressRegisterTest, dontGetWhenOtherRegAddressed)
{
    reg->setValue(0x5555);
    system->cycle(false, true, true, 2, 1, 0, 0x37);
    ASSERT_EQ(system->bus().readDataBus(), 0x37);
}

}
