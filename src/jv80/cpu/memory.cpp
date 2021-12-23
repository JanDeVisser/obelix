/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cstring>
#include <memory>

#include <jv80/cpu/memory.h>

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

    m_image = new byte[size];
    if (image) {
        memcpy(m_image, image, size);
    } else {
        erase();
    }
}

MemoryBank::~MemoryBank()
{
    delete[] m_image;
}

SystemError MemoryBank::poke(std::size_t addr, byte value)
{
    if (mapped(addr) && writable()) {
        m_image[offset(addr)] = value;
        return {};
    } else {
        return SystemErrorCode::ProtectedMemory;
    }
}

ErrorOr<byte, SystemErrorCode> MemoryBank::peek(std::size_t addr) const
{
    if (mapped(addr)) {
        return m_image[offset(addr)];
    } else {
        return SystemErrorCode::ProtectedMemory;
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
    return format("{} {04x}-{04x}", ((writable()) ? "RAM" : "ROM"), start(), end());
}

void MemoryBank::erase()
{
    memset(m_image, 0, m_size);
}

void MemoryBank::copy(size_t addr, size_t size, const byte* contents)
{
    if (fits(addr, size)) {
        for (auto ix = 0; ix < size; ix++) {
            m_image[offset(addr) + ix] = contents[ix];
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

Memory::Memory(word ramStart, word ramSize, word romStart, word romSize)
    : Memory()
{
    add(ramStart, ramSize, true);
    add(romStart, romSize, false);
}

std::optional<size_t> Memory::findBankForAddress(size_t addr) const
{
    for (auto ix = 0; ix < m_banks.size(); ++ix) {
        auto& bank = m_banks[ix];
        if (bank.mapped(addr))
            return ix;
    }
    return {};
}

std::optional<size_t> Memory::findBankForBlock(size_t addr, size_t size) const
{
    for (size_t ix = 0; ix < m_banks.size(); ++ix) {
        auto& bank = m_banks[ix];
        if (bank.fits(addr, size))
            return ix;
    }
    return {};
}

MemoryBank const& Memory::bank(size_t addr) const
{
    auto ix = findBankForAddress(addr);
    assert(ix.has_value());
    return m_banks[ix.value()];
}

word Memory::start() const
{
    return (!m_banks.empty()) ? (*m_banks.begin()).start() : 0xFFFF;
}

bool Memory::disjointFromAll(size_t addr, size_t size) const
{
    return std::all_of(m_banks.begin(), m_banks.end(),
        [addr, size](auto& bank) { return bank.disjointFrom(addr, size); });
}

void Memory::erase()
{
    for (auto& bank : m_banks) {
        bank.erase();
    }
}

bool Memory::add(word address, word size, bool writable, const byte* contents)
{
    auto b = findBankForBlock(address, size);
    if (b.has_value()) {
        m_banks[b.value()].copy(address, size, contents);
    } else if (disjointFromAll(address, size)) {
        m_banks.emplace_back(address, size, writable, contents);
        sendEvent(EV_CONFIGCHANGED);
    } else {
        return false;
    }
    if (contents) {
        sendEvent(EV_IMAGELOADED);
    }
    return true;
}

bool Memory::remove(word addr, word size)
{
    auto ret = std::remove_if(m_banks.begin(), m_banks.end(), [addr, size](auto& bank) {
        return (bank.start() == addr) && (bank.size() == size);
    });
    return ret != m_banks.end();
}

bool Memory::initialize(word address, word size, const byte* contents, bool writable)
{
    return add(address, size, writable, contents);
}

bool Memory::inRAM(word addr) const
{
    auto bank_ix = findBankForAddress(addr);
    return bank_ix.has_value() && m_banks[bank_ix.value()].writable();
}

bool Memory::inROM(word addr) const
{
    auto bank_ix = findBankForAddress(addr);
    return bank_ix.has_value() && !m_banks[bank_ix.value()].writable();
}

bool Memory::isMapped(word addr) const
{
    return std::any_of(m_banks.begin(), m_banks.end(), [addr](auto& bank) {
        return bank.mapped(addr);
    });
}

SystemError Memory::poke(std::size_t addr, byte value)
{
    auto bank = findBankForAddress(addr);
    if (bank.has_value() && m_banks[bank.value()].writable()) {
        return m_banks[bank.value()].poke(addr, value);
    } else {
        return SystemErrorCode::ProtectedMemory;
    }
}

ErrorOr<byte, SystemErrorCode> Memory::peek(std::size_t addr) const
{
    auto bank = findBankForAddress(addr);
    if (bank.has_value()) {
        return m_banks[bank.value()].peek(addr);
    } else {
        return SystemErrorCode::ProtectedMemory;
    }
}

std::string Memory::to_string() const
{
    return format("{01x}. M  {04x}   CONTENTS {01x}. [{02x}]", id(), getValue(),
        MEM_ID, (isMapped(getValue()) ? peek(getValue()).value() : 0xFF));
}

SystemError Memory::onRisingClockEdge()
{
    if ((!bus()->xdata() || !bus()->xaddr() || (!bus()->io() && (bus()->opflags() & SystemBus::IOOut))) && (bus()->getID() == MEM_ID)) {
        if (!isMapped(getValue())) {
            return SystemErrorCode::ProtectedMemory;
        }
        bus()->putOnAddrBus(0x00);
        bus()->putOnDataBus(peek(getValue()).value());
    }
    return {};
}

SystemError Memory::onHighClock()
{
    if (((!bus()->xdata() || !bus()->xaddr()) && (bus()->putID() == MEM_ID)) || (!bus()->io() && (bus()->opflags() & SystemBus::IOIn) && (bus()->getID() == MEM_ID))) {
        TRY_RETURN(poke(getValue(), bus()->readDataBus()));
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
    return {};
}

}
