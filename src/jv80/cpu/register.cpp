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
    value = val;
    sendEvent(EV_VALUECHANGED);
}

std::string Register::to_string() const
{
    return format("{01x}. {}  {02x}", id(), name().c_str(), value);
}

SystemError Register::reset()
{
    value = 0;
    sendEvent(EV_VALUECHANGED);
    return {};
}

SystemError Register::onRisingClockEdge()
{
    if ((!bus()->xdata() || (!bus()->io() && (bus()->opflags() & SystemBus::IOOut))) && (bus()->getID() == id())) {
        bus()->putOnDataBus(value);
    }
    return {};
}

SystemError Register::onHighClock()
{
    if ((!bus()->xdata() && (bus()->putID() == id())) || (!bus()->io() && (bus()->opflags() & SystemBus::IOIn) && (bus()->getID() == id()))) {
        setValue(bus()->readDataBus());
    }
    return {};
}

}
