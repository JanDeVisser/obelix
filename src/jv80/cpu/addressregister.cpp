/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <core/Format.h>
#include <jv80/cpu/addressregister.h>

namespace Obelix::JV80::CPU {

AddressRegister::AddressRegister(int registerID, std::string n, bool xdata)
    : ConnectedComponent(registerID, move(n))
    , m_xdata(xdata)
{
}

void AddressRegister::setValue(word val)
{
    m_value = val;
    sendEvent(EV_VALUECHANGED);
}

std::string AddressRegister::to_string() const
{
    return format("{01x}. {02s}  {04x}\n", id(), name(), m_value);
}

SystemError AddressRegister::reset()
{
    m_value = 0;
    sendEvent(EV_VALUECHANGED);
    return {};
}

SystemError AddressRegister::onRisingClockEdge()
{
    if (bus()->getID() == id()) {
        if (m_xdata && !bus()->xdata()) {
            if (bus()->opflags() & SystemBus::MSB) {
                bus()->putOnDataBus((m_value & 0xFF00) >> 8);
            } else {
                bus()->putOnDataBus(m_value & 0x00FF);
            }
        } else if (!bus()->xaddr()) {
            if (bus()->opflags() & SystemBus::DEC) {
                setValue(m_value - (word)1);
                if (bus()->opflags() & SystemBus::Flags) {
                    bus()->clearFlags();
                    if (m_value == 0x0000) {
                        bus()->setFlag(SystemBus::Z);
                    }
                }
            }
            bus()->putOnDataBus(m_value & 0x00FF);
            bus()->putOnAddrBus((m_value & 0xFF00) >> 8);
            if (bus()->opflags() & SystemBus::INC) {
                setValue(m_value + (word)1);
                if (bus()->opflags() & SystemBus::Flags) {
                    bus()->clearFlags();
                    if (m_value == 0x0000) {
                        bus()->setFlag(SystemBus::Z);
                        bus()->setFlag(SystemBus::C);
                    }
                }
            }
        }
    }
    return {};
}

SystemError AddressRegister::onHighClock()
{
    if (m_xdata && !bus()->xdata() && (bus()->putID() == id())) {
        if (!(bus()->opflags() & SystemBus::MSB)) {
            m_value &= 0xFF00;
            setValue(m_value | bus()->readDataBus());
        } else {
            m_value &= 0x00FF;
            setValue(m_value | ((word)bus()->readDataBus()) << 8);
        }
    } else if (!bus()->xaddr() && (bus()->putID() == id())) {
        setValue((word)((bus()->readAddrBus() << 8) | bus()->readDataBus()));
    }
    return {};
}

}
