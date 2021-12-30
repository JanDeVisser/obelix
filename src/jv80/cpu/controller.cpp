/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iomanip>
#include <iostream>
#include <vector>

#include <core/Format.h>
#include <core/StringUtil.h>
#include <jv80/cpu/alu.h>
#include <jv80/cpu/addressregister.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/opcodes.h>
#include <jv80/cpu/register.h>
#include <jv80/cpu/registers.h>
#include <jv80/cpu/ComponentContainer.h>

namespace Obelix::JV80::CPU {

static MicroCode mcNMI = {
    .opcode = 0xFE,
    .instruction = "__nmi",
    .addressingMode = AddressingMode::Implied,
    .steps = {
        // Push the processor flags:
        { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::INC },
        { .action = MicroCode::XADDR, .src = RHS, .target = MEM, .opflags = SystemBus::None },

        // Push the return address:
        { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::INC },
        { .action = MicroCode::XDATA, .src = PC, .target = MEM, .opflags = SystemBus::None },
        { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::INC },
        { .action = MicroCode::XDATA, .src = PC, .target = MEM, .opflags = SystemBus::MSB },

        // Load PC with the subroutine address:
        { .action = MicroCode::XADDR, .src = CONTROLLER, .target = PC, .opflags = SystemBus::Done },
    }
};

MicroCodeRunner::MicroCodeRunner(Controller& controller, SystemBus& bus, const MicroCode* microCode)
    : m_controller(controller)
    , m_bus(bus)
    , m_mc(microCode)
    , m_steps()
{

    evaluateCondition();
    fetchSteps();
    if (!(m_mc->addressingMode & AddressingMode::Done)) {
        int ix = -1;
        do {
            ix++;
            m_steps.emplace_back(m_mc->steps[ix]);
        } while (!(m_mc->steps[ix].opflags & SystemBus::Done));
    }
}

void MicroCodeRunner::evaluateCondition()
{
    byte valid_byte;
    switch (m_mc->condition_op) {
    case MicroCode::And:
        valid_byte = m_bus.isSet((SystemBus::ProcessorFlags)m_mc->condition);
        break;
    case MicroCode::Nand:
        valid_byte = !m_bus.isSet((SystemBus::ProcessorFlags)m_mc->condition);
        break;
    default:
        valid_byte = true;
        break;
    }
    m_valid = valid_byte != 0;
}

void MicroCodeRunner::fetchSteps()
{
    switch (m_mc->addressingMode & AddressingMode::Mask) {
    case AddressingMode::ImmediateByte:
        fetchImmediateByte();
        break;
    case AddressingMode::ImmediateWord:
        fetchImmediateWord();
        break;
    case AddressingMode::IndirectByte:
        fetchIndirectByte();
        break;
    case AddressingMode::IndirectWord:
        fetchIndirectWord();
        break;
    case AddressingMode::IndexedByte:
    case AddressingMode::IndexedWord:
        fetchIndexed();
        break;
    default:
        break;
    }
}

void MicroCodeRunner::fetchImmediateByte()
{
    byte target = (m_valid) ? m_mc->subject : TX;
    m_steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::Action::XDATA, MEM, target, SystemBus::None });
}

void MicroCodeRunner::fetchImmediateWord()
{
    byte target = (m_valid && (m_mc->subject != PC) && (m_mc->subject != MEMADDR)) ? m_mc->subject : TX;
    m_steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::Action::XDATA, MEM, target, SystemBus::None });
    m_steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::Action::XDATA, MEM, target, SystemBus::MSB });
    if (m_valid && m_mc->subject != target) {
        m_steps.push_back({ MicroCode::Action::XADDR, TX, m_mc->subject, SystemBus::None });
    }
}

void MicroCodeRunner::fetchIndirectByte()
{
    m_steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::None });
    m_steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::MSB });
    if (m_valid) {
        m_steps.push_back({ MicroCode::Action::XADDR, TX, MEMADDR, SystemBus::None });
        m_steps.push_back({ MicroCode::Action::XDATA, MEM, m_mc->subject, SystemBus::None });
    }
}

