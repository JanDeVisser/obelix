/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "component.h"
#include <vector>

namespace Obelix::CPU {

using Reset = std::function<void(void)>;
using ClockEvent = std::function<SystemError()>;

class ComponentContainer;

class SystemBus : public Component {
public:
    enum class RunMode {
        Continuous = 0,
        BreakAtInstruction = 1,
        BreakAtClock = 2,
    };

    enum ProcessorFlags {
        Clear = 0x00,
        Z = 0x01,
        C = 0x02,
        V = 0x04,
    };

    enum OperatorFlags {
        None = 0x00,
        IOIn = 0x01,
        INC = 0x01,
        DEC = 0x02,
        IDX = 0x04,
        Flags = 0x04,
        MSB = 0x08,
        Halt = 0x08,
        IOOut = 0x08,
        Mask = 0x0F,
        Done = 0x10,
    };

    explicit SystemBus(ComponentContainer*);
    ~SystemBus() override = default;
    [[nodiscard]] byte readDataBus() const;
    void putOnDataBus(byte);
    [[nodiscard]] byte readAddrBus() const;
    void putOnAddrBus(byte);
    [[nodiscard]] bool xdata() const { return _xdata; }
    [[nodiscard]] bool xaddr() const { return _xaddr; }
    [[nodiscard]] bool io() const { return _io; }
    [[nodiscard]] bool halt() const { return _halt; }
    [[nodiscard]] bool sus() const { return _sus; }
    void clearSus() { _sus = true; }
    [[nodiscard]] bool nmi() const { return _nmi; }
    void setNmi() { _nmi = false; }
    void clearNmi() { _nmi = true; }
    [[nodiscard]] byte putAddress() const { return put; }
    [[nodiscard]] byte getAddress() const { return get; }
    [[nodiscard]] byte opflags() const { return op; }

    void initialize(bool, bool, bool, byte, byte, byte, byte = 0x00, byte = 0x00);
    void xdata(int, int, int);
    void xaddr(int, int, int);
    void io(int, int, int);
    void stop();
    void suspend();
    SystemError reset() override;
    [[nodiscard]] std::string to_string() const override;

    void setFlag(ProcessorFlags, bool = true);
    void clearFlag(ProcessorFlags);
    void clearFlags();
    void setFlags(byte);
    [[nodiscard]] byte flags() const { return m_flags; }
    [[nodiscard]] bool isSet(ProcessorFlags) const;
    [[nodiscard]] std::string flagsString() const;

    [[nodiscard]] RunMode runMode() const { return m_runMode; }
    void setRunMode(RunMode runMode) { m_runMode = runMode; }

    ComponentContainer* backplane() { return m_backplane; }
    [[nodiscard]] ComponentContainer* backplane() const { return m_backplane; }

private:
    ComponentContainer* m_backplane;
    byte data_bus = 0;
    byte addr_bus = 0;
    byte put = 0;
    byte get = 0;
    byte op = 0;
    bool _halt = true;
    bool _sus = true;
    bool _nmi = true;
    bool _xdata = true;
    bool _xaddr = true;
    bool rst = false;
    bool _io = true;

    byte m_flags = 0x0;

    void _reset();
    RunMode m_runMode = RunMode::Continuous;
};

}
