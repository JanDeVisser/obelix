/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <vector>

#include <cpu/addressregister.h>
#include <cpu/controller.h>
#include <cpu/opcodes.h>
#include <cpu/register.h>
#include <cpu/registers.h>

namespace Obelix::JV80::CPU {

static MicroCode mcNMI = {
    .opcode = 0xFE, .instruction = "__nmi", .addressingMode = Immediate, .steps = {
                                                                             // Push the processor flags:
                                                                             { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                             { .action = MicroCode::XADDR, .src = RHS, .target = MEM, .opflags = SystemBus::None },

                                                                             // Push the return address:
                                                                             { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                             { .action = MicroCode::XDATA, .src = PC, .target = MEM, .opflags = SystemBus::None },
                                                                             { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                             { .action = MicroCode::XDATA, .src = PC, .target = MEM, .opflags = SystemBus::MSB },

                                                                             // Load PC with the subroutine address:
                                                                             { .action = MicroCode::XADDR, .src = CONTROLLER, .target = PC, .opflags = SystemBus::Done },
                                                                         }
};

MicroCodeRunner::MicroCodeRunner(Controller* controller, SystemBus* bus, const MicroCode* microCode)
    : m_controller(controller)
    , m_bus(bus)
    , mc(microCode)
    , steps()
{

    evaluateCondition();
    fetchSteps();
    if (!(mc->addressingMode & AddressingMode::Done)) {
        int ix = -1;
        do {
            ix++;
            steps.emplace_back(mc->steps[ix]);
        } while (!(mc->steps[ix].opflags & SystemBus::Done));
    }
}

void MicroCodeRunner::evaluateCondition()
{
    byte valid_byte;
    switch (mc->condition_op) {
    case MicroCode::And:
        valid_byte = m_bus->isSet((SystemBus::ProcessorFlags)mc->condition);
        break;
    case MicroCode::Nand:
        valid_byte = !m_bus->isSet((SystemBus::ProcessorFlags)mc->condition);
        break;
    default:
        valid_byte = true;
        break;
    }
    valid = valid_byte != 0;
}

void MicroCodeRunner::fetchSteps()
{
    switch (mc->addressingMode & AddressingMode::Mask) {
    case DirectByte:
        fetchDirectByte();
        break;
    case DirectWord:
        fetchDirectWord();
        break;
    case AbsoluteByte:
        fetchAbsoluteByte();
        break;
    case AbsoluteWord:
        fetchAbsoluteWord();
        break;
    default:
        break;
    }
}

void MicroCodeRunner::fetchDirectByte()
{
    byte target = (valid) ? mc->target : TX;
    steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::Inc });
    steps.push_back({ MicroCode::Action::XDATA, MEM, target, SystemBus::None });
}

void MicroCodeRunner::fetchDirectWord()
{
    byte target = (valid && (mc->target != PC) && (mc->target != MEMADDR)) ? mc->target : TX;
    steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::Inc });
    steps.push_back({ MicroCode::Action::XDATA, MEM, target, SystemBus::None });
    steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::Inc });
    steps.push_back({ MicroCode::Action::XDATA, MEM, target, SystemBus::MSB });
    if (valid && mc->target != target) {
        steps.push_back({ MicroCode::Action::XADDR, TX, mc->target, SystemBus::None });
    }
}

void MicroCodeRunner::fetchAbsoluteByte()
{
    steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::Inc });
    steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::None });
    steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::Inc });
    steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::MSB });
    if (valid) {
        steps.push_back({ MicroCode::Action::XADDR, TX, MEMADDR, SystemBus::None });
        steps.push_back({ MicroCode::Action::XDATA, MEM, mc->target, SystemBus::None });
    }
}

void MicroCodeRunner::fetchAbsoluteWord()
{
    steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::Inc });
    steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::None });
    steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::Inc });
    steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::MSB });
    if (valid) {
        steps.push_back({ MicroCode::Action::XADDR, TX, MEMADDR, SystemBus::Inc });
        steps.push_back({ MicroCode::Action::XDATA, MEM, mc->target, SystemBus::None });
        steps.push_back({ MicroCode::Action::XADDR, TX, MEMADDR, SystemBus::None });
        steps.push_back({ MicroCode::Action::XDATA, MEM, mc->target, SystemBus::MSB });
    }
}

bool MicroCodeRunner::grabConstant(int step)
{
    switch (mc->addressingMode & Mask) {
    case Immediate:
        m_complete = (step == 1);
        break;
    case DirectByte:
    case ImmediateByte:
        if (step == 2) {
            m_constant = m_bus->readDataBus();
            m_complete = true;
        }
        break;
    case ImmediateWord:
    case DirectWord:
    case AbsoluteByte:
    case AbsoluteWord:
        if (step == 2) {
            m_constant = m_bus->readDataBus();
        } else if (step == 4) {
            m_constant |= (((word)m_bus->readDataBus()) << 8);
            m_complete = true;
        }
        break;
    default:
        break;
    }
    return m_complete;
}

