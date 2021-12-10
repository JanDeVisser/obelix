/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <cpu/alu.h>
#include <cpu/controller.h>
#include <cpu/opcodes.h>
#include <cpu/registers.h>

#define PUSH_REG(reg, opflag)                                                                \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Inc }, \
    {                                                                                        \
        .action = MicroCode::XDATA, .src = reg, .target = MEM, .opflags = opflag             \
    }

#define POP_REG(reg, opflag)                                                                 \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Dec }, \
    {                                                                                        \
        .action = MicroCode::XDATA, .src = MEM, .target = reg, .opflags = opflag             \
    }

#define PUSH_ADDR(reg, opflag)                                                                    \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Inc },      \
        { .action = MicroCode::XDATA, .src = reg, .target = MEM, .opflags = SystemBus::None },    \
        { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Inc },  \
    {                                                                                             \
        .action = MicroCode::XDATA, .src = reg, .target = MEM, .opflags = SystemBus::MSB | opflag \
    }

#define POP_ADDR(reg, opflag)                                                                    \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Dec },     \
        { .action = MicroCode::XDATA, .src = MEM, .target = reg, .opflags = SystemBus::MSB },    \
        { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Dec }, \
    {                                                                                            \
        .action = MicroCode::XDATA, .src = MEM, .target = reg, .opflags = opflag                 \
    }

#define ALU_OP(op, instr, lhs, rhs, aluOp)                                                         \
    {                                                                                              \
        .opcode = op, .instruction = instr, .addressingMode = Immediate,                           \
        .steps = {                                                                                 \
            { .action = MicroCode::XDATA, .src = lhs, .target = LHS, .opflags = SystemBus::None }, \
            { .action = MicroCode::XDATA, .src = rhs, .target = RHS, .opflags = ALU::aluOp },      \
            { .action = MicroCode::XDATA, .src = LHS, .target = lhs, .opflags = SystemBus::Done }, \
        }                                                                                          \
    }

#define ALU_UNARY_OP(op, instr, reg, aluOp)                                                        \
    {                                                                                              \
        .opcode = op, .instruction = instr, .addressingMode = Immediate,                           \
        .steps = {                                                                                 \
            { .action = MicroCode::XDATA, .src = reg, .target = RHS, .opflags = ALU::aluOp },      \
            { .action = MicroCode::XDATA, .src = LHS, .target = reg, .opflags = SystemBus::Done }, \
        }                                                                                          \
    }

#define SWAP(op, instr, reg1, reg2)                                                                  \
    {                                                                                                \
        .opcode = op, .instruction = instr, .addressingMode = Immediate,                             \
        .steps = {                                                                                   \
            { .action = MicroCode::XDATA, .src = reg1, .target = TX, .opflags = SystemBus::None },   \
            { .action = MicroCode::XDATA, .src = reg2, .target = reg1, .opflags = SystemBus::None }, \
            { .action = MicroCode::XDATA, .src = TX, .target = reg2, .opflags = SystemBus::Done },   \
        }                                                                                            \
    }

#define ALU_WIDE_OP(op, instr, aluOp, aluOpWithCarry)                                                   \
    {                                                                                                   \
        .opcode = op, .instruction = instr, .addressingMode = Immediate,                                \
        .steps = {                                                                                      \
            { .action = MicroCode::XDATA, .src = GP_A, .target = LHS, .opflags = SystemBus::None },     \
            { .action = MicroCode::XDATA, .src = GP_C, .target = RHS, .opflags = ALU::aluOp },          \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_A, .opflags = SystemBus::None },     \
            { .action = MicroCode::XDATA, .src = GP_B, .target = LHS, .opflags = SystemBus::None },     \
            { .action = MicroCode::XDATA, .src = GP_D, .target = RHS, .opflags = ALU::aluOpWithCarry }, \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_B, .opflags = SystemBus::Done },     \
        }                                                                                               \
    }

#define CMP(op, instr, lhs, rhs)                                                                                      \
    {                                                                                                                 \
        .opcode = op, .instruction = instr, .addressingMode = Immediate,                                              \
        .steps = {                                                                                                    \
            { .action = MicroCode::XDATA, .src = lhs, .target = LHS, .opflags = SystemBus::None },                    \
            { .action = MicroCode::XDATA, .src = rhs, .target = RHS, .opflags = ((byte)ALU::SUB) | SystemBus::Done }, \
        }                                                                                                             \
    }

