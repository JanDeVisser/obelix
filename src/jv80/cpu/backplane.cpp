/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Logging.h>
#include <jv80/cpu/addressregister.h>
#include <jv80/cpu/alu.h>
#include <jv80/cpu/backplane.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/memory.h>
#include <jv80/cpu/microcode.h>
#include <jv80/cpu/register.h>

namespace Obelix::JV80::CPU {

static byte sample_mem[] = {
    /* 0x0000 */ CLR_A,
    /* 0x0001 */ CLR_B,
    /* 0x0002 */ MOV_C_IMM, 0x01,
    /* 0x0004 */ CLR_D,
    /* 0x0005 */ MOV_SI_IMM, 0x17, 0x00,
    /* 0x0008 */ ADD_AB_CD,
    /* 0x0009 */ SWP_A_C,
    /* 0x000A */ SWP_B_D,
    /* 0x000B */ DEC_SI,
    /* 0x000C */ JNZ, 0x08, 0x00,
    /* 0x000F */ MOV_DI_CD,
    /* 0x0010 */ HLT
};

BackPlane::BackPlane()
    : clock(this, 1.0)
{
}

void BackPlane::defaultSetup()
{
    insert(std::make_shared<Register>(GP_A)); // 0x00
    insert(std::make_shared<Register>(GP_B)); // 0x01
    insert(std::make_shared<Register>(GP_C)); // 0x02
    insert(std::make_shared<Register>(GP_D)); // 0x03
    auto lhs = std::make_shared<Register>(LHS, "LHS");
    insert(lhs);                                         // 0x04
    insert(std::make_shared<ALU>(RHS, lhs));             // 0x05
    insert(std::make_shared<Controller>(mc));            // 0x06
    insert(std::make_shared<AddressRegister>(PC, "PC")); // 0x08
    insert(std::make_shared<AddressRegister>(SP, "SP")); // 0x09
    insert(std::make_shared<AddressRegister>(SI, "Si")); // 0x0A
    insert(std::make_shared<AddressRegister>(DI, "Di")); // 0x0B
    insert(std::make_shared<AddressRegister>(TX, "TX")); // 0x0C
    auto mem = std::make_shared<Memory>();
    mem->add(0x0000, 0xC000, false, sample_mem);
    mem->add(0xC000, 0x4000, true);
    insert(mem); // 0x0F
}

SystemBus::RunMode BackPlane::runMode()
{
    return bus().runMode();
}

void BackPlane::setRunMode(SystemBus::RunMode runMode)
{
    return bus().setRunMode(runMode);
}

std::shared_ptr<Controller> BackPlane::controller() const
{
    return component<Controller>();
}

std::shared_ptr<Memory> BackPlane::memory() const
{
    return component<Memory>();
}

SystemError BackPlane::loadImage(word sz, const byte* data, word addr, bool writable)
{
    if (!memory()->add(addr, sz, writable, data))
        return SystemError { SystemErrorCode::ProtectedMemory };
    return reset();
}

SystemError BackPlane::run(word fromAddress)
{
    if (!bus().sus()) {
        bus().clearSus();
    }

    // TODO: Better: build a prolog microcode script, or maybe
    // assembly, to setup SP, NMI, and PC.
    auto pc = component<AddressRegister>(PC);
    if ((fromAddress != 0xFFFF) && (fromAddress != pc->getValue())) {
        pc->setValue(fromAddress);
    }
    return clock.start();
}

SystemError BackPlane::onClockEvent(const ComponentHandler& handler)
{
    switch (m_phase) {
    case SystemClock:
        return forAllComponents(handler);
    case IOClock:
        // xx
    default:
        return {};
    }
}

SystemError BackPlane::reset()
{
    if (auto err = bus().reset(); err.is_error())
        return err;
    return forAllComponents([](Component& c) -> SystemError {
        return c.reset();
    });
}

std::string BackPlane::to_string() const
{
    std::string ret = bus().to_string();
    if (auto err = forAllComponents([&ret](Component& c) -> SystemError {
            ret = ret + "\n" + c.to_string();
            return {};
        });
        err.is_error()) {
        fatal("Error in BackPlane::to_string: {}", err.error());
    }
    return ret;
}

SystemError BackPlane::onRisingClockEdge()
{
    sendEvent(EV_RISING_CLOCK);
    return onClockEvent([](Component& c) -> SystemError {
        return c.onRisingClockEdge();
    });
}

SystemError BackPlane::onHighClock()
{
    sendEvent(EV_HIGH_CLOCK);
    auto on_high_clock = [](Component& c) -> SystemError {
        return c.onHighClock();
    };

    if (auto err = onClockEvent(on_high_clock); err.is_error()) {
        return err;
    }
    if (!bus().halt()) {
        stop();
    }
    return {};
}

SystemError BackPlane::onFallingClockEdge()
{
    sendEvent(EV_FALLING_CLOCK);
    return onClockEvent([](Component& c) -> SystemError {
        return c.onFallingClockEdge();
    });
}

SystemError BackPlane::onLowClock()
{
    sendEvent(EV_LOW_CLOCK);
    auto on_low_clock = [](Component& c) -> SystemError {
        return c.onHighClock();
    };

    if (auto err = onClockEvent(on_low_clock); err.is_error()) {
        return err;
    }
    if (!bus().halt() || !bus().sus()) {
        stop();
    }
    m_phase = (m_phase == SystemClock) ? IOClock : SystemClock;
    return {};
}

bool BackPlane::setClockSpeed(double freq)
{
    return clock.setSpeed(freq);
}

double BackPlane::clockSpeed()
{
    return clock.frequency();
}

}