SystemError MicroCodeRunner::executeNextStep(int step)
{
    MicroCode::MicroCodeStep s = steps[step];
    byte src = (s.src != DEREFCONTROLLER) ? s.src : m_controller->scratch();
    byte target = (s.target != DEREFCONTROLLER) ? s.target : m_controller->scratch();
    switch (s.action) {
    case MicroCode::XDATA:
        m_bus->xdata(src, target, s.opflags & SystemBus::Mask);
        break;
    case MicroCode::XADDR:
        m_bus->xaddr(src, target, s.opflags & SystemBus::Mask);
        break;
    case MicroCode::IO:
        m_bus->io(src, target, s.opflags & SystemBus::Mask);
        break;
    case MicroCode::OTHER:
        switch (s.opflags & SystemBus::Mask) {
        case SystemBus::Halt:
            m_bus->stop();
            break;
        default:
            std::cerr << "Unhandled operation flag '" << std::hex << s.opflags
                      << "' for instruction " << std::hex << mc->opcode << " step "
                      << std::dec << step << std::endl;
            return InvalidMicroCode;
        }
        break;
    default:
        std::cerr << "Unhandled microcode action '" << std::hex << s.action
                  << "' for instruction " << std::hex << mc->opcode << " step "
                  << std::dec << step << std::endl;
        return InvalidMicroCode;
    }
    return NoError;
}

bool MicroCodeRunner::hasStep(int step)
{
    return steps.size() > step;
}

std::string MicroCodeRunner::instruction() const
{
    if (strchr(mc->instruction, '%')) {
        char buf[32];
        snprintf(buf, 32, mc->instruction, m_constant);
        return buf;
    } else {
        return mc->instruction;
    }
}

word MicroCodeRunner::constant() const
{
    return m_constant;
}

// -----------------------------------------------------------------------

Controller::Controller(const MicroCode* mc)
    : Register(IR)
    , microCode(mc)
{
}

std::string Controller::instruction() const
{
    if (m_runner) {
        return m_runner->instruction();
    } else {
        return "----";
    }
}

word Controller::constant() const
{
    if (m_runner) {
        return m_runner->constant();
    } else {
        return 0;
    }
}

std::ostream& Controller::status(std::ostream& os)
{
    char buf[80];
    snprintf(buf, 80, "%1x. IR %02x %04x %-15.15s Step %d", id(), getValue(), constant(), instruction().c_str(), step);
    os << buf << std::endl;
    return os;
}

SystemError Controller::reset()
{
    step = 0;
    Register::reset();
    return NoError;
}

SystemError Controller::onRisingClockEdge()
{
    if (bus()->getID() == CONTROLLER) {
        if (!bus()->xdata()) {
            bus()->putOnDataBus(scratch());
        } else if (!bus()->xaddr()) {
            bus()->putOnDataBus(m_interruptVector & 0x00FF);
            bus()->putOnAddrBus((m_interruptVector & 0xFF00) >> 8);
        }
    } else {
        this->Register::onRisingClockEdge();
    }
    return NoError;
}

SystemError Controller::onHighClock()
{
    if (bus()->putID() == CONTROLLER) {
        if (!bus()->xdata()) {
            m_scratch = bus()->readDataBus();
        } else if (!bus()->xaddr()) {
            m_interruptVector = bus()->readDataBus();
            m_interruptVector |= ((word)bus()->readAddrBus()) << 8;
        }
    } else {
        this->Register::onHighClock();
    }
    m_suspended++;
    if (m_runner && m_runner->grabConstant(step - 2)) {
        sendEvent(EV_VALUECHANGED);
    }
    return NoError;
}

SystemError Controller::onLowClock()
{
    const MicroCode* mc = nullptr;

    if ((m_suspended >= 1) && (runMode() == SystemBus::BreakAtInstruction) && m_runner && m_runner->complete()) {
        m_suspended = -16;
        bus()->suspend();
        return NoError;
    }

    switch (step) {
    case 0:
        bus()->xaddr(PC, MEMADDR, SystemBus::Inc);
        break;
    case 1:
        bus()->xdata(MEM, IR, SystemBus::None);
        m_suspended = 0;
        break;
    case 2:
        if (!bus()->nmi()) {
            if ((m_interruptVector != 0xFFFF) && !m_servicingNMI) {
                mc = &mcNMI;
                m_servicingNMI = true;
            }
            bus()->clearNmi();
        } else {
            mc = microCode + getValue();
            if (!mc->opcode) {
                m_runner = nullptr;
            } else if (mc->opcode != getValue()) {
                std::cerr << "Microcode mismatch for opcode " << std::hex << getValue()
                          << ": 'opcode' field contains " << std::hex << mc->opcode
                          << std::endl;
                return InvalidMicroCode;
            }
        }
        m_runner = (mc->opcode) ? new MicroCodeRunner(this, bus(), mc) : nullptr;
        // fall through:
    default:
        if (m_runner && m_runner->hasStep(step - 2)) {
            auto err = m_runner->executeNextStep(step - 2);
            if (err != NoError) {
                return err;
            }
            if (!bus()->halt()) {
                sendEvent(EV_AFTERINSTRUCTION);
            }
        } else {
            if (getValue() == RTI) {
                m_servicingNMI = false;
            }
            sendEvent(EV_AFTERINSTRUCTION);
            delete m_runner;
            m_runner = nullptr;
            setValue(0);
            if (!bus()->nmi()) {
                step = 1;
            } else {
                step = 0;
                bus()->xaddr(PC, MEMADDR, SystemBus::Inc);
            }
        }
        break;
    }
    step++;
    sendEvent(EV_STEPCHANGED);
    if (runMode() == SystemBus::BreakAtClock) {
        bus()->suspend();
    }
    return NoError;
}

std::string Controller::instructionWithOpcode(int opcode) const
{
    auto mc = microCode + opcode;
    return (mc && (mc->opcode == opcode)) ? mc->instruction : "NOP";
}

int Controller::opcodeForInstruction(const std::string& instr) const
{
    for (int ix = 0; ix < 256; ix++) {
        auto mc = microCode + ix;
        if (mc && (mc->opcode == ix) && (instr == mc->instruction)) {
            return ix;
        }
    }
    return -1;
}
}
