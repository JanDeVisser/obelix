/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <jv80/cpu/register.h>

namespace Obelix::JV80::CPU {

Register::Register(int registerID, std::string&& name)
    : ConnectedComponent(registerID,
        (!name.empty()) ? name : std::string(1, 'A' + registerID))
{
}

void Register::setValue(byte val)
{
    m_value = val;
    sendEvent(EV_VALUECHANGED);
}

std::string Register::to_string() const
{
    return format("{01x}. {}  {02x}", address(), name().c_str(), m_value);
}

SystemError Register::reset()
{
    m_value = 0;
    m_stash = 0;
    sendEvent(EV_VALUECHANGED);
    return {};
}

SystemError Register::onRisingClockEdge()
{
    if ((!bus()->xdata() || (!bus()->io() && (bus()->opflags() & SystemBus::IOOut))) && (bus()->getAddress() == address())) {
        bus()->putOnDataBus(m_value);
        return {};
    }
    if (!bus()->xaddr() && (bus()->getAddress() == address())) {
        bus()->putOnDataBus(m_stash & 0x00FF);
        bus()->putOnAddrBus((m_stash & 0xFF00) >> 8);
    }
    return {};
}

SystemError Register::onHighClock()
{
    if ((!bus()->xdata() && (bus()->putAddress() == address())) || (!bus()->io() && (bus()->opflags() & SystemBus::IOIn) && (bus()->getAddress() == address()))) {
        setValue(bus()->readDataBus());
        return {};
    }
    if (!bus()->xaddr() && (bus()->putAddress() == address())) {
        m_stash = (word)((bus()->readAddrBus() << 8) | bus()->readDataBus());
        return {};
    }
    return {};
}

}
