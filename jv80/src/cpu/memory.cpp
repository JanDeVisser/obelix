/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cpu/memory.h>
#include <cstring>
#include <iostream>
#include <memory>

namespace Obelix::JV80::CPU {

/* ----------------------------------------------------------------------- */

MemoryBank::MemoryBank(word start, word size, bool writable, const byte* image) noexcept
    : m_start(start)
    , m_size(size)
    , m_writable(writable)
{
    if ((int(start) + int(size)) > 0xFFFF) {
        m_start = m_size = 0x00;
        return;
    }

    m_image = std::shared_ptr<byte>(new byte[m_size], std::default_delete<byte[]>());
    if (image) {
        memcpy(m_image.get(), image, size);
    } else {
        erase();
    }
}

MemoryBank::MemoryBank(const MemoryBank& other)
    : m_start(other.start())
    , m_size(other.size())
    , m_writable(other.writable())
{
    m_image = other.m_image;
}

MemoryBank::MemoryBank(MemoryBank&& other) noexcept
{
    m_start = other.start();
    m_size = other.size();
    m_writable = other.writable();
    m_image = other.m_image;
    other.m_image = nullptr;
    other.m_size = 0;
}

MemoryBank& MemoryBank::operator=(const MemoryBank& other)
{
    if (this != &other) {
        m_start = other.start();
        m_size = other.size();
        m_writable = other.writable();
        m_image = other.m_image;
    }
    return *this;
}

MemoryBank& MemoryBank::operator=(MemoryBank&& other) noexcept
{
    m_start = other.start();
    m_size = other.size();
    m_writable = other.writable();
    m_image = other.m_image;
    other.m_image = nullptr;
    other.m_size = 0;
    return *this;
}

byte& MemoryBank::operator[](std::size_t addr)
{
    if (mapped(addr)) {
        return m_image.get()[offset(addr)];
    } else {
        throw std::exception(); // FIXME
    }
}

const byte& MemoryBank::operator[](std::size_t addr) const
{
    if (mapped(addr)) {
        return m_image.get()[offset(addr)];
    } else {
        throw std::exception(); // FIXME
    }
}

bool MemoryBank::operator<(const MemoryBank& other) const
{
    return start() < other.start();
}

bool MemoryBank::operator<=(const MemoryBank& other) const
{
    return start() < other.start();
}

bool MemoryBank::operator>(const MemoryBank& other) const
{
    return start() > other.start();
}

bool MemoryBank::operator>=(const MemoryBank& other) const
{
    return start() > other.start();
}

bool MemoryBank::operator==(const MemoryBank& other) const
{
    return (start() == other.start()) && (start() || (size() == other.size()));
}

bool MemoryBank::operator!=(const MemoryBank& other) const
{
    return (start() != other.start()) || (!start() && (size() != other.size()));
}

std::string MemoryBank::name() const
{
    char buf[80];
    snprintf(buf, 80, "%s %04x-%04x", ((writable()) ? "RAM" : "ROM"), start(), end());
    return buf;
}

void MemoryBank::erase()
{
    memset(m_image.get(), 0, m_size);
}

void MemoryBank::copy(MemoryBank& other)
{
    if (fits(other.start(), other.size())) {
        for (auto ix = 0; ix < other.size(); ix++) {
            m_image.get()[offset(other.start()) + ix] = other.m_image.get()[ix];
        }
        other.m_image = nullptr;
        other.m_size = 0;
    }
}

void MemoryBank::copy(MemoryBank&& other)
{
    copy(other);
}

void MemoryBank::copy(size_t addr, size_t size, const byte* contents)
{
    if (fits(addr, size)) {
        for (auto ix = 0; ix < size; ix++) {
            m_image.get()[offset(addr) + ix] = contents[ix];
        }
    }
}

bool MemoryBank::mapped(size_t addr) const
{
    return (start() <= addr) && (addr < end());
}

bool MemoryBank::fits(size_t addr, size_t size) const
{
    return mapped(addr) && mapped(addr + size);
}

bool MemoryBank::disjointFrom(size_t addr, size_t size) const
{
    return ((addr < start()) && ((addr + size) < start())) || (addr >= end());
}

/* ----------------------------------------------------------------------- */

Memory::Memory()
    : AddressRegister(ADDR_ID, "M")
    , m_banks()
{
}

Memory::Memory(MemoryBank&& bank)
    : Memory()
{
    initialize(bank);
}

Memory::Memory(MemoryBank& bank)
    : Memory()
{
    initialize(bank);
}

Memory::Memory(word ramStart, word ramSize, word romStart, word romSize, MemoryBank& bank)
    : Memory()
{
    add(MemoryBank(ramStart, ramSize, true));
    add(MemoryBank(romStart, romSize, false));
    initialize(bank);
}

Memory::Memory(word ramStart, word ramSize, word romStart, word romSize, MemoryBank&& bank)
    : Memory(ramStart, ramSize, romStart, romSize, bank)
{
}

MemoryBank Memory::findBankForAddress(size_t addr) const
{
    static MemoryBank dummy;
    MemoryBank ret;
    for (auto& bank : m_banks) {
        if (bank.mapped(addr)) {
            ret = bank;
            return ret;
        }
    }
    return dummy;
}

MemoryBank Memory::findBankForBlock(size_t addr, size_t size) const
{
    static MemoryBank dummy;
    MemoryBank ret = findBankForAddress(addr);
    if (ret.valid() && !ret.fits(addr, size)) {
        ret = dummy;
    }
    return ret;
}

MemoryBank Memory::bank(word addr) const
{
    return findBankForBlock(addr, 0);
}

MemoryBanks Memory::banks() const
{
    return MemoryBanks(m_banks);
}

word Memory::start() const
{
    return (m_banks.begin() != m_banks.end()) ? m_banks.begin()->start() : 0xFFFF;
}

bool Memory::disjointFromAll(size_t addr, size_t size) const
{
    for (auto const& bank : m_banks) {
        if (bank.disjointFrom(addr, size)) {
            return true;
        }
    }
    return true;
}

void Memory::erase()
{
    for (auto& bank : m_banks) {
        MemoryBank b(bank);
        b.erase();
    }
}

bool Memory::add(word address, word size, bool writable, const byte* contents)
{
    MemoryBank b = findBankForBlock(address, size);
    if (b.valid()) {
        b.copy(address, size, contents);
    } else if (disjointFromAll(address, size)) {
        m_banks.emplace(address, size, writable, contents);
        sendEvent(EV_CONFIGCHANGED);
    } else {
        return false;
    }
    if (contents) {
        sendEvent(EV_IMAGELOADED);
    }
    return true;
}

bool Memory::add(MemoryBank&& bank)
{
    if (!bank.size()) {
        return true;
    }
    MemoryBank b = findBankForBlock(bank.start(), bank.size());
    if (b.valid()) {
        b.copy(bank);
    } else if (disjointFromAll(bank.start(), bank.size())) {
        m_banks.emplace(bank);
        sendEvent(EV_CONFIGCHANGED);
    } else {
        return false;
    }
    sendEvent(EV_IMAGELOADED);
    return true;
}

bool Memory::add(MemoryBank& bank)
{
    if (!bank.size()) {
        return true;
    }
    MemoryBank b = findBankForBlock(bank.start(), bank.size());
    if (b.valid()) {
        b.copy(bank);
    } else if (disjointFromAll(bank.start(), bank.size())) {
        m_banks.insert(std::move(bank));
        sendEvent(EV_CONFIGCHANGED);
    } else {
        return false;
    }
    sendEvent(EV_IMAGELOADED);
    return true;
}

bool Memory::remove(MemoryBank& bank)
{
    if (!bank.valid()) {
        return false;
    }
    m_banks.erase(bank);
    sendEvent(EV_CONFIGCHANGED);
    return true;
}

bool Memory::initialize()
{
    m_banks.clear();
    return true;
}

bool Memory::initialize(word address, word size, const byte* contents, bool writable)
{
    return initialize(MemoryBank(address, size, writable, contents));
}

bool Memory::initialize(MemoryBank& bank)
{
    erase();
    return add(bank);
}

bool Memory::initialize(MemoryBank&& bank)
{
    return initialize(bank);
}

bool Memory::inRAM(word addr) const
{
    MemoryBank bank = findBankForAddress(addr);
    return bank.valid() && bank.writable();
}

bool Memory::inROM(word addr) const
{
    MemoryBank bank = findBankForAddress(addr);
    return bank.valid() && !bank.writable();
}

bool Memory::isMapped(word addr) const
{
    MemoryBank b(findBankForAddress(addr));
    return b.valid();
}

byte& Memory::operator[](std::size_t addr)
{
    static byte dummy = 0xFF;
    MemoryBank bank(findBankForAddress(addr));
    if (bank.valid()) {
        return bank[addr];
    } else {
        return dummy;
    }
}

const byte& Memory::operator[](std::size_t addr) const
{
    static byte dummy = 0xFF;
    MemoryBank bank(findBankForAddress(addr));
    if (bank.valid()) {
        return bank[addr];
    } else {
        return dummy;
    }
}

std::ostream& Memory::status(std::ostream& os)
{
    char buf[80];
    snprintf(buf, 80, "%1x. M  %04x   CONTENTS %1x. [%02x]", id(), getValue(),
        MEM_ID, (isMapped(getValue()) ? (*this)[getValue()] : 0xFF));
    os << buf << std::endl;
    return os;
}

SystemError Memory::onRisingClockEdge()
{
    if ((!bus()->xdata() || !bus()->xaddr() || (!bus()->io() && (bus()->opflags() & SystemBus::IOOut))) && (bus()->getID() == MEM_ID)) {
        if (!isMapped(getValue())) {
            return error(ProtectedMemory);
        }
        bus()->putOnAddrBus(0x00);
        bus()->putOnDataBus((*this)[getValue()]);
    }
    return NoError;
}

SystemError Memory::onHighClock()
{
    if (((!bus()->xdata() || !bus()->xaddr()) && (bus()->putID() == MEM_ID)) || (!bus()->io() && (bus()->opflags() & SystemBus::IOIn) && (bus()->getID() == MEM_ID))) {
        if (!inRAM(getValue())) {
            return error(ProtectedMemory);
        }
        (*this)[getValue()] = bus()->readDataBus();
        sendEvent(EV_CONTENTSCHANGED);
    } else if (bus()->putID() == ADDR_ID) {
        if (!(bus()->xaddr())) {
            setValue(((word)bus()->readAddrBus() << 8) | ((word)bus()->readDataBus()));
        } else if (!(bus()->xdata())) {
            if (!(bus()->opflags() & SystemBus::MSB)) {
                setValue((getValue() & 0xFF00) | bus()->readDataBus());
            } else {
                setValue((getValue() & 0x00FF) | (((word)bus()->readDataBus()) << 8));
            }
        }
    }
    return NoError;
}

}
