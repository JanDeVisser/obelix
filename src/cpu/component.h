/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>

#include "core/Error.h"
#include "core/Logging.h"

namespace Obelix::CPU {

typedef uint8_t byte;
typedef uint16_t word;

#define ENUMERATE_SYSTEM_ERROR_CODES(S) \
    S(NoError)                          \
    S(InvalidComponentID)               \
    S(ProtectedMemory)                  \
    S(InvalidInstruction)               \
    S(InvalidMicroCode)                 \
    S(NoMicroCode)                      \
    S(GeneralError)

enum class SystemErrorCode {
#undef ENUM_SYSTEM_ERROR_CODE
#define ENUM_SYSTEM_ERROR_CODE(code) code,
    ENUMERATE_SYSTEM_ERROR_CODES(ENUM_SYSTEM_ERROR_CODE)
#undef ENUM_SYSTEM_ERROR_CODE
};

constexpr const char* SystemErrorCode_name(SystemErrorCode code)
{
    switch (code) {
#undef ENUM_SYSTEM_ERROR_CODE
#define ENUM_SYSTEM_ERROR_CODE(code) \
    case SystemErrorCode::code:      \
        return #code;
        ENUMERATE_SYSTEM_ERROR_CODES(ENUM_SYSTEM_ERROR_CODE)
#undef ENUM_SYSTEM_ERROR_CODE
    default:
        fatal("Unreachable");
    }
}

using SystemError = ErrorOr<void, SystemErrorCode>;

class Component;

class ComponentListener {
public:
    virtual ~ComponentListener() = default;

    virtual void componentEvent(Component const* sender, int ev) = 0;
};

class Component {
public:
    virtual ~Component() = default;
    ComponentListener* setListener(ComponentListener*);
    [[nodiscard]] SystemErrorCode error() const { return m_error; }
    virtual SystemError reset() { return {}; }
    virtual SystemError onRisingClockEdge() { return {}; }
    virtual SystemError onHighClock() { return {}; }
    virtual SystemError onFallingClockEdge() { return {}; }
    virtual SystemError onLowClock() { return {}; };
    [[nodiscard]] virtual std::string to_string() const = 0;

    constexpr static int EV_VALUECHANGED = 0x00;
    constexpr static int EV_RISING_CLOCK = 0x01;
    constexpr static int EV_HIGH_CLOCK = 0x02;
    constexpr static int EV_FALLING_CLOCK = 0x03;
    constexpr static int EV_LOW_CLOCK = 0x04;

protected:
    void sendEvent(int) const;
    SystemErrorCode error(SystemErrorCode err);

private:
    ComponentListener* m_listener { nullptr };
    SystemErrorCode m_error = SystemErrorCode::NoError;
};

using ComponentHandler = std::function<SystemError(Component&)>;

}

namespace Obelix {

template<>
struct Converter<Obelix::CPU::SystemErrorCode> {
    static std::string to_string(Obelix::CPU::SystemErrorCode val)
    {
        return SystemErrorCode_name(val);
    }

    static double to_double(Obelix::CPU::SystemErrorCode val)
    {
        return static_cast<double>(val);
    }

    static long to_long(Obelix::CPU::SystemErrorCode val)
    {
        return (long)val;
    }
};

}
