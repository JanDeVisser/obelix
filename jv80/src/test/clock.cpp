/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "cpu/clock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <iostream>

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

    std::ostream& status(std::ostream& os) override
    {
        //    std::cout << "status" << std::endl;
        return os;
    }

    SystemError reset() override
    {
        //    std::cout << "reset" << std::endl;
        cycles = 0;
        state = 0;
        return NoError;
    }

    SystemError onRisingClockEdge() override
    {
        //    std::cout << "onRisingClockEdge" << std::endl;
        if (state != 0) {
            return GeneralError;
        }
        state = 1;
        return NoError;
    }

    SystemError onHighClock() override
    {
        //    std::cout << "onHighClock" << std::endl;
        if (state != 1) {
            return GeneralError;
        }
        state = 2;
        return NoError;
    }

    SystemError onFallingClockEdge() override
    {
        //    std::cout << "onFallingClockEdge" << std::endl;
        if (state != 2) {
            return GeneralError;
        }
        state = 3;
        return NoError;
    }

    SystemError onLowClock() override
    {
        //    std::cout << "onLowClock" << std::endl;
        if (state != 3) {
            return GeneralError;
        }
        state = 0;
        cycles++;
        if (max_cycles > 0 && (cycles >= max_cycles)) {
            clock.stop();
        }
        return NoError;
    }
};

class ClockTest : public ::testing::Test {
protected:
    TestSystem* system = nullptr;
    void SetUp() override
    {
        system = new TestSystem();
        system->reset();
    }

    void TearDown() override
    {
        delete system;
    }
};

TEST_F(ClockTest, canStart)
{
    system->max_cycles = 1;
    ASSERT_EQ(system->clock.tick(), 500000);
    SystemError err = system->clock.start();
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(system->cycles, 1);
}

TEST_F(ClockTest, ticksAreAccurate)
{
    system->max_cycles = 1000;
    auto start = std::chrono::high_resolution_clock::now();
    SystemError err = system->clock.start();
    auto finish = std::chrono::high_resolution_clock::now();
    ASSERT_EQ(err, NoError);
    ASSERT_EQ(system->cycles, 1000);
    ASSERT_NEAR(std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count(), 1000, 500);
}
