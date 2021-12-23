/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <core/ScopeGuard.h>
#include <jv80/cpu/systembus.h>
#include <jv80/cpu/ComponentContainer.h>

namespace Obelix::JV80::CPU {

class Harness : public ComponentContainer {
public:
    explicit Harness()
        : ComponentContainer()
    {
    }

    explicit Harness(std::shared_ptr<ConnectedComponent> const& c)
        : Harness()
    {
        insert(c);
    }

    ErrorOr<int, SystemErrorCode> run(bool debug = false, int cycles = -1)
    {
        int i = 0;
        do {
            if (auto err = cycle(i); err.is_error())
                return ErrorOr<int, SystemErrorCode> { err.error() };
            i++;
        } while (bus().halt() && ((cycles == -1) || (i < cycles)));
        return ErrorOr<int, SystemErrorCode> { i };
    }

    [[nodiscard]] std::string to_string() const override
    {
        return "Harness";
    }

    SystemError cycles(int count)
    {
        for (int i = 0; i < count; i++) {
            TRY_RETURN(cycle(i));
        }
        return {};
    }

    SystemError status_message(const std::string& msg, int num)
    {
        std::cout << "Cycle " << num << " " << msg << "\n";
        std::cout << bus().to_string() << "\n";
        TRY_RETURN(forAllComponents([](Component& c) -> SystemError {
            std::cout << c.to_string() << "\n";
            return {};
        }));
        return {};
    }

    SystemError onClockEvent(const ComponentHandler& handler)
    {
        TRY_RETURN(forAllComponents(handler));
        TRY_RETURN(forAllChannels(handler));
        return {};
    }

    SystemError onRisingClockEdge() override
    {
        return onClockEvent([](Component& c) -> SystemError {
            return c.onRisingClockEdge();
        });
    }

    SystemError onHighClock() override
    {
        return onClockEvent([](Component& c) -> SystemError {
            return c.onHighClock();
        });
    }

    SystemError onFallingClockEdge() override
    {
        return onClockEvent([](Component& c) -> SystemError {
            return c.onFallingClockEdge();
        });
    }

    SystemError onLowClock() override
    {
        return onClockEvent([](Component& c) -> SystemError {
            return c.onLowClock();
        });
    }

    SystemError cycle(int num)
    {
        TRY_RETURN(onRisingClockEdge());
        TRY_RETURN(onHighClock());
        TRY_RETURN(onFallingClockEdge());
        TRY_RETURN(onLowClock());
        return {};
    }

    SystemError cycle(bool xdata, bool xaddr, bool io, byte getReg, byte putReg, byte opflags_val,
        byte data_bus_val = 0x00, byte addr_bus_val = 0x00)
    {
        bus().initialize(xdata, xaddr, io, getReg, putReg, opflags_val, data_bus_val, addr_bus_val);
        return cycle(0);
    }

    SystemError cycle(bool xdata, bool xaddr, byte getReg, byte putReg, byte opflags_val,
        byte data_bus_val = 0x00, byte addr_bus_val = 0x00)
    {
        return cycle(xdata, xaddr, true, getReg, putReg, opflags_val, data_bus_val, addr_bus_val);
    }
};

}
