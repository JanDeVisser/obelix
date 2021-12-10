/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <cpu/register.h>

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

std::ostream& Register::status(std::ostream& os)
{
    char buf[80];
    snprintf(buf, 80, "%1x. %s  %02x\n", id(), name().c_str(), value);
    os << buf;
    return os;
}

SystemError Register::reset()
{
    value = 0;
    sendEvent(EV_VALUECHANGED);
    return NoError;
}

SystemError Register::onRisingClockEdge()
{
    if ((!bus()->xdata() || (!bus()->io() && (bus()->opflags() & SystemBus::IOOut))) && (bus()->getID() == id())) {
        bus()->putOnDataBus(value);
    }
    return NoError;
}

SystemError Register::onHighClock()
{
    if ((!bus()->xdata() && (bus()->putID() == id())) || (!bus()->io() && (bus()->opflags() & SystemBus::IOIn) && (bus()->getID() == id()))) {
        setValue(bus()->readDataBus());
    }
    return NoError;
}

}
