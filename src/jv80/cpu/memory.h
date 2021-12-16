/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

//
// Created by jan on 2021-01-25.
//

#pragma once

#include <cstring>
#include <memory>
#include <set>
#include <vector>

#include <jv80/cpu/addressregister.h>
#include <jv80/cpu/systembus.h>

namespace Obelix::JV80::CPU {

class MemoryBank {
private:
    word m_start = 0;
    word m_size = 0;
    bool m_writable = false;
    std::shared_ptr<byte> m_image = nullptr;

public:
    MemoryBank() = default;
    MemoryBank(const MemoryBank&);
    MemoryBank(MemoryBank&&) noexcept;
    MemoryBank(word, word, bool = true, const byte* = nullptr) noexcept;
    ~MemoryBank() = default;

    MemoryBank& operator=(const MemoryBank&);
    MemoryBank& operator=(MemoryBank&&) noexcept;
    byte& operator[](std::size_t);
    const byte& operator[](std::size_t) const;
    bool operator<(const MemoryBank&) const;
    bool operator<=(const MemoryBank&) const;
    bool operator>(const MemoryBank&) const;
    bool operator>=(const MemoryBank&) const;
    bool operator==(const MemoryBank&) const;
    bool operator!=(const MemoryBank&) const;

    std::string name() const;
    bool mapped(size_t) const;
    bool fits(size_t, size_t) const;
    bool disjointFrom(size_t, size_t) const;
    void erase();
    void copy(size_t, size_t, const byte*);
    void copy(MemoryBank&);
    void copy(MemoryBank&&);
    word offset(size_t addr) const { return addr - start(); }
    bool valid() const { return m_size > 0; }
    word start() const { return m_start; }
    word size() const { return m_size; }
    word end() const { return m_start + m_size; }
    bool writable() const { return m_writable; }
};

typedef std::set<MemoryBank> MemoryBanks;

class Memory : public AddressRegister {
private:
    MemoryBanks m_banks;

    MemoryBank findBankForAddress(size_t) const;
    MemoryBank findBankForBlock(size_t, size_t) const;

public:
    Memory();
    Memory(Memory&) = delete;
    Memory(Memory&&) = delete;
    Memory(word, word, word, word, MemoryBank&& = MemoryBank());
    Memory(word, word, word, word, MemoryBank&);
    explicit Memory(MemoryBank&&);
    explicit Memory(MemoryBank&);
    ~Memory() override = default;

    byte& operator[](std::size_t);
    const byte& operator[](std::size_t) const;

    void erase();
    bool add(word, word, bool = true, const byte* = nullptr);
    bool add(MemoryBank&&);
    bool add(MemoryBank&);
    bool remove(MemoryBank&);
    bool initialize(word, word, const byte* = nullptr, bool = true);
    bool initialize();
    bool initialize(MemoryBank&&);
    bool initialize(MemoryBank&);
    MemoryBanks banks() const;
    MemoryBank bank(word) const;
    word start() const;
    bool disjointFromAll(size_t, size_t) const;

    bool inRAM(word) const;
    bool inROM(word) const;
    bool isMapped(word) const;

    std::ostream& status(std::ostream&) override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;

    constexpr static byte MEM_ID = 0x7;
    constexpr static byte ADDR_ID = 0xF;
    constexpr static int EV_CONTENTSCHANGED = 0x04;
    constexpr static int EV_IMAGELOADED = 0x05;
    constexpr static int EV_CONFIGCHANGED = 0x06;
};

}
