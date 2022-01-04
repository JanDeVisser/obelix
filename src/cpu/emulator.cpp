/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <unistd.h>

#include "core/ScopeGuard.h"
#include "backplane.h"
#include "controller.h"
#include "emulator.h"
#include "register.h"
#include "registers.h"

namespace Obelix::CPU {

CPU::CPU(std::string const& image)
    : m_queuedKeys()
{
    m_system.defaultSetup();
    m_system.setRunMode(SystemBus::RunMode::Continuous);
    m_system.controller()->setListener(this);
    m_keyboard = std::make_shared<IOChannel>(0x00, "KEY", [this]() {
        byte b = 0xFF;
        {
            while (read(0, &b, 1)) {
                m_queuedKeys.push_back(b);
            }
        }
        if (!m_queuedKeys.empty()) {
            b = m_queuedKeys.front();
            m_queuedKeys.pop_front();
        }
        return b;
    });

    m_terminal = std::make_shared<IOChannel>(0x01, "OUT", [](byte out) {
        write(1, &out, 1);
    });

    m_system.insertIO(m_keyboard);
    m_system.insertIO(m_terminal);

    openImage(image, 0, false);
}

CPU::~CPU()
{
}

ErrorOr<uint16_t, SystemErrorCode> CPU::run(bool trace, word addr)
{
    ScopeGuard sg([this]() { m_trace = false; });
    m_trace = trace;
    if (auto err = m_system.reset(); err.is_error()) {
        auto e = err.error();
        return { e };
    }
    if (auto err = m_system.run(addr); err.is_error()) {
        auto e = err.error();
        return ErrorOr<uint16_t, SystemErrorCode> { e };
    }
    return ErrorOr<uint16_t, SystemErrorCode> { (uint16_t)m_system.component<AddressRegister>(DI)->getValue() };
}

void CPU::componentEvent(Component const* sender, int ev)
{
    switch (ev) {
    case Controller::EV_AFTERINSTRUCTION:
        if (m_trace) {
            auto instr = dynamic_cast<Controller const*>(sender)->instruction();
            std::string args;
            std::string mnemonic = instr;
            auto pos = instr.find_first_of(' ');
            if (pos != std::string::npos) {
                mnemonic = instr.substr(0, pos);
                args = instr.substr(pos);
            }
            auto sp = m_system.component<AddressRegister>(SP)->getValue();
            auto bp = m_system.component<AddressRegister>(BP)->getValue();
            auto mem = m_system.component<Memory>();
            std::string stack;
            for (auto ix = sp; 0xC000 < ix; ix -= 2) {
                if (ix == bp)
                    stack += " | ";
                stack += format("{04x} ", (((uint16_t)mem->peek(ix - 1).value()) << 8) | mem->peek(ix - 2).value());
            }
            printf("%04x %-6.6s%-15.15s    %02x %02x %02x %02x %04x %04x %04x %04x    %s\n",
                dynamic_cast<Controller const*>(sender)->pc(),
                mnemonic.c_str(), args.c_str(),
                m_system.component<Register>(GP_A)->getValue(),
                m_system.component<Register>(GP_B)->getValue(),
                m_system.component<Register>(GP_C)->getValue(),
                m_system.component<Register>(GP_D)->getValue(),
                m_system.component<AddressRegister>(SI)->getValue(),
                m_system.component<AddressRegister>(DI)->getValue(),
                m_system.component<AddressRegister>(SP)->getValue(),
                m_system.component<AddressRegister>(BP)->getValue(),
                stack.c_str());
        }
        break;
    default:
        break;
    }
}

void CPU::openImage(FILE* img, word addr, bool writable)
{
    if (fseek(img, 0, SEEK_END)) {
        perror("fseek");
        exit(-1);
    }
    size_t sz;
    if ((sz = ftell(img)) < 0) {
        perror("ftell");
        exit(-1);
    }
    if (fseek(img, 0, SEEK_SET)) {
        perror("fseek");
        exit(-1);
    }

    byte* bytearr = new byte[sz];
    {
        ScopeGuard sg { [bytearr]() { delete[] bytearr; } };
        if (!fread(bytearr, sz, 1, img)) {
            perror("fread");
            exit(-1);
        }
        if (auto err = m_system.loadImage(sz, bytearr, addr, writable); err.is_error()) {
            fprintf(stderr, "Error loading image\n");
            exit(-1);
        }
    }
}

void CPU::openImage(std::string const& img, word addr, bool writable)
{
    FILE* f = fopen(img.c_str(), "rb");
    {
        ScopeGuard sg { [f]() { fclose(f); } };
        if (!f) {
            perror("fopen");
            exit(-1);
        }
        openImage(f, addr, writable);
    }
}

}