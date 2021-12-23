/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <unistd.h>

#include <core/ScopeGuard.h>
#include <jv80/cpu/backplane.h>
#include <jv80/cpu/controller.h>
#include <jv80/cpu/emulator.h>
#include <jv80/cpu/register.h>
#include <jv80/cpu/registers.h>

namespace Obelix::JV80::CPU {

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
    return ErrorOr<uint16_t, SystemErrorCode> { (uint16_t)m_system.component(DI)->getValue() };
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
            printf("%04x %-6.6s%-9.9s    %02x %02x %02x %02x %04x %04x %04x\n",
                dynamic_cast<Controller const*>(sender)->pc(),
                mnemonic.c_str(), args.c_str(),
                std::dynamic_pointer_cast<Register>(m_system.component(GP_A))->getValue(),
                std::dynamic_pointer_cast<Register>(m_system.component(GP_B))->getValue(),
                std::dynamic_pointer_cast<Register>(m_system.component(GP_C))->getValue(),
                std::dynamic_pointer_cast<Register>(m_system.component(GP_D))->getValue(),
                std::dynamic_pointer_cast<AddressRegister>(m_system.component(SI))->getValue(),
                std::dynamic_pointer_cast<AddressRegister>(m_system.component(DI))->getValue(),
                std::dynamic_pointer_cast<AddressRegister>(m_system.component(SP))->getValue());
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