void MicroCodeRunner::fetchIndirectWord()
{
    m_steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::None });
    m_steps.push_back({ MicroCode::Action::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::Action::XDATA, MEM, TX, SystemBus::MSB });
    if (m_valid) {
        m_steps.push_back({ MicroCode::Action::XADDR, TX, MEMADDR, SystemBus::None });
        m_steps.push_back({ MicroCode::Action::XDATA, MEM, m_mc->subject, SystemBus::INC });
        m_steps.push_back({ MicroCode::Action::XDATA, MEM, m_mc->subject, SystemBus::MSB });
    }
}

void MicroCodeRunner::fetchIndexed()
{
    m_steps.push_back({ MicroCode::XADDR, PC, MEMADDR, SystemBus::INC });
    m_steps.push_back({ MicroCode::XDATA, MEM, TX, SystemBus::None });
    m_steps.push_back({ MicroCode::XADDR, m_mc->subject, MEMADDR, SystemBus::None });
    m_steps.push_back({ MicroCode::XDATA, TX, MEMADDR, SystemBus::IDX });
}

bool MicroCodeRunner::grabConstant(int step)
{
    switch (m_mc->addressingMode & Mask) {
    case AddressingMode::Implied:
        m_complete = (step == 1);
        break;
    case AddressingMode::ImmediateByte:
    case AddressingMode::ImpliedByte:
    case AddressingMode::IndexedByte:
    case AddressingMode::IndexedWord:
        if (step == 2) {
            m_constant = m_bus.readDataBus();
            m_complete = true;
        }
        break;
    case AddressingMode::ImmediateWord:
    case AddressingMode::ImpliedWord:
    case AddressingMode::IndirectWord:
    case AddressingMode::IndirectByte:
        if (step == 2) {
            m_constant = m_bus.readDataBus();
        } else if (step == 4) {
            m_constant |= (((word)m_bus.readDataBus()) << 8);
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
    MicroCode::MicroCodeStep s = m_steps[step];
    byte src = (s.src != DEREFCONTROLLER) ? s.src : m_controller.scratch();
    byte target = (s.target != DEREFCONTROLLER) ? s.target : m_controller.scratch();
    switch (s.action) {
    case MicroCode::XDATA:
        m_bus.xdata(src, target, s.opflags & SystemBus::Mask);
        break;
    case MicroCode::XADDR:
        m_bus.xaddr(src, target, s.opflags & SystemBus::Mask);
        break;
    case MicroCode::IO:
        m_bus.io(src, target, s.opflags & SystemBus::Mask);
        break;
    case MicroCode::OTHER:
        switch (s.opflags & SystemBus::Mask) {
        case SystemBus::Halt:
            m_bus.stop();
            break;
        default:
            std::cerr << "Unhandled operation flag '" << std::hex << s.opflags
                      << "' for instruction " << std::hex << m_mc->opcode << " step "
                      << std::dec << step << std::endl;
            return SystemError { SystemErrorCode::InvalidMicroCode };
        }
        break;
    default:
        std::cerr << "Unhandled microcode action '" << std::hex << s.action
                  << "' for instruction " << std::hex << m_mc->opcode << " step "
                  << std::dec << step << std::endl;
        return SystemError { SystemErrorCode::InvalidMicroCode };
    }
    return {};
}

bool MicroCodeRunner::hasStep(int step)
{
    return m_steps.size() > step;
}

std::string MicroCodeRunner::instruction() const
{
    std::string instr = m_mc->instruction;
    bool do_format = false;
    auto pos = instr.find("$xxxx");
    if (pos != std::string::npos) {
        instr.replace(pos, 5, "${04x}", 6);
        do_format = true;
    } else {
        pos = instr.find_first_of("$xx");
        if (pos != std::string::npos) {
            instr.replace(pos, 3, "${02x}", 6);
            do_format = true;
        }
    }
    if (do_format)
        instr = format(instr, m_constant);
    return to_lower(instr);
}

word MicroCodeRunner::constant() const
{
    return m_constant;
}

// -----------------------------------------------------------------------

Controller::Controller(const MicroCode* mc)
    : Register(IR)
    , m_microCode(mc)
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

std::string Controller::to_string() const
{
    return format("{01x}. IR {02x} {04x} {>15s} Step {}", address(), getValue(), constant(), instruction().c_str(), m_step);
}

SystemError Controller::reset()
{
    TRY_RETURN(Register::reset());
    m_step = 0;
    return {};
}

SystemError Controller::onRisingClockEdge()
{
    if (bus()->getAddress() == CONTROLLER) {
        if (!bus()->xdata()) {
            bus()->putOnDataBus(scratch());
        } else if (!bus()->xaddr()) {
            bus()->putOnDataBus(m_interruptVector & 0x00FF);
            bus()->putOnAddrBus((m_interruptVector & 0xFF00) >> 8);
        }
    } else {
        TRY_RETURN(this->Register::onRisingClockEdge());
    }
    return {};
}

SystemError Controller::onHighClock()
{
    if (bus()->putAddress() == CONTROLLER) {
        if (!bus()->xdata()) {
            m_scratch = bus()->readDataBus();
        } else if (!bus()->xaddr()) {
            m_interruptVector = bus()->readDataBus();
            m_interruptVector |= ((word)bus()->readAddrBus()) << 8;
        }
    } else {
        TRY_RETURN(this->Register::onHighClock());
    }
    m_suspended++;
    if (m_runner && m_runner->grabConstant(m_step - 2)) {
        sendEvent(EV_VALUECHANGED);
    }
    return {};
}

SystemError Controller::onLowClock()
{
    const MicroCode* mc = nullptr;

    if ((m_suspended >= 1) && (runMode() == SystemBus::RunMode::BreakAtInstruction) && m_runner && m_runner->complete()) {
        m_suspended = -16;
        bus()->suspend();
        return {};
    }

    switch (m_step) {
    case 0:
        m_pc = bus()->backplane()->component<AddressRegister>(PC)->getValue();
        bus()->xaddr(PC, MEMADDR, SystemBus::INC);
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
            mc = m_microCode + getValue();
            if (!mc->opcode) {
                m_runner.reset();
            } else if (mc->opcode != getValue()) {
                std::cerr << "Microcode mismatch for opcode " << std::hex << getValue()
                          << ": 'opcode' field contains " << std::hex << mc->opcode
                          << std::endl;
                return SystemError { SystemErrorCode::InvalidMicroCode };
            }
        }
        if (mc->opcode) {
            m_runner.emplace(*this, *bus(), mc);
        } else {
            m_runner.reset();
        }
        // fall through:
    default:
        if (m_runner.has_value() && m_runner.value().hasStep(m_step - 2)) {
            TRY_RETURN(m_runner->executeNextStep(m_step - 2));
            if (!bus()->halt()) {
                sendEvent(EV_AFTERINSTRUCTION);
            }
        } else {
            if (getValue() == RTI) {
                m_servicingNMI = false;
            }
            sendEvent(EV_AFTERINSTRUCTION);
            m_runner.reset();
            setValue(0);
            if (!bus()->nmi()) {
                m_step = 1;
            } else {
                m_step = 0;
                m_pc = bus()->backplane()->component<AddressRegister>(PC)->getValue();
                bus()->xaddr(PC, MEMADDR, SystemBus::INC);
            }
        }
        break;
    }
    m_step++;
    sendEvent(EV_STEPCHANGED);
    if (runMode() == SystemBus::RunMode::BreakAtClock) {
        bus()->suspend();
    }
    return {};
}

std::string Controller::instructionWithOpcode(int opcode) const
{
    auto mc = m_microCode + opcode;
    return (mc && (mc->opcode == opcode)) ? mc->instruction : "NOP";
}

int Controller::opcodeForInstruction(const std::string& instr) const
{
    for (int ix = 0; ix < 256; ix++) {
        auto mc = m_microCode + ix;
        if (mc && (mc->opcode == ix) && (instr == mc->instruction)) {
            return ix;
        }
    }
    return -1;
}

}
