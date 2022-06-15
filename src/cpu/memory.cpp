/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <cstring>
#include <memory>

#include "memory.h"

namespace Obelix::CPU {

class Block {
public:
    explicit Block(size_t size)
        : m_size(size)
        , m_bytes((size) ? new byte[size] : nullptr)
    {
    }

    ~Block()
    {
        delete[] m_bytes;
    }

    [[nodiscard]] byte* operator*() const { return m_bytes; }
    byte& operator[](size_t ix)
    {
        assert(m_bytes && ix < m_size);
        return m_bytes[ix];
    }
    byte const& operator[](size_t ix) const
    {
        assert(m_bytes && ix < m_size);
        return m_bytes[ix];
    }
    void copy(byte const* bytes, size_t size)
    {
        memcpy(m_bytes, bytes, std::min(m_size, size));
    }
    void erase()
    {
        memset(m_bytes, 0, m_size);
    }

private:
    size_t m_size { 0 };
    byte* m_bytes { nullptr };
};

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
    m_image = std::make_shared<Block>(size);
    if (image) {
        m_image->copy(image, size);
    } else {
        erase();
    }
}

SystemError MemoryBank::poke(std::size_t addr, byte value)
{
    if (mapped(addr) && writable()) {
        (*m_image)[offset(addr)] = value;
        return {};
    } else {
        return SystemErrorCode::ProtectedMemory;
    }
}

ErrorOr<byte, SystemErrorCode> MemoryBank::peek(std::size_t addr) const
{
    if (mapped(addr)) {
        return (*m_image)[offset(addr)];
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
    m_image->erase();
}

void MemoryBank::copy(size_t addr, size_t size, const byte* contents)
{
    if (fits(addr, size)) {
        m_image->copy(contents, size);
    }
}

bool MemoryBank::mapped(size_t addr) const
{
    return (start() <= addr) && (addr < end());
}

bool MemoryBank::fits(size_t addr, size_t size) const
{
    return mapped(addr) && mapped(addr + size - 1);
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
    for (auto ix = 0u; ix < m_banks.size(); ++ix) {
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
    auto bank_ix = findBankForAddress(addr);
    if (!bank_ix.has_value()) {
        std::cout << format("poke({04x}, {02x}): Unmapped address.\n", addr, value);
        return SystemErrorCode::ProtectedMemory;
    }
    auto bank = m_banks[bank_ix.value()];
    if (!m_banks[bank_ix.value()].writable()) {
        std::cout << format("poke({04x}, {02x}): Read-only bank {04x}-{04x}.\n", addr, value, bank.start(), bank.end());
        return SystemErrorCode::ProtectedMemory;
    }
    return bank.poke(addr, value);
}

ErrorOr<byte, SystemErrorCode> Memory::peek(std::size_t addr) const
{
    auto bank_ix = findBankForAddress(addr);
    if (!bank_ix.has_value()) {
        std::cout << format("peek({04x}): Unmapped address.\n", addr);
        return SystemErrorCode::ProtectedMemory;
    }
    return m_banks[bank_ix.value()].peek(addr);
}

std::string Memory::to_string() const
{
    return format("{01x}. M  {04x}   CONTENTS {01x}. [{02x}]", address(), getValue(),
        MEM_ID, (isMapped(getValue()) ? peek(getValue()).value() : 0xFF));
}

SystemError Memory::onRisingClockEdge()
{
    if ((!bus()->xdata() || !bus()->xaddr() || (!bus()->io() && (bus()->opflags() & SystemBus::IOOut))) && (bus()->getAddress() == MEM_ID)) {
        if (!isMapped(getValue())) {
            std::cout << format("onRisingClockEdge(): Unmapped address {04x}.\n", getValue());
            return SystemErrorCode::ProtectedMemory;
        }
        bus()->putOnAddrBus(0x00);
        bus()->putOnDataBus(peek(getValue()).value());
        if (bus()->opflags() & SystemBus::INC) {
            setValue(AddressRegister::getValue() + 1);
        }
        if (bus()->opflags() & SystemBus::DEC) {
            setValue(AddressRegister::getValue() - 1);
        }
    }
    return {};
}

SystemError Memory::onHighClock()
{
    if (((!bus()->xdata() || !bus()->xaddr()) && (bus()->putAddress() == MEM_ID)) || (!bus()->io() && (bus()->opflags() & SystemBus::IOIn) && (bus()->getAddress() == MEM_ID))) {
        TRY_RETURN(poke(getValue(), bus()->readDataBus()));
        if (bus()->opflags() & SystemBus::INC) {
            setValue(AddressRegister::getValue() + 1);
        }
        if (bus()->opflags() & SystemBus::DEC) {
            setValue(AddressRegister::getValue() - 1);
        }
        sendEvent(EV_CONTENTSCHANGED);
    } else if (bus()->putAddress() == ADDR_ID) {
        if (!(bus()->xaddr())) {
            setValue(((word)bus()->readAddrBus() << 8) | ((word)bus()->readDataBus()));
        } else if (!(bus()->xdata())) {
            if (bus()->opflags() & SystemBus::IDX) {
                auto idx = (signed char)bus()->readDataBus();
                setValue(getValue() + idx);
            } else if (bus()->opflags() & SystemBus::MSB) {
                setValue((getValue() & 0x00FF) | (((word)bus()->readDataBus()) << 8));
            } else {
                setValue((getValue() & 0xFF00) | bus()->readDataBus());
            }
        }
    }
    return {};
}

}
