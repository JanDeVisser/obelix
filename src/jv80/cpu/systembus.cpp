/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Format.h>
#include <jv80/cpu/systembus.h>

namespace Obelix::JV80::CPU {

SystemBus::SystemBus(ComponentContainer* bp)
    : m_backplane(bp)
{
    _reset();
}

void SystemBus::_reset()
{
    _xdata = true;
    _xaddr = true;
    get = 0;
    put = 0;
    op = 0;
    data_bus = 0;
    addr_bus = 0;
    _sus = true;
    _nmi = true;
    rst = false;
    _io = true;
    _halt = true;
    m_flags = Clear;
}

SystemError SystemBus::reset()
{
    _reset();
    sendEvent(EV_VALUECHANGED);
    return {};
}

byte SystemBus::readDataBus() const
{
    return data_bus;
}

void SystemBus::putOnDataBus(byte value)
{
    data_bus = value;
    sendEvent(EV_VALUECHANGED);
}

byte SystemBus::readAddrBus() const
{
    return addr_bus;
}

void SystemBus::putOnAddrBus(byte value)
{
    addr_bus = value;
    sendEvent(EV_VALUECHANGED);
}

void SystemBus::initialize(bool xdata, bool xaddr, bool io,
    byte getReg, byte putReg, byte opflags_val,
    byte data_bus_val, byte addr_bus_val)
{
    _xdata = xdata;
    _xaddr = xaddr;
    _io = io;
    get = getReg;
    put = putReg;
    op = opflags_val;
    data_bus = data_bus_val;
    addr_bus = addr_bus_val;
    sendEvent(EV_VALUECHANGED);
}

void SystemBus::xdata(int from, int to, int opflags)
{
    _xdata = false;
    _xaddr = true;
    _io = true;
    get = from;
    put = to;
    op = opflags;
    sendEvent(EV_VALUECHANGED);
}

void SystemBus::xaddr(int from, int to, int opflags)
{
    _xdata = true;
    _xaddr = false;
    _io = true;
    get = from;
    put = to;
    op = opflags;
    sendEvent(EV_VALUECHANGED);
}

void SystemBus::io(int reg, int channel, int opflags)
{
    _xdata = true;
    _xaddr = true;
    _io = false;
    get = reg;
    put = channel;
    op = opflags;
    sendEvent(EV_VALUECHANGED);
}

void SystemBus::stop()
{
    _halt = false;
    sendEvent(EV_VALUECHANGED);
}

void SystemBus::suspend()
{
    _sus = false;
    sendEvent(EV_VALUECHANGED);
}

std::string SystemBus::to_string() const
{
    return format("DATA {02x} ADDR {02x} GET {01x} PUT {01x} OP {01x} ACT {1c} FLAGS {}\n",
        data_bus, addr_bus, get, put, op,
        (_xdata) ? ((_xaddr) ? '_' : 'A') : 'D',
        flagsString());
}

void SystemBus::setFlags(byte flags)
{
    m_flags = flags;
}

void SystemBus::setFlag(ProcessorFlags flag, bool flagValue)
{
    if (flagValue) {
        m_flags |= flag;
    } else {
        m_flags &= ~flag;
    }
}

void SystemBus::clearFlag(ProcessorFlags flag)
{
    setFlag(flag, false);
}

void SystemBus::clearFlags()
{
    m_flags = Clear;
}

bool SystemBus::isSet(ProcessorFlags flag) const
{
    return (m_flags & flag) != Clear;
}

std::string SystemBus::flagsString() const
{
    char ret[4];
    ret[0] = (isSet(C)) ? 'C' : '-';
    ret[1] = (isSet(Z)) ? 'Z' : '-';
    ret[2] = (isSet(V)) ? 'V' : '-';
    ret[3] = 0;
    return ret;
}

}
