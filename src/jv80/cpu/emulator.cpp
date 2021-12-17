/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <sys/stat.h>
#include <unistd.h>

#include <jv80/cpu/backplane.h>
#include <jv80/cpu/emulator.h>
#include <jv80/cpu/memory.h>

namespace Obelix::JV80::CPU {

CPU::CPU(std::string const& image)
    : m_status()
    , m_queuedKeys()
{
    m_system = new BackPlane();
    m_system->defaultSetup();
    m_system->setOutputStream(m_status);
    m_system->setRunMode(SystemBus::Continuous);
    m_keyboard = new IOChannel(0x00, "KEY", [this]() {
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

    m_terminal = new IOChannel(0x01, "OUT", [this](byte out) {
        write(1, &out, 1);
    });

    m_system->insertIO(m_keyboard);
    m_system->insertIO(m_terminal);

    openImage(image, 0, false);
}

uint16_t CPU::run(word addr)
{
    m_system->reset();
    m_system->run(addr);
    return m_system->component(0x0b)->getValue();
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
    if (!fread(bytearr, sz, 1, img)) {
        perror("fread");
        exit(-1);
    }
    m_system->loadImage(sz, bytearr, addr, writable);
    delete[] bytearr;
}

void CPU::openImage(std::string const& img, word addr, bool writable)
{
    FILE* f = fopen(img.c_str(), "rb");
    if (!f) {
        perror("fopen");
        exit(-1);
    }
    openImage(f, addr, writable);
}

}
