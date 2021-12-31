/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <chrono>
#include <iostream>

#include <gtest/gtest.h>

#include "cpu/clock.h"

namespace Obelix::CPU {

class TestSystem : public Component {
protected:
public:
    Clock clock;
    int max_cycles = -1;
    int cycles = 0;
    int state = 0;

    TestSystem()
        : clock(this, 1)
    {
    }

    [[nodiscard]] std::string to_string() const override
    {
        return "Clock Test System";
    }

    SystemError reset() override
    {
        cycles = 0;
        state = 0;
        return {};
    }

    SystemError onRisingClockEdge() override
    {
        if (state != 0) {
            return SystemError { SystemErrorCode::GeneralError };
        }
        state = 1;
        return {};
    }

    SystemError onHighClock() override
    {
        if (state != 1) {
            return SystemError { SystemErrorCode::GeneralError };
        }
        state = 2;
        return {};
    }

    SystemError onFallingClockEdge() override
    {
        if (state != 2) {
            return SystemError { SystemErrorCode::GeneralError };
        }
        state = 3;
        return {};
    }

    SystemError onLowClock() override
    {
        if (state != 3) {
            return SystemError { SystemErrorCode::GeneralError };
        }
        state = 0;
        cycles++;
        if (max_cycles > 0 && (cycles >= max_cycles)) {
            clock.stop();
        }
        return {};
    }
};

class ClockTest : public ::testing::Test {
protected:
    TestSystem system;
    void SetUp() override
    {
        ASSERT_FALSE(system.reset().is_error());
    }
};

TEST_F(ClockTest, canStart)
{
    system.max_cycles = 1;
    ASSERT_EQ(system.clock.tick(), 500000);
    auto err = system.clock.start();
    ASSERT_FALSE(err.is_error());
    ASSERT_EQ(system.cycles, 1);
}

TEST_F(ClockTest, ticksAreAccurate)
{
    system.max_cycles = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    auto err = system.clock.start();
    auto finish = std::chrono::high_resolution_clock::now();
    ASSERT_FALSE(err.is_error());
    ASSERT_EQ(system.cycles, 1000);
    ASSERT_NEAR(std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count(), 1000, 500);
}

}
