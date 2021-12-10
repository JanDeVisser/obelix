/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cpu/register.h>
#include <cpu/registers.h>
#include <cpu/systembus.h>
#include <vector>

namespace Obelix::JV80::CPU {

enum AddressingMode {
    Immediate = 0x00,
    ImmediateByte = 0x01,
    ImmediateWord = 0x02,
    DirectByte = 0x11,
    DirectWord = 0x21,
    AbsoluteByte = 0x12,
    AbsoluteWord = 0x22,
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

class MicroCodeRunner;

class Controller : public Register {
private:
    byte step = 0;
    byte m_scratch = 0;
    word m_interruptVector = 0xFFFF;
    bool m_servicingNMI = false;
    const MicroCode* microCode;
    MicroCodeRunner* m_runner = nullptr;
    int m_suspended = 0;

public:
    explicit Controller(const MicroCode*);
    std::string name() const override { return "IR"; }
    int alias() const override { return CONTROLLER; }

    std::string instruction() const;
    word constant() const;
    byte scratch() const { return m_scratch; }
    word interruptVector() const { return m_interruptVector; }
    int getStep() const { return step; }
    SystemBus::RunMode runMode() const { return bus()->runMode(); }
    void setRunMode(SystemBus::RunMode runMode) { bus()->setRunMode(runMode); }
    std::string instructionWithOpcode(int) const;
    int opcodeForInstruction(std::string&& instr) const { return opcodeForInstruction(instr); }
    int opcodeForInstruction(const std::string& instr) const;

    std::ostream& status(std::ostream&) override;
    SystemError reset() override;
    SystemError onRisingClockEdge() override;
    SystemError onHighClock() override;
    SystemError onLowClock() override;
    //  void           NMI();

    constexpr static int EV_STEPCHANGED = 0x02;
    constexpr static int EV_AFTERINSTRUCTION = 0x03;
};

class MicroCodeRunner {
private:
    Controller* m_controller;
    SystemBus* m_bus;
    const MicroCode* mc;
    std::vector<MicroCode::MicroCodeStep> steps;
    bool valid = true;
    word m_constant = 0;
    bool m_complete = false;

    void evaluateCondition();
    void fetchSteps();
    void fetchDirectByte();
    void fetchDirectWord();
    void fetchAbsoluteByte();
    void fetchAbsoluteWord();

public:
    MicroCodeRunner(Controller*, SystemBus*, const MicroCode*);
    SystemError executeNextStep(int step);
    bool hasStep(int step);
    bool grabConstant(int step);
    std::string instruction() const;
    word constant() const;
    bool complete() const { return m_complete; }
};
}
