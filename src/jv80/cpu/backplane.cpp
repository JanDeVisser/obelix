/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <jv80/cpu/addressregister.h>
#include <jv80/cpu/alu.h>
#include <jv80/cpu/backplane.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/memory.h>
#include <jv80/cpu/microcode.h>
#include <jv80/cpu/register.h>

namespace Obelix::JV80::CPU {

static byte mem[] = {
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

static MemoryBank image { 0x00, 0x11, true, mem };

BackPlane::BackPlane()
    : clock(this, 1.0)
{
}

void BackPlane::defaultSetup()
{
    insert(new Register(GP_A)); // 0x00
    insert(new Register(GP_B)); // 0x01
    insert(new Register(GP_C)); // 0x02
    insert(new Register(GP_D)); // 0x03
    auto lhs = new Register(LHS, "LHS");
    insert(lhs);                                               // 0x04
    insert(new ALU(RHS, lhs));                                 // 0x05
    insert(new Controller(mc));                                // 0x06
    insert(new AddressRegister(PC, "PC"));                     // 0x08
    insert(new AddressRegister(SP, "SP"));                     // 0x09
    insert(new AddressRegister(SI, "Si"));                     // 0x0A
    insert(new AddressRegister(DI, "Di"));                     // 0x0B
    insert(new AddressRegister(TX, "TX"));                     // 0x0C
    insert(new Memory(0x0000, 0xC000, 0xC000, 0x4000, image)); // 0x0F
}

SystemBus::RunMode BackPlane::runMode()
{
    return bus().runMode();
}

void BackPlane::setRunMode(SystemBus::RunMode runMode)
{
    return bus().setRunMode(runMode);
}

Controller* BackPlane::controller() const
{
    return dynamic_cast<Controller*>(component(IR));
}

Memory* BackPlane::memory() const
{
    return dynamic_cast<Memory*>(component(MEMADDR));
}

void BackPlane::loadImage(word sz, const byte* data, word addr, bool writable)
{
    image = MemoryBank(addr, sz, writable, data);
    memory()->add(image);
    reset();
}

void BackPlane::run(word fromAddress)
{
    if (!bus().sus()) {
        bus().clearSus();
    }

    // TODO: Better: build a prolog microcode script, or maybe
    // assembly, to setup SP, NMI, and PC.
    auto* pc = dynamic_cast<AddressRegister*>(component(PC));
    if ((fromAddress != 0xFFFF) && (fromAddress != pc->getValue())) {
        pc->setValue(fromAddress);
    }
    clock.start();
}

SystemError BackPlane::reportError()
{
    if (error() == NoError) {
        return NoError;
    }
    std::cout << "EXCEPTION " << error() << std::endl;
    clock.stop();
    return error();
}

SystemError BackPlane::onClockEvent(const ComponentHandler& handler)
{
    if (error() != NoError) {
        return error();
    }
    switch (m_phase) {
    case SystemClock:
        error(forAllComponents(handler));
        if (error() == NoError) {
            error(forAllChannels(handler));
        }
        return error();
    case IOClock:
        // xx
    default:
        return NoError;
    }
}

SystemError BackPlane::reset()
{
    if (error() != NoError) {
        return error();
    }
    error(bus().reset());
    if (error() == NoError) {
        forAllComponents([](Component* c) -> SystemError {
            return (c) ? c->reset() : NoError;
        });
    }
    return NoError;
}

std::ostream& BackPlane::status(std::ostream& os)
{
    bus().status(os);
    if (error(bus().error()) == NoError) {
        forAllComponents([&os](Component* c) -> SystemError {
            c->status(os);
            return c->error();
        });
    }
    return os;
}

SystemError BackPlane::onRisingClockEdge()
{
    if (error() != NoError) {
        return error();
    }
    if (m_phase == SystemClock) {
        if (m_output) {
            status(*m_output);
        }
        if (error() != NoError) {
            return error();
        }
    }
    return onClockEvent([](Component* c) -> SystemError {
        return (c) ? c->onRisingClockEdge() : NoError;
    });
}

SystemError BackPlane::onHighClock()
{
    error(onClockEvent([](Component* c) -> SystemError {
        return (c) ? c->onHighClock() : NoError;
    }));
    if ((error() == NoError) && !bus().halt()) {
        stop();
    }
    return error();
}

SystemError BackPlane::onFallingClockEdge()
{
    return onClockEvent([](Component* c) -> SystemError {
        return (c) ? c->onFallingClockEdge() : NoError;
    });
}

SystemError BackPlane::onLowClock()
{
    error(onClockEvent([](Component* c) -> SystemError {
        return (c) ? c->onLowClock() : NoError;
    }));
    if ((error() == NoError) && (!bus().halt() || !bus().sus())) {
        stop();
    }
    m_phase = (m_phase == SystemClock) ? IOClock : SystemClock;
    return error();
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
