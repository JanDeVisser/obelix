/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <jv80/cpu/component.h>

namespace Obelix::JV80::CPU {

enum class ClockCycleEvent {
    RisingEdge,
    High,
    FallingEdge,
    Low,
};

class ClockListener {
public:
    enum class ClockEvent {
        Started,
        Stopped,
        Error,
        FreqChange,
    };

    virtual void clockEvent(ClockEvent event) = 0;
};

class Clock {
public:
    enum class State {
        Running,
        Stopped
    };

    Clock(Component* owner, double speed_khz)
        : m_khz(speed_khz)
        , m_owner(owner)
    {
    }

    virtual ~Clock() = default;

    [[nodiscard]] double frequency() const { return m_khz; }
    [[nodiscard]] unsigned long tick() const;

    SystemError start();

    void stop();

    bool setSpeed(double);

    std::shared_ptr<ClockListener> const& setListener(std::shared_ptr<ClockListener> const&);

private:
    double m_khz;
    Component* m_owner;
    State m_state = State::Stopped;
    std::shared_ptr<ClockListener> m_listener = nullptr;

    void sendEvent(ClockListener::ClockEvent);
    void sleep() const;
};

}