#define CMP_CONST(op, instr, lhs)                                                                                     \
    {                                                                                                                 \
        .opcode = op, .instruction = instr, .addressingMode = Immediate, .target = TX,                                \
        .steps = {                                                                                                    \
            { .action = MicroCode::XDATA, .src = lhs, .target = LHS, .opflags = SystemBus::None },                    \
            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::Inc },                  \
            { .action = MicroCode::XDATA, .src = MEM, .target = RHS, .opflags = ((byte)ALU::SUB) | SystemBus::Done }, \
        }                                                                                                             \
    }

#define ALU_OP_CONST(op, instr, lhs, aluOp)                                                          \
    {                                                                                                \
        .opcode = op, .instruction = instr, .addressingMode = Immediate,                             \
        .steps = {                                                                                   \
            { .action = MicroCode::XDATA, .src = lhs, .target = LHS, .opflags = SystemBus::None },   \
            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::Inc }, \
            { .action = MicroCode::XDATA, .src = MEM, .target = RHS, .opflags = ALU::aluOp },        \
            { .action = MicroCode::XDATA, .src = LHS, .target = lhs, .opflags = SystemBus::Done },   \
        }                                                                                            \
    }

#define OUT(op, instr, reg)                                                                                                    \
    {                                                                                                                          \
        .opcode = op, .instruction = instr, .addressingMode = DirectByte, .target = CONTROLLER,                                \
        .steps = {                                                                                                             \
            { .action = MicroCode::IO, .src = reg, .target = DEREFCONTROLLER, .opflags = SystemBus::IOOut | SystemBus::Done }, \
        }                                                                                                                      \
    }

#define IN(op, instr, reg)                                                                                                    \
    {                                                                                                                         \
        .opcode = op, .instruction = instr, .addressingMode = DirectByte, .target = CONTROLLER,                               \
        .steps = {                                                                                                            \
            { .action = MicroCode::IO, .src = reg, .target = DEREFCONTROLLER, .opflags = SystemBus::IOIn | SystemBus::Done }, \
        }                                                                                                                     \
    }

