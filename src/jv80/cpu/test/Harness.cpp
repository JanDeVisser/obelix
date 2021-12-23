/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gtest/gtest.h>

#include <jv80/cpu/alu.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/harness.h>
#include <jv80/cpu/iochannel.h>
#include <jv80/cpu/memory.h>

#include <jv80/cpu/microcode.h>

#include "controllertest.h"

namespace Obelix::JV80::CPU {

TEST(Harness, CreateHarness)
{
    Harness system;
    ASSERT_EQ(system.to_string(), "Harness");
}

TEST_F(HarnessTest, CreateHarness)
{
    ASSERT_EQ(system.to_string(), "Harness");
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
