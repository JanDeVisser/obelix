/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <jv80/cpu/iochannel.h>

namespace Obelix::JV80::CPU {

IOChannel::IOChannel(int channelID, std::string&& name, Input& input)
    : ConnectedComponent(channelID, name)
    , m_input(std::move(input))
{
}

IOChannel::IOChannel(int channelID, std::string&& name, Output& output)
    : ConnectedComponent(channelID, name)
    , m_output(std::move(output))
{
}

IOChannel::IOChannel(int channelID, std::string&& name, Input&& input)
    : ConnectedComponent(channelID, name)
    , m_input(input)
{
}

IOChannel::IOChannel(int channelID, std::string&& name, Output&& output)
    : ConnectedComponent(channelID, name)
    , m_output(output)
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

std::ostream& IOChannel::status(std::ostream& os)
{
    char buf[80];
    snprintf(buf, 80, "#%1x. %s ", id(), name().c_str());
    if (m_status) {
        m_status(os);
    }
    os << buf << std::endl;
    return os;
}

SystemError IOChannel::reset()
{
    return NoError;
}

SystemError IOChannel::onRisingClockEdge()
{
    if (!bus()->io() && (bus()->putID() == id()) && (bus()->opflags() & SystemBus::IOIn)) {
        bus()->putOnDataBus(getValue());
    }
    return NoError;
}

SystemError IOChannel::onHighClock()
{
    if (!bus()->io() && (bus()->putID() == id()) && (bus()->opflags() & SystemBus::IOOut)) {
        setValue(bus()->readDataBus());
    }
    return NoError;
}
}
