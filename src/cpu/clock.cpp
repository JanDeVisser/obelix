/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cmath>
#include <ctime>
#include <thread>

#include "clock.h"

namespace Obelix::CPU {

/**
 * @return Duration of a clock tick, in nanoseconds.
 */
unsigned long Clock::tick() const
{
    return (unsigned long)round(1000000.0 / (2.0 * m_khz));
}

std::shared_ptr<ClockListener> const& Clock::setListener(std::shared_ptr<ClockListener> const& listener)
{
    auto& old = m_listener;
    m_listener = listener;
    return old;
}

void Clock::sendEvent(ClockListener::ClockEvent event)
{
    if (m_listener) {
        m_listener->clockEvent(event);
    }
}

void Clock::sleep() const
{
    struct timespec req = { 0 };
    req.tv_sec = 0;
    req.tv_nsec = (long)tick();
    nanosleep(&req, (struct timespec*)nullptr);
}

SystemError Clock::start()
{
    SystemError err;
    sendEvent(ClockListener::ClockEvent::Started);
    for (m_state = State::Running; !err.is_error() && m_state == State::Running;) {
        if ((err = m_owner->onRisingClockEdge()).is_error()) {
            break;
        }
        if ((err = m_owner->onHighClock()).is_error()) {
            break;
        }
        sleep();
        if ((err = m_owner->onFallingClockEdge()).is_error()) {
            break;
        }
        if ((err = m_owner->onLowClock()).is_error()) {
            break;
        }
        sleep();
    }
    sendEvent(err.is_error() ? ClockListener::ClockEvent::Error : ClockListener::ClockEvent::Stopped);
    return err;
}

void Clock::stop()
{
    m_state = State::Stopped;
}

bool Clock::setSpeed(double freq)
{
    if ((freq > 0) && (freq <= 1000)) {
        m_khz = freq;
        sendEvent(ClockListener::ClockEvent::FreqChange);
        return true;
    } else {
        return false;
    }
}

}
