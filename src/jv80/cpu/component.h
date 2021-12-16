/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <iomanip>
#include <iostream>

namespace Obelix::JV80::CPU {

typedef unsigned char byte;
typedef unsigned short word;

enum SystemError {
    NoError,
    InvalidComponentID,
    ProtectedMemory,
    InvalidInstruction,
    InvalidMicroCode,
    NoMicroCode,
    GeneralError,
};

class Component;

class ComponentListener {
public:
    virtual ~ComponentListener() = default;

    virtual void componentEvent(Component const* sender, int ev) = 0;
};

class Component {
private:
    ComponentListener* listener = nullptr;
    SystemError m_error = NoError;

protected:
    void sendEvent(int) const;

    SystemError error(SystemError err)
    {
        m_error = err;
        return m_error;
    }

public:
    virtual ~Component() = default;
    ComponentListener* setListener(ComponentListener*);
    SystemError error() const { return m_error; }
    virtual std::ostream& status(std::ostream& os) { return os; }
    virtual SystemError reset() { return error(NoError); }
    virtual SystemError onRisingClockEdge() { return error(NoError); }
    virtual SystemError onHighClock() { return error(NoError); }
    virtual SystemError onFallingClockEdge() { return error(NoError); }
    virtual SystemError onLowClock() { return error(NoError); };

    constexpr static int EV_VALUECHANGED = 0;

    friend std::ostream& operator<<(std::ostream&, Component&);
};

typedef std::function<void(Component*)> ComponentHandler;

}