namespace Obelix::JV80::CPU {

constexpr static MicroCode mc[256] = {
    { .opcode = NOP, .instruction = "nop", .addressingMode = static_cast<AddressingMode>(Immediate | Done) },

    { .opcode = MOV_A_CONST, .instruction = "mov a,#%02x", .addressingMode = static_cast<AddressingMode>(DirectByte | Done), .target = GP_A },

    { .opcode = MOV_A_ADDR, .instruction = "mov a,*%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteByte | Done), .target = GP_A },

    { .opcode = MOV_A_B, .instruction = "mov a,b", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_B, .target = GP_A, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_A_C, .instruction = "mov a,c", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_C, .target = GP_A, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_A_D, .instruction = "mov a,d", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_D, .target = GP_A, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_B_CONST, .instruction = "mov b,#%02x", .addressingMode = static_cast<AddressingMode>(DirectByte | Done), .target = GP_B },

    { .opcode = MOV_B_ADDR, .instruction = "mov b,*%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteByte | Done), .target = GP_B },

    { .opcode = MOV_B_A, .instruction = "mov b,a", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_A, .target = GP_B, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_B_C, .instruction = "mov b,c", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_C, .target = GP_B, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_B_D, .instruction = "mov b,d", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_D, .target = GP_B, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_C_CONST, .instruction = "mov c,#%02x", .addressingMode = static_cast<AddressingMode>(DirectByte | Done), .target = GP_C },

    { .opcode = MOV_C_ADDR, .instruction = "mov c,*%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteByte | Done), .target = GP_C },

    { .opcode = MOV_C_A, .instruction = "mov c,a", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_B, .target = GP_C, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_C_B, .instruction = "mov c,b", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_B, .target = GP_C, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_C_D, .instruction = "mov c,d", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_D, .target = GP_C, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_D_CONST, .instruction = "mov d,#%02x", .addressingMode = static_cast<AddressingMode>(DirectByte | Done), .target = GP_D },

    { .opcode = MOV_D_ADDR, .instruction = "mov d,*%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteByte | Done), .target = GP_D },

    { .opcode = MOV_D_A, .instruction = "mov d,a", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_A, .target = GP_D, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_D_B, .instruction = "mov d,b", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_B, .target = GP_D, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_D_C, .instruction = "mov d,c", .addressingMode = Immediate, .steps = {
                                                                                    { .action = MicroCode::XDATA, .src = GP_C, .target = GP_D, .opflags = SystemBus::Done },
                                                                                } },

    { .opcode = MOV_SP_CONST, .instruction = "mov sp,#%04x", .addressingMode = static_cast<AddressingMode>(DirectWord | Done), .target = SP },

    { .opcode = MOV_SP_ADDR, .instruction = "mov sp,*%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = SP },

    { .opcode = MOV_SP_SI, .instruction = "mov sp,si", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Si, .target = SP, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_SI_CONST, .instruction = "mov si,#%04x", .addressingMode = static_cast<AddressingMode>(AddressingMode::DirectWord | AddressingMode::Done), .target = Si },

    { .opcode = MOV_SI_ADDR, .instruction = "mov si,*%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = Si },

    { .opcode = MOV_SI_CD, .instruction = "mov si,cd", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XDATA, .src = GP_C, .target = Si, .opflags = SystemBus::None },
                                                                                        { .action = MicroCode::XDATA, .src = GP_D, .target = Si, .opflags = SystemBus::MSB | SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_DI_CONST, .instruction = "mov di,#%04x", .addressingMode = static_cast<AddressingMode>(DirectWord | Done), .target = Di },

    { .opcode = MOV_DI_ADDR, .instruction = "mov di,*%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = Di },

    { .opcode = MOV_DI_CD, .instruction = "mov di,cd", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XDATA, .src = GP_C, .target = Di, .opflags = SystemBus::None },
                                                                                        { .action = MicroCode::XDATA, .src = GP_D, .target = Di, .opflags = SystemBus::MSB | SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_A__SI, .instruction = "mov a,*si", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_A, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_B__SI, .instruction = "mov b,*si", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_B, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_C__SI, .instruction = "mov c,*si", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_C, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_D__SI, .instruction = "mov d,*si", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_D, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_A__DI, .instruction = "mov a,*di", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_A, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_B__DI, .instruction = "mov b,*di", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_B, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_C__DI, .instruction = "mov c,*di", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_C, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_D__DI, .instruction = "mov d,*di", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_D, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_DI_SI, .instruction = "mov *di,*si", .addressingMode = Immediate, .steps = {
                                                                                          { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                          { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::None },
                                                                                          { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                          { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done },
                                                                                      } },

    //  JMP             = 0x27,
    { .opcode = JMP, .instruction = "jmp #%04x", .addressingMode = static_cast<AddressingMode>(DirectWord | Done), .target = PC },

    //  JNZ             = 0x28,
    { .opcode = JNZ, .instruction = "jnz #%04x", .addressingMode = static_cast<AddressingMode>(DirectWord | Done), .target = PC, .condition = SystemBus::Z, .condition_op = MicroCode::Nand },

    //  JC              = 0x29,
    { .opcode = JC, .instruction = "jc #%04x", .addressingMode = static_cast<AddressingMode>(DirectWord | Done), .target = PC, .condition = SystemBus::C, .condition_op = MicroCode::And },

    //  JV              = 0x2A,
    { .opcode = JV, .instruction = "jv #%04x", .addressingMode = static_cast<AddressingMode>(DirectWord | Done), .target = PC, .condition = SystemBus::V, .condition_op = MicroCode::And },

    { .opcode = CALL, .instruction = "call #%04x", .addressingMode = DirectWord, .target = TX, // 0x2b
        .steps = {
            // TX Contains the address to jump to. PC has the address to return to (one past the address)

            // Push the return address:
            PUSH_ADDR(PC, SystemBus::None),

            // Load PC with the subroutine address:
            { .action = MicroCode::XADDR, .src = TX, .target = PC, .opflags = SystemBus::Done },

        } },

    { .opcode = RET, .instruction = "ret", .addressingMode = Immediate, // 0x02c
        .steps = { POP_ADDR(PC, SystemBus::Done) } },

    { .opcode = PUSH_A, .instruction = "push a", .addressingMode = Immediate, // 0x2d
        .steps = { PUSH_REG(GP_A, SystemBus::Done) } },

    { .opcode = PUSH_B, .instruction = "push b", .addressingMode = Immediate, // 0x2e
        .steps = { PUSH_REG(GP_B, SystemBus::Done) } },

    { .opcode = PUSH_C, .instruction = "push c", .addressingMode = Immediate, // 0x2f
        .steps = { PUSH_REG(GP_C, SystemBus::Done) } },

    { .opcode = PUSH_D, .instruction = "push d", .addressingMode = Immediate, // 0x30
        .steps = { PUSH_REG(GP_D, SystemBus::Done) } },

    { .opcode = PUSH_SI, .instruction = "push si", .addressingMode = Immediate, // 0x31
        .steps = { PUSH_ADDR(Si, SystemBus::Done) } },

    { .opcode = PUSH_DI, .instruction = "push di", .addressingMode = Immediate, // 0x32
        .steps = { PUSH_ADDR(Di, SystemBus::Done) } },

    { .opcode = POP_A, .instruction = "pop a", .addressingMode = Immediate, // 0x33
        .steps = { POP_REG(GP_A, SystemBus::Done) } },

    { .opcode = POP_B, .instruction = "pop b", .addressingMode = Immediate, // 0x34
        .steps = { POP_REG(GP_B, SystemBus::Done) } },

    { .opcode = POP_C, .instruction = "pop c", .addressingMode = Immediate, // 0x35
        .steps = { POP_REG(GP_C, SystemBus::Done) } },

    { .opcode = POP_D, .instruction = "pop d", .addressingMode = Immediate, // 0x36
        .steps = { POP_REG(GP_D, SystemBus::Done) } },

    { .opcode = POP_SI, .instruction = "pop si", .addressingMode = Immediate, // 0x37
        .steps = { POP_ADDR(Si, SystemBus::Done) } },

    { .opcode = POP_DI, .instruction = "pop di", .addressingMode = Immediate, // 0x38
        .steps = { POP_ADDR(Di, SystemBus::Done) } },

    { .opcode = MOV_ADDR_A, .instruction = "mov *%04x,a", .addressingMode = DirectWord, .target = MEMADDR, // 0x39
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_A, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV__DI_A, .instruction = "mov *di,a", .addressingMode = Immediate, // 0x3A
        .steps = {
            { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_A, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV_ADDR_B, .instruction = "mov *%04x,b", .addressingMode = DirectWord, .target = MEMADDR, // 0x3B
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_B, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV__DI_B, .instruction = "mov *di,b", .addressingMode = Immediate, // 0x3C
        .steps = {
            { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_B, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV_ADDR_C, .instruction = "mov *%04x,c", .addressingMode = DirectWord, .target = MEMADDR, // 0x3D
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV__DI_C, .instruction = "mov *di,c", .addressingMode = Immediate, // 0x3E
        .steps = {
            { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV_ADDR_D, .instruction = "mov *%04x,d", .addressingMode = DirectWord, .target = MEMADDR, // 0x3F
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV__DI_D, .instruction = "mov *di,d", .addressingMode = Immediate, // 0x40
        .steps = {
            { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::Done },
        } },

    { .opcode = MOV_ADDR_SI, .instruction = "mov *%04x,si", .addressingMode = DirectWord, .target = TX, // 0x41
        .steps = {
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = Si, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = Si, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        } },

    { .opcode = MOV_ADDR_DI, .instruction = "mov *%04x,di", .addressingMode = DirectWord, .target = TX, // 0x42
        .steps = {
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = Di, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = Di, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        } },

    { .opcode = MOV_ADDR_CD, .instruction = "mov *%04x,cd", .addressingMode = DirectWord, .target = TX, // 0x43
        .steps = {
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        } },

    { .opcode = MOV__SI_CD, .instruction = "mov *si,cd", .addressingMode = Immediate, // 0x44
        .steps = {
            { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        } },

    { .opcode = MOV__DI_CD, .instruction = "mov *di,cd", .addressingMode = Immediate, // 0x45
        .steps = {
            { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::Inc },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        } },

    // Arithmetic

    // A Register
    ALU_OP(ADD_A_B, "add a,b", GP_A, GP_B, ADD), ALU_OP(ADC_A_B, "adc a,b", GP_A, GP_B, ADC), ALU_OP(SUB_A_B, "sub a,b", GP_A, GP_B, SUB), ALU_OP(SBB_A_B, "sbb a,b", GP_A, GP_B, SBB), ALU_OP(AND_A_B, "and a,b", GP_A, GP_B, AND), ALU_OP(OR_A_B, "or a,b", GP_A, GP_B, OR), ALU_OP(XOR_A_B, "xor a,b", GP_A, GP_B, XOR), ALU_UNARY_OP(NOT_A, "not a", GP_A, NOT), ALU_UNARY_OP(SHL_A, "shl a", GP_A, SHL), ALU_UNARY_OP(SHR_A, "shr a", GP_A, SHR),

    ALU_OP(ADD_A_C, "add a,c", GP_A, GP_C, ADD), ALU_OP(ADC_A_C, "adc a,c", GP_A, GP_C, ADC), ALU_OP(SUB_A_C, "sub a,c", GP_A, GP_C, SUB), ALU_OP(SBB_A_C, "sbb a,c", GP_A, GP_C, SBB), ALU_OP(AND_A_C, "and a,c", GP_A, GP_C, AND), ALU_OP(OR_A_C, "or a,c", GP_A, GP_C, OR), ALU_OP(XOR_A_C, "xor a,c", GP_A, GP_C, XOR),

    ALU_OP(ADD_A_D, "add a,d", GP_A, GP_D, ADD), ALU_OP(ADC_A_D, "adc a,d", GP_A, GP_D, ADC), ALU_OP(SUB_A_D, "sub a,d", GP_A, GP_D, SUB), ALU_OP(SBB_A_D, "sbb a,d", GP_A, GP_D, SBB), ALU_OP(AND_A_D, "and a,d", GP_A, GP_D, AND), ALU_OP(OR_A_D, "or a,d", GP_A, GP_D, OR), ALU_OP(XOR_A_D, "xor a,d", GP_A, GP_D, XOR),

    // B Register
    ALU_OP(ADD_B_C, "add b,c", GP_B, GP_C, ADD), ALU_OP(ADC_B_C, "adc b,c", GP_B, GP_C, ADC), ALU_OP(SUB_B_C, "sub b,c", GP_B, GP_C, SUB), ALU_OP(SBB_B_C, "sbb b,c", GP_B, GP_C, SBB), ALU_OP(AND_B_C, "and b,c", GP_B, GP_C, AND), ALU_OP(OR_B_C, "or b,c", GP_B, GP_C, OR), ALU_OP(XOR_B_C, "xor b,c", GP_B, GP_C, XOR), ALU_UNARY_OP(NOT_B, "not b", GP_B, NOT), ALU_UNARY_OP(SHL_B, "shl b", GP_B, SHL), ALU_UNARY_OP(SHR_B, "shr b", GP_B, SHR),

    ALU_OP(ADD_B_D, "add b,d", GP_B, GP_D, ADD), ALU_OP(ADC_B_D, "adc b,d", GP_B, GP_D, ADC), ALU_OP(SUB_B_D, "sub b,d", GP_B, GP_D, SUB), ALU_OP(SBB_B_D, "sbb b,d", GP_B, GP_D, SBB), ALU_OP(AND_B_D, "and b,d", GP_B, GP_D, AND), ALU_OP(OR_B_D, "or b,d", GP_B, GP_D, OR), ALU_OP(XOR_B_D, "xor b,d", GP_B, GP_D, XOR),

    // C Register
    ALU_OP(ADD_C_D, "add c,d", GP_C, GP_D, ADD), ALU_OP(ADC_C_D, "adc c,d", GP_C, GP_D, ADC), ALU_OP(SUB_C_D, "sub c,d", GP_C, GP_D, SUB), ALU_OP(SBB_C_D, "sbb c,d", GP_C, GP_D, SBB), ALU_OP(AND_C_D, "and c,d", GP_C, GP_D, AND), ALU_OP(OR_C_D, "or c,d", GP_C, GP_D, OR), ALU_OP(XOR_C_D, "xor c,d", GP_C, GP_D, XOR), ALU_UNARY_OP(NOT_C, "not c", GP_C, NOT), ALU_UNARY_OP(SHL_C, "shl c", GP_C, SHL), ALU_UNARY_OP(SHR_C, "shr c", GP_C, SHR),

    // D Register
    ALU_UNARY_OP(NOT_D, "not d", GP_D, NOT), ALU_UNARY_OP(SHL_D, "shl d", GP_D, SHL), ALU_UNARY_OP(SHR_D, "shr d", GP_D, SHR),

    // Clear
    ALU_OP(CLR_A, "clr a", GP_A, GP_A, XOR), ALU_OP(CLR_B, "clr b", GP_B, GP_B, XOR), ALU_OP(CLR_C, "clr c", GP_C, GP_C, XOR), ALU_OP(CLR_D, "clr d", GP_D, GP_D, XOR),

    // Swap
    SWAP(SWP_A_B, "swp a,b", GP_A, GP_B), SWAP(SWP_A_C, "swp a,c", GP_A, GP_C), SWAP(SWP_A_D, "swp a,d", GP_A, GP_D), SWAP(SWP_B_C, "swp b,c", GP_B, GP_C), SWAP(SWP_B_D, "swp b,d", GP_B, GP_D), SWAP(SWP_C_D, "swp c,d", GP_C, GP_D),

    ALU_WIDE_OP(ADD_AB_CD, "add ab,cd", ADD, ADC), ALU_WIDE_OP(ADC_AB_CD, "adc ab,cd", ADC, ADC), ALU_WIDE_OP(SUB_AB_CD, "sub ab,cd", SUB, SBB), ALU_WIDE_OP(SBB_AB_CD, "sbb ab,cd", SBB, SBB),

    { .opcode = JMP_ABS, .instruction = "jmp *%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = PC },

    { .opcode = JNZ_ABS, .instruction = "jnz *%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = PC, .condition = SystemBus::Z, .condition_op = MicroCode::Nand },

    { .opcode = JC_ABS, .instruction = "jc *%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = PC, .condition = SystemBus::C, .condition_op = MicroCode::And },

    { .opcode = JV_ABS, .instruction = "jv *%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = PC, .condition = SystemBus::V, .condition_op = MicroCode::And },

    { .opcode = CALL_ABS, .instruction = "call *%04x", .addressingMode = ImmediateWord, .steps = {
                                                                                            // Can't use AbsoluteWord addressing mode because need to push PC before
                                                                                            // reading destination address from memory.
                                                                                            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                            { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::None },
                                                                                            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                            { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::MSB },

                                                                                            // Push the return address:
                                                                                            PUSH_ADDR(PC, SystemBus::None),

                                                                                            // Load PC with the subroutine address:
                                                                                            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                            { .action = MicroCode::XDATA, .src = MEM, .target = PC, .opflags = SystemBus::None },
                                                                                            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::Inc },
                                                                                            { .action = MicroCode::XDATA, .src = MEM, .target = PC, .opflags = SystemBus::MSB | SystemBus::Done },

                                                                                        } },

    CMP(CMP_A_B, "cmp a,b", GP_A, GP_B), CMP(CMP_A_C, "cmp a,c", GP_A, GP_C), CMP(CMP_A_D, "cmp a,d", GP_A, GP_D), CMP(CMP_B_C, "cmp b,c", GP_B, GP_C), CMP(CMP_B_D, "cmp b,d", GP_B, GP_D), CMP(CMP_C_D, "cmp c,d", GP_C, GP_D),

    ALU_UNARY_OP(INC_A, "inc a", GP_A, INC), ALU_UNARY_OP(INC_B, "inc b", GP_B, INC), ALU_UNARY_OP(INC_C, "inc c", GP_C, INC), ALU_UNARY_OP(INC_D, "inc d", GP_D, INC), ALU_UNARY_OP(DEC_A, "dec a", GP_A, DEC), ALU_UNARY_OP(DEC_B, "dec b", GP_B, DEC), ALU_UNARY_OP(DEC_C, "dec c", GP_C, DEC), ALU_UNARY_OP(DEC_D, "dec d", GP_D, DEC),

    { .opcode = INC_SI, .instruction = "inc si", .addressingMode = Immediate, .steps = {
                                                                                  { .action = MicroCode::XADDR, .src = Si, .target = TX, .opflags = SystemBus::Inc | SystemBus::Flags | SystemBus::Done },
                                                                              } },

    { .opcode = INC_DI, .instruction = "inc di", .addressingMode = Immediate, .steps = {
                                                                                  { .action = MicroCode::XADDR, .src = Di, .target = TX, .opflags = SystemBus::Inc | SystemBus::Flags | SystemBus::Done },
                                                                              } },

    { .opcode = DEC_SI, .instruction = "dec si", .addressingMode = Immediate, .steps = {
                                                                                  { .action = MicroCode::XADDR, .src = Si, .target = TX, .opflags = SystemBus::Dec | SystemBus::Flags | SystemBus::Done },
                                                                              } },

    { .opcode = DEC_DI, .instruction = "dec di", .addressingMode = Immediate, .steps = {
                                                                                  { .action = MicroCode::XADDR, .src = Di, .target = TX, .opflags = SystemBus::Dec | SystemBus::Flags | SystemBus::Done },
                                                                              } },

    OUT(OUT_A, "out #%02x, a", GP_A), OUT(OUT_B, "out #%02x, b", GP_B), OUT(OUT_C, "out #%02x, c", GP_C), OUT(OUT_D, "out #%02x, d", GP_D),

    IN(IN_A, "in a, #%02x", GP_A), IN(IN_B, "in b, #%02x", GP_B), IN(IN_C, "in c, #%02x", GP_C), IN(IN_D, "in d, #%02x", GP_D),

    { .opcode = PUSH_FLAGS, .instruction = "pushfl", .addressingMode = Immediate, .steps = { { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Inc }, { .action = MicroCode::XADDR, .src = RHS, .target = MEM, .opflags = SystemBus::Done } } },

    { .opcode = POP_FLAGS, .instruction = "popfl", .addressingMode = Immediate, .steps = { { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Dec }, { .action = MicroCode::XADDR, .src = MEM, .target = RHS, .opflags = SystemBus::Done } } },

    { .opcode = CLR_FLAGS, .instruction = "clrfl", .addressingMode = Immediate, .steps = {

                                                                                    // Clear TX LSB by XORing in the ALU:
                                                                                    { .action = MicroCode::XDATA, .src = TX, .target = LHS, .opflags = SystemBus::None }, { .action = MicroCode::XDATA, .src = TX, .target = RHS, .opflags = ALU::XOR },

                                                                                    // Move back result (zero) to TX LSB:
                                                                                    { .action = MicroCode::XDATA, .src = LHS, .target = TX, .opflags = SystemBus::None },

                                                                                    // Move TX LSB (which is 0) to flags using xaddr on ALU RHS register:
                                                                                    { .action = MicroCode::XADDR, .src = TX, .target = RHS, .opflags = SystemBus::Done } } },

    { .opcode = JZ, .instruction = "jz #%04x", .addressingMode = static_cast<AddressingMode>(DirectWord | Done), .target = PC, .condition = SystemBus::Z, .condition_op = MicroCode::And },

    { .opcode = JZ_ABS, .instruction = "jz *%04x", .addressingMode = static_cast<AddressingMode>(AbsoluteWord | Done), .target = PC, .condition = SystemBus::Z, .condition_op = MicroCode::And },

    { .opcode = MOV__CD_A, .instruction = "mov *cd,a", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
                                                                                        { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
                                                                                        { .action = MicroCode::XDATA, .src = GP_A, .target = MEM, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV__CD_B, .instruction = "mov *cd,b", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
                                                                                        { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
                                                                                        { .action = MicroCode::XDATA, .src = GP_B, .target = MEM, .opflags = SystemBus::Done },
                                                                                    } },

    CMP_CONST(CMP_A_CONST, "cmp a,#%02x", GP_A), CMP_CONST(CMP_B_CONST, "cmp b,#%02x", GP_B), CMP_CONST(CMP_C_CONST, "cmp c,#%02x", GP_C), CMP_CONST(CMP_D_CONST, "cmp d,#%02x", GP_D),

    ALU_OP_CONST(AND_A_CONST, "and a,#%02x", GP_A, AND), ALU_OP_CONST(AND_B_CONST, "and b,#%02x", GP_B, AND), ALU_OP_CONST(AND_C_CONST, "and c,#%02x", GP_C, AND), ALU_OP_CONST(AND_D_CONST, "and d,#%02x", GP_D, AND),

    ALU_OP_CONST(OR_A_CONST, "or a,#%02x", GP_A, OR), ALU_OP_CONST(OR_B_CONST, "or b,#%02x", GP_B, OR), ALU_OP_CONST(OR_C_CONST, "or c,#%02x", GP_C, OR), ALU_OP_CONST(OR_D_CONST, "or d,#%02x", GP_D, OR),

    { .opcode = MOV_A__CD, .instruction = "mov a,*cd", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
                                                                                        { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_A, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV_B__CD, .instruction = "mov b,*cd", .addressingMode = Immediate, .steps = {
                                                                                        { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
                                                                                        { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
                                                                                        { .action = MicroCode::XDATA, .src = MEM, .target = GP_B, .opflags = SystemBus::Done },
                                                                                    } },

    { .opcode = MOV__SI_CONST, .instruction = "mov *si,#%02x", .addressingMode = DirectByte, .target = TX, .steps = {
                                                                                                               { .action = MicroCode::XADDR, .src = Si, .target = MEMADDR, .opflags = SystemBus::None },
                                                                                                               { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done },
                                                                                                           } },

    { .opcode = MOV__DI_CONST, .instruction = "mov *di,#%02x", .addressingMode = DirectByte, .target = TX, .steps = {
                                                                                                               { .action = MicroCode::XADDR, .src = Di, .target = MEMADDR, .opflags = SystemBus::None },
                                                                                                               { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done },
                                                                                                           } },

    { .opcode = MOV__CD_CONST, .instruction = "mov *cd,#%02x", .addressingMode = DirectByte, .target = TX, .steps = {
                                                                                                               { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
                                                                                                               { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
                                                                                                               { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done },
                                                                                                           } },

    { .opcode = MOV_CD_CONST, .instruction = "mov cd,#%04x", .addressingMode = DirectWord, .target = TX, .steps = {
                                                                                                             { .action = MicroCode::XDATA, .src = TX, .target = GP_C, .opflags = SystemBus::None },
                                                                                                             { .action = MicroCode::XDATA, .src = TX, .target = GP_D, .opflags = SystemBus::MSB | SystemBus::Done },
                                                                                                         } },

    { /* 194 */ }, { /* 195 */ }, { /* 196 */ }, { /* 197 */ }, { /* 198 */ }, { /* 199 */ }, { /* 200 */ }, { /* 201 */ }, { /* 202 */ }, { /* 203 */ }, { /* 204 */ }, { /* 205 */ }, { /* 206 */ }, { /* 207 */ }, { /* 208 */ }, { /* 209 */ }, { /* 210 */ }, { /* 211 */ }, { /* 212 */ }, { /* 213 */ }, { /* 214 */ }, { /* 215 */ }, { /* 216 */ }, { /* 217 */ }, { /* 218 */ }, { /* 219 */ }, { /* 220 */ }, { /* 221 */ }, { /* 222 */ }, { /* 223 */ }, { /* 224 */ }, { /* 225 */ }, { /* 226 */ }, { /* 227 */ }, { /* 228 */ }, { /* 229 */ }, { /* 230 */ }, { /* 231 */ }, { /* 232 */ }, { /* 233 */ }, { /* 234 */ }, { /* 235 */ }, { /* 236 */ }, { /* 237 */ }, { /* 238 */ }, { /* 239 */ }, { /* 240 */ }, { /* 241 */ }, { /* 242 */ }, { /* 243 */ }, { /* 244 */ }, { /* 245 */ }, { /* 246 */ }, { /* 247 */ }, { /* 248 */ }, { /* 249 */ }, { /* 250 */ }, { /* 251 */ }, { /* 252 */ },

    { .opcode = RTI, .instruction = "rti", .addressingMode = Immediate, .steps = {
                                                                            POP_ADDR(PC, SystemBus::None),
                                                                            { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::Dec },
                                                                            { .action = MicroCode::XADDR, .src = MEM, .target = RHS, .opflags = SystemBus::Done },
                                                                        } },

    { .opcode = NMIVEC, .instruction = "nmi #%04x", .addressingMode = DirectWord, .target = TX, .steps = {
                                                                                                    { .action = MicroCode::XADDR, .src = TX, .target = CONTROLLER, .opflags = SystemBus::Done },
                                                                                                } },

    { .opcode = HLT, .instruction = "hlt", .steps = {
                                               { .action = MicroCode::OTHER, .src = GP_A, .target = GP_A, .opflags = SystemBus::Halt | SystemBus::Done },
                                           } }

};

}
