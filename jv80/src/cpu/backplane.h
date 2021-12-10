/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cpu/clock.h>
#include <cpu/controller.h>
#include <cpu/memory.h>
#include <cpu/systembus.h>
#include <functional>
#include <vector>

namespace Obelix::JV80::CPU {

class BackPlane : public ComponentContainer {
private:
    enum ClockPhase {
        SystemClock = 0x00,
        IOClock = 0x01,
    };
    Clock clock;
    ClockPhase m_phase = SystemClock;
    std::ostream* m_output = nullptr;

    SystemError onClockEvent(const ComponentHandler&);

protected:
    SystemError reportError() override;

public:
    BackPlane();
    ~BackPlane() override = default;
    void run(word = 0x0000);
    void stop() { clock.stop(); }
    SystemBus::RunMode runMode();
    void setRunMode(SystemBus::RunMode runMode);
    Controller* controller() const;
    Memory* memory() const;
    void loadImage(word, const byte*, word addr = 0, bool writable = true);
    void setOutputStream(std::ostream& os) { m_output = &os; }

    std::ostream& status(std::ostream&) override;
    SystemError reset() override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;
    SystemError onFallingClockEdge() override;
    SystemError onLowClock() override;

    bool setClockSpeed(double);
    double clockSpeed();

    void defaultSetup();
};

}
