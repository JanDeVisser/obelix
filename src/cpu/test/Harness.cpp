/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include "cpu/harness.h"
#include "cpu/iochannel.h"

#include "cpu/microcode.h"

#include "controllertest.h"

namespace Obelix::CPU {

TEST(Harness, CreateHarness)
{
    Harness system;
    ASSERT_EQ(system.to_string(), "Harness");
}

TEST_F(HarnessTest, CreateHarness)
{
    ASSERT_EQ(system.to_string(), "Harness");
}

TEST_F(HarnessTest, InsertRegister)
{
    gp_d = std::make_shared<Register>(0x3);
    system.insert(gp_d);

    auto gp = system.component<Register>(0x3);
    ASSERT_NE(gp, nullptr);
}

byte nop[] = {
    /* 8000 */ NOP,
    /* 8001 */ HLT
};

TEST_F(HarnessTest, Nop)
{
    mem->initialize(ROM_START, 2, nop);
    check_memory(START_VECTOR, NOP);

    pc->setValue(START_VECTOR);
    ASSERT_EQ(pc->getValue(), START_VECTOR);

    check_cycles(5);
}

}
