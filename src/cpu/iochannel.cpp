/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "iochannel.h"

namespace Obelix::CPU {

IOChannel::IOChannel(int channelID, std::string name, Input input)
    : ConnectedComponent(channelID, move(name))
    , m_input(std::move(input))
{
}

IOChannel::IOChannel(int channelID, std::string name, Output output)
    : ConnectedComponent(channelID, move(name))
    , m_output(std::move(output))
{
}

void IOChannel::setValue(byte val)
{
    if (m_output) {
        m_output(val);
        sendEvent(EV_OUTPUTWRITTEN);
    }
}

int IOChannel::getValue() const
{
    int ret = 0;
    if (m_input) {
        ret = m_input();
        if (ret) {
            sendEvent(EV_INPUTREAD);
        }
    }
    return ret;
}

std::string IOChannel::to_string() const
{
    auto ret = format("#%{01x}. {} ", address(), name());
    //    if (m_status) {
    //        ret += m_status();
    //    }
    return ret;
}

SystemError IOChannel::reset()
{
    return {};
}

SystemError IOChannel::onRisingClockEdge()
{
    if (!bus()->io() && (bus()->putAddress() == address()) && (bus()->opflags() & SystemBus::IOIn)) {
        bus()->putOnDataBus(getValue());
    }
    return {};
}

SystemError IOChannel::onHighClock()
{
    if (!bus()->io() && (bus()->putAddress() == address()) && (bus()->opflags() & SystemBus::IOOut)) {
        setValue(bus()->readDataBus());
    }
    return {};
}

}