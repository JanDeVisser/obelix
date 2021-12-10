/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cpu/component.h>

namespace Obelix::JV80::CPU {

enum ClockCycleEvent {
    RisingEdge,
    High,
    FallingEdge,
    Low,
};

class ClockListener {
public:
    enum ClockEvent {
        Started,
        Stopped,
        Error,
        FreqChange,
    };

    virtual void clockEvent(ClockEvent event) = 0;
};

class Clock {
public:
    enum State {
        Running,
        Stopped
    };

private:
    double khz;
    Component* owner;
    State state = Stopped;
    ClockListener* m_listener = nullptr;

    void sendEvent(ClockListener::ClockEvent);
    void sleep() const;

public:
    Clock(Component* o, double speed_khz)
        : khz(speed_khz)
        , owner(o)
    {
    }

    virtual ~Clock() = default;

    double frequency() { return khz; }

    unsigned long tick() const;

    SystemError start();

    void stop();

    bool setSpeed(double);

    ClockListener* setListener(ClockListener*);
};

}
