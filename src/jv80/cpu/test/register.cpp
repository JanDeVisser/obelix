/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <gtest/gtest.h>

#include <jv80/cpu/harness.h>
#include <jv80/cpu/register.h>

namespace Obelix::JV80::CPU {

static int REGID = 0xC;

class RegisterTest : public ::testing::Test {
protected:
    Harness system;
    std::shared_ptr<Register> reg = nullptr;

    void SetUp() override
    {
        reg = std::make_shared<Register>(REGID);
        system.insert(reg);
    }
};

TEST_F(RegisterTest, canPut)
{
    ASSERT_FALSE(system.cycle(false, true, true, 1, REGID, 0, 0x42).is_error());
    ASSERT_EQ(reg->getValue(), 0x42);
}

TEST_F(RegisterTest, canGet)
{
    reg->setValue(0x42);
    ASSERT_FALSE(system.cycle(false, true, true, REGID, 1, 0, 0x37).is_error());
    ASSERT_EQ(system.bus().readDataBus(), 0x42);
}

TEST_F(RegisterTest, dontPutWhenOtherRegAddressed)
{
    reg->setValue(0x37);
    ASSERT_FALSE(system.cycle(false, true, true, 1, 2, 0, 0x42).is_error());
    ASSERT_EQ(reg->getValue(), 0x37);
}

TEST_F(RegisterTest, dontGetWhenOtherRegAddressed)
{
    reg->setValue(0x42);
    ASSERT_FALSE(system.cycle(false, true, true, 2, 1, 0, 0x37).is_error());
    ASSERT_EQ(system.bus().readDataBus(), 0x37);
}

}
