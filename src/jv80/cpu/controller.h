/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <jv80/cpu/register.h>
#include <jv80/cpu/registers.h>
#include <jv80/cpu/systembus.h>
#include <vector>

namespace Obelix::JV80::CPU {

enum AddressingMode {
    Implied = 0x00,
    ImpliedByte = 0x01,
    ImpliedWord = 0x02,
    ImmediateByte = 0x11,
    ImmediateWord = 0x21,
    IndirectByte = 0x12,
    IndirectWord = 0x22,
    Mask = 0x7F,
    Done = 0x80
};

struct MicroCode {
    enum Action {
        XDATA = 0x01,
        XADDR = 0x02,
        IO = 0x04,
        OTHER = 0x08
    };

    enum Op {
        None = 0x00,
        And = 0x01,
        Nand = 0x02,
    };

    struct MicroCodeStep {
        Action action;
        byte src;
        byte target;
        byte opflags;
    };

    byte opcode;
    const char* instruction;
    AddressingMode addressingMode;
    byte target;
    byte condition;
    Op condition_op;
    MicroCodeStep steps[24];
};

class MicroCodeRunner {
public:
    MicroCodeRunner(class Controller&, SystemBus&, const MicroCode*);
    SystemError executeNextStep(int step);
    bool hasStep(int step);
    bool grabConstant(int step);
    [[nodiscard]] std::string instruction() const;
    [[nodiscard]] word constant() const;
    [[nodiscard]] bool complete() const { return m_complete; }

private:
    Controller& m_controller;
    SystemBus& m_bus;
    const MicroCode* m_mc;
    std::vector<MicroCode::MicroCodeStep> m_steps;
    bool m_valid { true };
    word m_constant { 0 };
    bool m_complete { false };

    void evaluateCondition();
    void fetchSteps();
    void fetchImmediateByte();
    void fetchImmediateWord();
    void fetchIndirectByte();
    void fetchIndirectWord();
};

class Controller : public Register {
public:
    explicit Controller(const MicroCode*);
    [[nodiscard]] std::string name() const override { return "IR"; }

    [[nodiscard]] word pc() const { return m_pc; }
    [[nodiscard]] std::string instruction() const;
    [[nodiscard]] word constant() const;
    [[nodiscard]] byte scratch() const { return m_scratch; }
    [[nodiscard]] word interruptVector() const { return m_interruptVector; }
    [[nodiscard]] int getStep() const { return m_step; }
    [[nodiscard]] SystemBus::RunMode runMode() const { return bus()->runMode(); }
    void setRunMode(SystemBus::RunMode runMode) { bus()->setRunMode(runMode); }
    [[nodiscard]] std::string instructionWithOpcode(int) const;
    int opcodeForInstruction(std::string&& instr) const { return opcodeForInstruction(instr); }
    [[nodiscard]] int opcodeForInstruction(const std::string& instr) const;

    [[nodiscard]] std::string to_string() const override;
    SystemError reset() override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;
    SystemError onLowClock() override;

    constexpr static int EV_STEPCHANGED = 0x05;
    constexpr static int EV_AFTERINSTRUCTION = 0x06;

private:
    byte m_step { 0 };
    byte m_scratch { 0 };
    word m_interruptVector { 0xFFFF };
    bool m_servicingNMI { false };
    const MicroCode* m_microCode;
    std::optional<MicroCodeRunner> m_runner {};
    int m_suspended { 0 };
    word m_pc { 0 };
};

}
