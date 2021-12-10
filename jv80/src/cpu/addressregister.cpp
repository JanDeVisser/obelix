/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <cpu/addressregister.h>

namespace Obelix::JV80::CPU {

AddressRegister::AddressRegister(int registerID, std::string n)
    : ConnectedComponent(registerID, n)
{
}

void AddressRegister::setValue(word val)
{
    value = val;
    sendEvent(EV_VALUECHANGED);
}

std::ostream& AddressRegister::status(std::ostream& os)
{
    char buf[80];
    snprintf(buf, 80, "%1x. %2s  %04x\n", id(), name().c_str(), value);
    os << buf;
    return os;
}

SystemError AddressRegister::reset()
{
    value = 0;
    sendEvent(EV_VALUECHANGED);
    return NoError;
}

SystemError AddressRegister::onRisingClockEdge()
{
    if (bus()->getID() == id()) {
        if (!bus()->xdata()) {
            if (bus()->opflags() & SystemBus::MSB) {
                bus()->putOnDataBus((value & 0xFF00) >> 8);
            } else {
                bus()->putOnDataBus(value & 0x00FF);
            }
        } else if (!bus()->xaddr()) {
            if (bus()->opflags() & SystemBus::Dec) {
                setValue(value - (word)1);
                if (bus()->opflags() & SystemBus::Flags) {
                    bus()->clearFlags();
                    if (value == 0x0000) {
                        bus()->setFlag(SystemBus::Z);
                    }
                }
            }
            bus()->putOnDataBus(value & 0x00FF);
            bus()->putOnAddrBus((value & 0xFF00) >> 8);
            if (bus()->opflags() & SystemBus::Inc) {
                setValue(value + (word)1);
                if (bus()->opflags() & SystemBus::Flags) {
                    bus()->clearFlags();
                    if (value == 0x0000) {
                        bus()->setFlag(SystemBus::Z);
                        bus()->setFlag(SystemBus::C);
                    }
                }
            }
        }
    }
    return NoError;
}

SystemError AddressRegister::onHighClock()
{
    if (!bus()->xdata() && (bus()->putID() == id())) {
        if (!(bus()->opflags() & SystemBus::MSB)) {
            value &= 0xFF00;
            setValue(value | bus()->readDataBus());
        } else {
            value &= 0x00FF;
            setValue(value | ((word)bus()->readDataBus()) << 8);
        }
    } else if (!bus()->xaddr() && (bus()->putID() == id())) {
        setValue((word)((bus()->readAddrBus() << 8) | bus()->readDataBus()));
    }
    return NoError;
}

}