/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <vector>

#include <jv80/cpu/clock.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/memory.h>
#include <jv80/cpu/systembus.h>
#include <jv80/cpu/ComponentContainer.h>

namespace Obelix::JV80::CPU {

class BackPlane : public ComponentContainer {
public:
    enum ClockPhase {
        SystemClock = 0x00,
        IOClock = 0x01,
    };

    BackPlane();
    ~BackPlane() override = default;
    SystemError run(word = 0x0000);
    void stop() { clock.stop(); }
    SystemBus::RunMode runMode();
    void setRunMode(SystemBus::RunMode runMode);
    [[nodiscard]] std::shared_ptr<Controller> controller() const;
    [[nodiscard]] std::shared_ptr<Memory> memory() const;
    SystemError loadImage(word, const byte*, word addr = 0, bool writable = true);

    [[nodiscard]] std::string to_string() const override;
    SystemError reset() override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;
    SystemError onFallingClockEdge() override;
    SystemError onLowClock() override;

    bool setClockSpeed(double);
    double clockSpeed();

    void defaultSetup();

private:
    Clock clock;
    ClockPhase m_phase = SystemClock;

    SystemError onClockEvent(const ComponentHandler&);
};

}
