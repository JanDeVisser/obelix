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
public:
    MemoryBank() = default;
    MemoryBank(word, word, bool = true, const byte* = nullptr) noexcept;
    ~MemoryBank() = default;

    [[nodiscard]] ErrorOr<byte, SystemErrorCode> peek(std::size_t) const;
    SystemError poke(std::size_t, byte);
    bool operator<(const MemoryBank&) const;
    bool operator<=(const MemoryBank&) const;
    bool operator>(const MemoryBank&) const;
    bool operator>=(const MemoryBank&) const;
    bool operator==(const MemoryBank&) const;
    bool operator!=(const MemoryBank&) const;

    [[nodiscard]] std::string name() const;
    [[nodiscard]] bool mapped(size_t) const;
    [[nodiscard]] bool fits(size_t, size_t) const;
    [[nodiscard]] bool disjointFrom(size_t, size_t) const;
    void erase();
    void copy(size_t, size_t, const byte*);
    [[nodiscard]] word offset(size_t addr) const { return addr - start(); }
    [[nodiscard]] bool valid() const { return m_size > 0; }
    [[nodiscard]] word start() const { return m_start; }
    [[nodiscard]] word size() const { return m_size; }
    [[nodiscard]] word end() const { return m_start + m_size; }
    [[nodiscard]] bool writable() const { return m_writable; }

private:
    word m_start { 0 };
    word m_size { 0 };
    bool m_writable { false };
    std::shared_ptr<class Block> m_image;
};

using MemoryBanks = std::vector<MemoryBank>;

class Memory : public AddressRegister {
public:
    Memory();
    Memory(word, word, word, word);
    ~Memory() override = default;

    [[nodiscard]] ErrorOr<byte, SystemErrorCode> peek(std::size_t) const;
    SystemError poke(std::size_t, byte);

    void erase();
    bool add(word, word, bool = true, const byte* = nullptr);
    bool remove(word, word);
    bool initialize(word, word, const byte* = nullptr, bool = true);
    [[nodiscard]] word start() const;
    [[nodiscard]] bool disjointFromAll(size_t, size_t) const;

    [[nodiscard]] bool inRAM(word) const;
    [[nodiscard]] bool inROM(word) const;
    [[nodiscard]] bool isMapped(word) const;

    [[nodiscard]] std::string to_string() const override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;

    constexpr static byte MEM_ID = 0x7;
    constexpr static byte ADDR_ID = 0xF;
    constexpr static int EV_CONTENTSCHANGED = 0x09;
    constexpr static int EV_IMAGELOADED = 0x0A;
    constexpr static int EV_CONFIGCHANGED = 0x0B;

    [[nodiscard]] MemoryBanks const& banks() const { return m_banks; }
    [[nodiscard]] std::optional<size_t> findBankForAddress(size_t) const;
    [[nodiscard]] std::optional<size_t> findBankForBlock(size_t, size_t) const;
    [[nodiscard]] MemoryBank const& bank(size_t) const;

private:
    MemoryBanks m_banks;
};

}
