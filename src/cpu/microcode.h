/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "alu.h"
#include "controller.h"
#include "opcodes.h"
#include "registers.h"

#define BYTE_IMM_INTO(trg)                                                                        \
    {                                                                                             \
        .opcode = MOV_##trg##_IMM,                                                                \
        .instruction = "MOV " #trg ",#$xx",                                                       \
        .addressingMode = (AddressingMode)(AddressingMode::ImmediateByte | AddressingMode::Done), \
        .subject = GP_##trg                                                                       \
    }

#define BYTE_IMM_IND_INTO(trg)                                                                   \
    {                                                                                            \
        .opcode = MOV_##trg##_IMM_IND,                                                           \
        .instruction = "MOV " #trg ",*$xxxx",                                                    \
        .addressingMode = (AddressingMode)(AddressingMode::IndirectByte | AddressingMode::Done), \
        .subject = GP_##trg                                                                      \
    }

#define BYTE_INTO_IMM_IND(source)                                                                                  \
    {                                                                                                              \
        .opcode = MOV_IMM_IND_##source, .instruction = "MOV *$xxxx," #source,                                      \
        .addressingMode = AddressingMode::ImmediateWord,                                                           \
        .subject = MEMADDR,                                                                                        \
        .steps = { { .action = MicroCode::XDATA, .src = GP_##source, .target = MEM, .opflags = SystemBus::Done } } \
    }

#define BYTE_XFER(trg, source)                                                                                          \
    {                                                                                                                   \
        .opcode = MOV_##trg##_##source,                                                                                 \
        .instruction = "MOV " #trg "," #source,                                                                         \
        .addressingMode = AddressingMode::Implied,                                                                      \
        .steps = { { .action = MicroCode::XDATA, .src = GP_##source, .target = GP_##trg, .opflags = SystemBus::Done } } \
    }

#define WORD_IMM_INTO(trg)                                                                        \
    {                                                                                             \
        .opcode = MOV_##trg##_IMM, .instruction = "MOV " #trg ",#$xxxx",                          \
        .addressingMode = (AddressingMode)(AddressingMode::ImmediateWord | AddressingMode::Done), \
        .subject = trg                                                                            \
    }
// 6 steps

#define WORD_IMM_IND_INTO(trg)                                                                   \
    {                                                                                            \
        .opcode = MOV_##trg##_IMM_IND, .instruction = "MOV " #trg ",*$xxxx",                     \
        .addressingMode = (AddressingMode)(AddressingMode::IndirectWord | AddressingMode::Done), \
        .subject = trg                                                                           \
    }
// 9 steps

#define WORD_INTO_IMM_IND(source)                                                                                  \
    {                                                                                                              \
        .opcode = MOV_IMM_IND_##source, .instruction = "MOV *$xxxx," #source,                                      \
        .addressingMode = AddressingMode::ImmediateWord,                                                           \
        .subject = MEMADDR,                                                                                        \
        .steps = { { .action = MicroCode::XDATA, .src = GP_##source, .target = MEM, .opflags = SystemBus::Done } } \
    }

#define WORD_XFER(trg, source)                                                                                \
    {                                                                                                         \
        .opcode = MOV_##trg##_##source,                                                                       \
        .instruction = "MOV " #trg "," #source,                                                               \
        .addressingMode = AddressingMode::Implied,                                                            \
        .steps = { { .action = MicroCode::XADDR, .src = source, .target = trg, .opflags = SystemBus::Done } } \
    }
// 3 steps

#define BYTE_XFER_IND(trg, source)                                                                       \
    {                                                                                                    \
        .opcode = MOV_##trg##_##source##_IND,                                                            \
        .instruction = "MOV " #trg ",*" #source,                                                         \
        .addressingMode = AddressingMode::Implied,                                                       \
        .steps = {                                                                                       \
            { .action = MicroCode::XADDR, .src = source, .target = MEMADDR, .opflags = SystemBus::INC }, \
            { .action = MicroCode::XDATA, .src = MEM, .target = GP_##trg, .opflags = SystemBus::Done }   \
        }                                                                                                \
    }

#define JUMP_IMM(jmp, cond, cond_op)                                                              \
    {                                                                                             \
        .opcode = jmp,                                                                            \
        .instruction = #jmp " #$xxxx",                                                            \
        .addressingMode = (AddressingMode)(AddressingMode::ImmediateWord | AddressingMode::Done), \
        .subject = PC,                                                                            \
        .condition = cond,                                                                        \
        .condition_op = cond_op                                                                   \
    }

#define JUMP_IMM_IND(jmp, cond, cond_op)                                                         \
    {                                                                                            \
        .opcode = jmp##_IND,                                                                     \
        .instruction = #jmp " *$xxxx",                                                           \
        .addressingMode = (AddressingMode)(AddressingMode::IndirectWord | AddressingMode::Done), \
        .subject = PC,                                                                           \
        .condition = cond,                                                                       \
        .condition_op = cond_op                                                                  \
    }

// clang-format off
#define PUSH_REG_STEPS(reg, flags)                                                           \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::INC }, \
    { .action = MicroCode::XDATA, .src = reg, .target = MEM, .opflags = flags }
// clang-format om

#define PUSH_REG(reg)                                                                                  \
    {                                                                                                  \
        .opcode = PUSH_##reg,                                                                          \
        .instruction = "PUSH " #reg,                                                                   \
        .addressingMode = AddressingMode::Implied,                                                     \
        .steps = {                                                                                     \
            PUSH_REG_STEPS(GP_##reg, SystemBus::Done)                                                  \
        }                                                                                              \
    }

// clang-format off
#define POP_REG_STEPS(reg, opflag)                                                           \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::DEC }, \
    { .action = MicroCode::XDATA, .src = MEM, .target = reg, .opflags = opflag  }
// clang-format on

#define POP_REG(reg)                                          \
    {                                                         \
        .opcode = POP_##reg,                                  \
        .instruction = "POP " #reg,                           \
        .addressingMode = AddressingMode::Implied,            \
        .steps = { POP_REG_STEPS(GP_##reg, SystemBus::Done) } \
    }

// clang-format off
#define PUSH_ADDR_STEPS(reg, opflag)                                                              \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::INC },      \
    { .action = MicroCode::XDATA, .src = reg, .target = MEM, .opflags = SystemBus::None },        \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::INC },      \
    { .action = MicroCode::XDATA, .src = reg, .target = MEM, .opflags = SystemBus::MSB | opflag }

#define PUSH_ADDR(reg)                              \
    {                                               \
        .opcode = PUSH_##reg,                       \
        .instruction = "push " #reg,                \
        .addressingMode = AddressingMode::Implied,  \
        .steps = {                                  \
            PUSH_ADDR_STEPS(reg, SystemBus::Done)   \
        }                                           \
    }

#define POP_ADDR_STEPS(reg, opflag)                                                          \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::DEC }, \
    { .action = MicroCode::XDATA, .src = MEM, .target = reg, .opflags = SystemBus::MSB },    \
    { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::DEC }, \
    { .action = MicroCode::XDATA, .src = MEM, .target = reg, .opflags = opflag  }
// clang-format on

#define POP_ADDR(reg)                                     \
    {                                                     \
        .opcode = POP_##reg,                              \
        .instruction = "POP " #reg,                       \
        .addressingMode = AddressingMode::Implied,        \
        .steps = { POP_ADDR_STEPS(reg, SystemBus::Done) } \
    }

#define ALU_OP(op, lhs, rhs)                                                                            \
    {                                                                                                   \
        .opcode = op##_##lhs##_##rhs,                                                                   \
        .instruction = #op " " #lhs "," #rhs,                                                           \
        .addressingMode = AddressingMode::Implied,                                                      \
        .steps = {                                                                                      \
            { .action = MicroCode::XDATA, .src = GP_##lhs, .target = LHS, .opflags = SystemBus::None }, \
            { .action = MicroCode::XDATA, .src = GP_##rhs, .target = RHS, .opflags = ALU::op },         \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_##lhs, .opflags = SystemBus::Done }, \
        }                                                                                               \
    }

#define ALU_UNARY_OP(op, reg)                                                                           \
    {                                                                                                   \
        .opcode = op##_##reg,                                                                           \
        .instruction = #op " " #reg,                                                                    \
        .addressingMode = AddressingMode::Implied,                                                      \
        .steps = {                                                                                      \
            { .action = MicroCode::XDATA, .src = GP_##reg, .target = RHS, .opflags = ALU::op },         \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_##reg, .opflags = SystemBus::Done }, \
        }                                                                                               \
    }

#define ADDR_UNARY_OP(op, reg)                                                                                                               \
    {                                                                                                                                        \
        .opcode = op##_##reg,                                                                                                                \
        .instruction = #op " " #reg,                                                                                                         \
        .addressingMode = AddressingMode::Implied,                                                                                           \
        .steps = { { .action = MicroCode::XADDR, .src = reg, .target = TX, .opflags = SystemBus::op | SystemBus::Flags | SystemBus::Done } } \
    }

#define CLR(reg)                                                                                        \
    {                                                                                                   \
        .opcode = CLR_##reg,                                                                            \
        .instruction = "CLR " #reg,                                                                     \
        .addressingMode = AddressingMode::Implied,                                                      \
        .steps = {                                                                                      \
            { .action = MicroCode::XDATA, .src = GP_##reg, .target = LHS, .opflags = SystemBus::None }, \
            { .action = MicroCode::XDATA, .src = GP_##reg, .target = RHS, .opflags = ALU::XOR },        \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_##reg, .opflags = SystemBus::Done }, \
        }                                                                                               \
    }

#define SWAP(reg1, reg2)                                                                                       \
    {                                                                                                          \
        .opcode = SWP_##reg1##_##reg2,                                                                         \
        .instruction = "SWP " #reg1 "," #reg2,                                                                 \
        .addressingMode = AddressingMode::Implied,                                                             \
        .steps = {                                                                                             \
            { .action = MicroCode::XDATA, .src = GP_##reg1, .target = TX, .opflags = SystemBus::None },        \
            { .action = MicroCode::XDATA, .src = GP_##reg2, .target = GP_##reg1, .opflags = SystemBus::None }, \
            { .action = MicroCode::XDATA, .src = TX, .target = GP_##reg2, .opflags = SystemBus::Done },        \
        }                                                                                                      \
    }

#define ALU_WIDE_OP(op, aluOpWithCarry)                                                                 \
    {                                                                                                   \
        .opcode = op##_AB_CD,                                                                           \
        .instruction = #op " AB,CD",                                                                    \
        .addressingMode = AddressingMode::Implied,                                                      \
        .steps = {                                                                                      \
            { .action = MicroCode::XDATA, .src = GP_A, .target = LHS, .opflags = SystemBus::None },     \
            { .action = MicroCode::XDATA, .src = GP_C, .target = RHS, .opflags = ALU::op },             \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_A, .opflags = SystemBus::None },     \
            { .action = MicroCode::XDATA, .src = GP_B, .target = LHS, .opflags = SystemBus::None },     \
            { .action = MicroCode::XDATA, .src = GP_D, .target = RHS, .opflags = ALU::aluOpWithCarry }, \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_B, .opflags = SystemBus::Done },     \
        }                                                                                               \
    }

#define CMP(lhs, rhs)                                                                                                      \
    {                                                                                                                      \
        .opcode = CMP_##lhs##_##rhs,                                                                                       \
        .instruction = "CMP " #lhs "," #rhs,                                                                               \
        .addressingMode = AddressingMode::Implied,                                                                         \
        .steps = {                                                                                                         \
            { .action = MicroCode::XDATA, .src = GP_##lhs, .target = LHS, .opflags = SystemBus::None },                    \
            { .action = MicroCode::XDATA, .src = GP_##rhs, .target = RHS, .opflags = ((byte)ALU::SUB) | SystemBus::Done }, \
        }                                                                                                                  \
    }

#define CMP_IMM(reg)                                                                                                  \
    {                                                                                                                 \
        .opcode = CMP_##reg##_IMM,                                                                                    \
        .instruction = "CMP " #reg ",#$xx",                                                                           \
        .addressingMode = AddressingMode::Implied,                                                                    \
        .subject = TX,                                                                                                \
        .steps = {                                                                                                    \
            { .action = MicroCode::XDATA, .src = GP_##reg, .target = LHS, .opflags = SystemBus::None },               \
            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::INC },                  \
            { .action = MicroCode::XDATA, .src = MEM, .target = RHS, .opflags = ((byte)ALU::SUB) | SystemBus::Done }, \
        }                                                                                                             \
    }

#define ALU_OP_IMM(op, reg)                                                                             \
    {                                                                                                   \
        .opcode = op##_##reg##_IMM,                                                                     \
        .instruction = #op " " #reg ",#$xx",                                                            \
        .addressingMode = AddressingMode::Implied,                                                      \
        .steps = {                                                                                      \
            { .action = MicroCode::XDATA, .src = GP_##reg, .target = LHS, .opflags = SystemBus::None }, \
            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::INC },    \
            { .action = MicroCode::XDATA, .src = MEM, .target = RHS, .opflags = ALU::op },              \
            { .action = MicroCode::XDATA, .src = LHS, .target = GP_##reg, .opflags = SystemBus::Done }, \
        }                                                                                               \
    }

#define OUT(reg)                                                                                                                    \
    {                                                                                                                               \
        .opcode = OUT_##reg,                                                                                                        \
        .instruction = "OUT #$xx," #reg,                                                                                            \
        .addressingMode = ImmediateByte,                                                                                            \
        .subject = CONTROLLER,                                                                                                      \
        .steps = {                                                                                                                  \
            { .action = MicroCode::IO, .src = GP_##reg, .target = DEREFCONTROLLER, .opflags = SystemBus::IOOut | SystemBus::Done }, \
        }                                                                                                                           \
    }

#define IN(reg)                                                                                                                    \
    {                                                                                                                              \
        .opcode = IN_##reg,                                                                                                        \
        .instruction = "IN " #reg ",#$xx",                                                                                         \
        .addressingMode = ImmediateByte,                                                                                           \
        .subject = CONTROLLER,                                                                                                     \
        .steps = {                                                                                                                 \
            { .action = MicroCode::IO, .src = GP_##reg, .target = DEREFCONTROLLER, .opflags = SystemBus::IOIn | SystemBus::Done }, \
        }                                                                                                                          \
    }

#define WORD_FROM_INDEXED(trg, s)                                                                                  \
    {                                                                                                              \
        .opcode = MOV_##trg##_##s##_IDX,                                                                           \
        .instruction = "MOV " #trg "," #s "[$xx]",                                                                 \
        .addressingMode = IndexedWord,                                                                             \
        .subject = s,                                                                                              \
        .steps = {                                                                                                 \
            { .action = MicroCode::XDATA, .src = MEM, .target = trg, .opflags = SystemBus::INC },                  \
            { .action = MicroCode::XDATA, .src = MEM, .target = trg, .opflags = SystemBus::Done | SystemBus::MSB } \
        }                                                                                                          \
    }

#define BYTE_FROM_INDEXED(trg, s)                                                                       \
    {                                                                                                   \
        .opcode = MOV_##trg##_##s##_IDX,                                                                \
        .instruction = "MOV " #trg "," #s "[$xx]",                                                      \
        .addressingMode = IndexedByte,                                                                  \
        .subject = s,                                                                                   \
        .steps = {                                                                                      \
            { .action = MicroCode::XDATA, .src = MEM, .target = GP_##trg, .opflags = SystemBus::Done }, \
        }                                                                                               \
    }

#define WORD_TO_INDEXED(trg, s)                                                                                  \
    {                                                                                                            \
        .opcode = MOV_##trg##_IDX_##s,                                                                           \
        .instruction = "MOV " #trg "[$xx]," #s,                                                                  \
        .addressingMode = IndexedWord,                                                                           \
        .subject = trg,                                                                                          \
        .steps = {                                                                                               \
            { .action = MicroCode::XDATA, .src = s, .target = MEM, .opflags = SystemBus::INC },                  \
            { .action = MicroCode::XDATA, .src = s, .target = MEM, .opflags = SystemBus::Done | SystemBus::MSB } \
        }                                                                                                        \
    }

#define BYTE_TO_INDEXED(trg, s)                                                                       \
    {                                                                                                 \
        .opcode = MOV_##trg##_IDX_##s,                                                                \
        .instruction = "MOV " #trg "[$xx]," #s,                                                       \
        .addressingMode = IndexedByte,                                                                \
        .subject = trg,                                                                               \
        .steps = {                                                                                    \
            { .action = MicroCode::XDATA, .src = GP_##s, .target = MEM, .opflags = SystemBus::Done }, \
        }                                                                                             \
    }

namespace Obelix::CPU {

// clang-format off
constexpr static MicroCode mc[256] = {
    {
        .opcode = NOP,
        .instruction = "NOP",
        .addressingMode = (AddressingMode)(AddressingMode::Implied | AddressingMode::Done)
    },
    BYTE_IMM_INTO(A),
    BYTE_IMM_IND_INTO(A),
    BYTE_XFER(A, B),
    BYTE_XFER(A, C),
    BYTE_XFER(A, D),
    BYTE_IMM_INTO(B),
    BYTE_IMM_IND_INTO(B),
    BYTE_XFER(B, A),
    BYTE_XFER(B, C),
    BYTE_XFER(B, D),
    BYTE_IMM_INTO(C),
    BYTE_IMM_IND_INTO(C),
    BYTE_XFER(C, A),
    BYTE_XFER(C, B),
    BYTE_XFER(C, D),
    BYTE_IMM_INTO(D),
    BYTE_IMM_IND_INTO(D),
    BYTE_XFER(D, A),
    BYTE_XFER(D, B),
    BYTE_XFER(D, C),
    WORD_IMM_INTO(SP),
    WORD_IMM_IND_INTO(SP),
    WORD_XFER(SP, SI),
    WORD_IMM_INTO(SI),
    WORD_IMM_IND_INTO(SI),
    {
        .opcode = MOV_SI_CD,
        .instruction = "MOV SI,CD",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = SI, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = SI, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    WORD_IMM_INTO(DI),
    WORD_IMM_IND_INTO(DI),
    {
        .opcode = MOV_DI_CD,
        .instruction = "MOV DI,CD",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = DI, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = DI, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    BYTE_XFER_IND(A, SI),
    BYTE_XFER_IND(B, SI),
    BYTE_XFER_IND(C, SI),
    BYTE_XFER_IND(D, SI),
    BYTE_XFER_IND(A, DI),
    BYTE_XFER_IND(B, DI),
    BYTE_XFER_IND(C, DI),
    BYTE_XFER_IND(D, DI),
    {
        .opcode = MOV_DI_IND_SI_IND,
        .instruction = "MOV *DI,*SI",
        .addressingMode = AddressingMode::Implied,
        .steps = {
              { .action = MicroCode::XADDR, .src = SI, .target = MEMADDR, .opflags = SystemBus::INC },
              { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::None },
              { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::INC },
              { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done }
        }
    },
    JUMP_IMM(JMP, 0, MicroCode::None),
    JUMP_IMM(JNZ, SystemBus::Z, MicroCode::Nand),
    JUMP_IMM(JC, SystemBus::C, MicroCode::And),
    JUMP_IMM(JV, SystemBus::V, MicroCode::And),
    {
        .opcode = CALL,
        .instruction = "CALL #$xxxx",
        .addressingMode = ImmediateWord,
        .subject = TX,
        .steps = {
            // TX Contains the address to jump to. PC has the address to return to (one past the address)

            // Push the return address:
            PUSH_ADDR_STEPS(PC, SystemBus::None),

            // Load PC with the subroutine address:
            { .action = MicroCode::XADDR, .src = TX, .target = PC, .opflags = SystemBus::Done },
        }
    },
    {
        .opcode = RET,
        .instruction = "RET",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            POP_ADDR_STEPS(PC, SystemBus::Done)
        }
    },
    PUSH_REG(A),
    PUSH_REG(B),
    PUSH_REG(C),
    PUSH_REG(D),
    PUSH_ADDR(SI),
    PUSH_ADDR(DI),
    POP_REG(A),
    POP_REG(B),
    POP_REG(C),
    POP_REG(D),
    POP_ADDR(SI),
    POP_ADDR(DI),
    BYTE_INTO_IMM_IND(A),
    {
        .opcode = MOV_DI_IND_A,
        .instruction = "MOV *DI,A",
        .addressingMode = AddressingMode::Implied, // 0x3A
        .steps = {
            { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_A, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    BYTE_INTO_IMM_IND(B),
    {
        .opcode = MOV_DI_IND_B,
        .instruction = "MOV *DI,B",
        .addressingMode = AddressingMode::Implied, // 0x3C
        .steps = {
            { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_B, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    BYTE_INTO_IMM_IND(C),
    {
        .opcode = MOV_DI_IND_C,
        .instruction = "MOV *DI,C",
        .addressingMode = AddressingMode::Implied, // 0x3E
        .steps = {
            { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    BYTE_INTO_IMM_IND(D),
    {
        .opcode = MOV_DI_IND_D,
        .instruction = "MOV *DI,D",
        .addressingMode = AddressingMode::Implied, // 0x40
        .steps = {
            { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    {
        .opcode = MOV_IMM_IND_SI,
        .instruction = "MOV *$xxxx,SI",
        .addressingMode = ImmediateWord, .subject = TX, // 0x41
        .steps = {
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = SI, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = SI, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    {
        .opcode = MOV_IMM_IND_DI,
        .instruction = "MOV *$xxxx,DI",
        .addressingMode = ImmediateWord,
        .subject = TX, // 0x42
        .steps = {
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = DI, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = DI, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    {
        .opcode = MOV_IMM_IND_CD,
        .instruction = "MOV *$xxxx,CD",
        .addressingMode = ImmediateWord,
        .subject = TX, // 0x43
        .steps = {
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    {
        .opcode = MOV_SI_IND_CD,
        .instruction = "MOV *SI,CD",
        .addressingMode = AddressingMode::Implied, // 0x44
        .steps = {
            { .action = MicroCode::XADDR, .src = SI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = SI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    {
        .opcode = MOV_DI_IND_CD,
        .instruction = "MOV *DI,CD",
        .addressingMode = AddressingMode::Implied, // 0x45
        .steps = {
            { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEM, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    ALU_OP(ADD, A, B),
    ALU_OP(ADC, A, B),
    ALU_OP(SUB, A, B),
    ALU_OP(SBB, A, B),
    ALU_OP(AND, A, B),
    ALU_OP(OR, A, B),
    ALU_OP(XOR, A, B),
    ALU_UNARY_OP(NOT, A),
    ALU_UNARY_OP(SHL, A),
    ALU_UNARY_OP(SHR, A),
    ALU_OP(ADD, A, C),
    ALU_OP(ADC, A, C),
    ALU_OP(SUB, A, C),
    ALU_OP(SBB, A, C),
    ALU_OP(AND, A, C),
    ALU_OP(OR, A, C),
    ALU_OP(XOR, A, C),
    ALU_OP(ADD, A, D),
    ALU_OP(ADC, A, D),
    ALU_OP(SUB, A, D),
    ALU_OP(SBB, A, D),
    ALU_OP(AND, A, D),
    ALU_OP(OR, A, D),
    ALU_OP(XOR, A, D),
    ALU_OP(ADD, B, C),
    ALU_OP(ADC, B, C),
    ALU_OP(SUB, B, C),
    ALU_OP(SBB, B, C),
    ALU_OP(AND, B, C),
    ALU_OP(OR, B, C),
    ALU_OP(XOR, B, C),
    ALU_UNARY_OP(NOT, B),
    ALU_UNARY_OP(SHL, B),
    ALU_UNARY_OP(SHR, B),
    ALU_OP(ADD, B, D),
    ALU_OP(ADC, B, D),
    ALU_OP(SUB, B, D),
    ALU_OP(SBB, B, D),
    ALU_OP(AND, B, D),
    ALU_OP(OR, B, D),
    ALU_OP(XOR, B, D),
    ALU_OP(ADD, C, D),
    ALU_OP(ADC, C, D),
    ALU_OP(SUB, C, D),
    ALU_OP(SBB, C, D),
    ALU_OP(AND, C, D),
    ALU_OP(OR, C, D),
    ALU_OP(XOR, C, D),
    ALU_UNARY_OP(NOT, C),
    ALU_UNARY_OP(SHL, C),
    ALU_UNARY_OP(SHR, C),
    ALU_UNARY_OP(NOT, D),
    ALU_UNARY_OP(SHL, D),
    ALU_UNARY_OP(SHR, D),
    CLR(A),
    CLR(B),
    CLR(C),
    CLR(D),
    SWAP(A, B),
    SWAP(A, C),
    SWAP(A, D),
    SWAP(B, C),
    SWAP(B, D),
    SWAP(C, D),
    ALU_WIDE_OP(ADD, ADC),
    ALU_WIDE_OP(ADC, ADC),
    ALU_WIDE_OP(SUB, SBB),
    ALU_WIDE_OP(SBB, SBB),
    JUMP_IMM_IND(JMP, 0, MicroCode::None),
    JUMP_IMM_IND(JNZ, SystemBus::Z, MicroCode::Nand),
    JUMP_IMM_IND(JC, SystemBus::C, MicroCode::And),
    JUMP_IMM_IND(JV, SystemBus::V, MicroCode::And),
    {
        .opcode = CALL_IND,
        .instruction = "CALL *$xxxx",
        .addressingMode = AddressingMode::ImpliedWord,
        .steps = {
            // Can't use IndirectWord addressing mode because need to push PC before
            // reading destination address from memory.
            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = PC, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::MSB },

            // Push the return address:
            PUSH_ADDR_STEPS(PC, SystemBus::None),

            // Load PC with the subroutine address:
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = MEM, .target = PC, .opflags = SystemBus::None },
            { .action = MicroCode::XADDR, .src = TX, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = MEM, .target = PC, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    CMP(A, B),
    CMP(A, C),
    CMP(A, D),
    CMP(B, C),
    CMP(B, D),
    CMP(C, D),
    ALU_UNARY_OP(INC, A),
    ALU_UNARY_OP(INC, B),
    ALU_UNARY_OP(INC, C),
    ALU_UNARY_OP(INC, D),
    ALU_UNARY_OP(DEC, A),
    ALU_UNARY_OP(DEC, B),
    ALU_UNARY_OP(DEC, C),
    ALU_UNARY_OP(DEC, D),
    ADDR_UNARY_OP(INC, SI),
    ADDR_UNARY_OP(INC, DI),
    ADDR_UNARY_OP(DEC, SI),
    ADDR_UNARY_OP(DEC, DI),
    OUT(A),
    OUT(B),
    OUT(C),
    OUT(D),
    IN(A),
    IN(B),
    IN(C),
    IN(D),
    {
        .opcode = PUSH_FL,
        .instruction = "PUSHFL",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::INC },
            { .action = MicroCode::XADDR, .src = RHS, .target = MEM, .opflags = SystemBus::Done }
        }
    },
    {
        .opcode = POP_FL,
        .instruction = "POPFL",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::DEC },
            { .action = MicroCode::XADDR, .src = MEM, .target = RHS, .opflags = SystemBus::Done }
        }
    },
    {
        .opcode = CLR_FL,
        .instruction = "CLRFL",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            // Clear TX LSB by XORing in the ALU:
            { .action = MicroCode::XDATA, .src = TX, .target = LHS, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = TX, .target = RHS, .opflags = ALU::XOR },

            // Move back result (zero) to TX LSB:
            { .action = MicroCode::XDATA, .src = LHS, .target = TX, .opflags = SystemBus::None },

            // Move TX LSB (which is 0) to flags using xaddr on ALU RHS register:
            { .action = MicroCode::XADDR, .src = TX, .target = RHS, .opflags = SystemBus::Done }
        }
    },
    JUMP_IMM(JZ, SystemBus::Z, MicroCode::And),
    JUMP_IMM_IND(JZ, SystemBus::Z, MicroCode::And),
    {
        .opcode = MOV_CD_IND_A,
        .instruction = "MOV *CD,A",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
            { .action = MicroCode::XDATA, .src = GP_A, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    {
        .opcode = MOV_CD_IND_B,
        .instruction = "MOV *CD,B",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
            { .action = MicroCode::XDATA, .src = GP_B, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    CMP_IMM(A),
    CMP_IMM(B),
    CMP_IMM(C),
    CMP_IMM(D),

    ALU_OP_IMM(AND, A),
    ALU_OP_IMM(AND, B),
    ALU_OP_IMM(AND, C),
    ALU_OP_IMM(AND, D),
    ALU_OP_IMM(OR, A),
    ALU_OP_IMM(OR, B),
    ALU_OP_IMM(OR, C),
    ALU_OP_IMM(OR, D),
    {
        .opcode = MOV_A_CD_IND,
        .instruction = "MOV A,*CD",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
            { .action = MicroCode::XDATA, .src = MEM, .target = GP_A, .opflags = SystemBus::Done },
        }
    },
    {
        .opcode = MOV_B_CD_IND,
        .instruction = "MOV B,*CD",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
            { .action = MicroCode::XDATA, .src = MEM, .target = GP_B, .opflags = SystemBus::Done },
        } },

    {
        .opcode = MOV_SI_IND_IMM,
        .instruction = "MOV *SI,#$xx",
        .addressingMode = ImmediateByte,
        .subject = TX,
        .steps = {
           { .action = MicroCode::XADDR, .src = SI, .target = MEMADDR, .opflags = SystemBus::None },
           { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done },
       }
    },
    {
        .opcode = MOV_DI_IND_IMM,
        .instruction = "MOV *DI,#$xx",
        .addressingMode = ImmediateByte,
        .subject = TX,
        .steps = {
            { .action = MicroCode::XADDR, .src = DI, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    {
        .opcode = MOV_CD_IND_IMM,
        .instruction = "MOV *CD,#$xx",
        .addressingMode = ImmediateByte,
        .subject = TX,
        .steps = {
            { .action = MicroCode::XDATA, .src = GP_C, .target = MEMADDR, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = GP_D, .target = MEMADDR, .opflags = SystemBus::MSB },
            { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::Done },
        }
    },
    {
        .opcode = MOV_CD_IMM,
        .instruction = "MOV CD,#$xxxx",
        .addressingMode = ImmediateWord,
        .subject = TX,
        .steps = {
            { .action = MicroCode::XDATA, .src = TX, .target = GP_C, .opflags = SystemBus::None },
            { .action = MicroCode::XDATA, .src = TX, .target = GP_D, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },

    WORD_XFER(BP, SP),
    WORD_XFER(SP, BP),

    WORD_FROM_INDEXED(SI, BP),
    WORD_FROM_INDEXED(DI, BP),
    WORD_FROM_INDEXED(DI, SI),
    BYTE_FROM_INDEXED(A, BP),
    BYTE_FROM_INDEXED(B, BP),
    BYTE_FROM_INDEXED(C, BP),
    BYTE_FROM_INDEXED(D, BP),
    WORD_TO_INDEXED(BP, SI),
    WORD_TO_INDEXED(BP, DI),
    WORD_TO_INDEXED(SI, DI),
    BYTE_TO_INDEXED(BP, A),
    BYTE_TO_INDEXED(BP, B),
    BYTE_TO_INDEXED(BP, C),
    BYTE_TO_INDEXED(BP, D),
    PUSH_ADDR(BP),
    POP_ADDR(BP),
    {
        .opcode = PUSH_IMM,
        .instruction = "push #$xx",
        .addressingMode = AddressingMode::ImmediateByte,
        .subject = TX,
        .steps = {
            PUSH_REG_STEPS(TX, SystemBus::Done)
        }
    },
    {
        .opcode = PUSHW_IMM,
        .instruction = "pushw #$xxxx",
        .addressingMode = AddressingMode::ImmediateWord,
        .subject = TX,
        .steps = {
            PUSH_ADDR_STEPS(TX, SystemBus::Done)
        }
    },
    {
        .opcode = PUSH_AB,
        .instruction = "push ab",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            PUSH_REG_STEPS(GP_A, SystemBus::None),
            PUSH_REG_STEPS(GP_B, SystemBus::Done)
        }
    },
    {
        .opcode = PUSH_CD,
        .instruction = "push cd",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            PUSH_REG_STEPS(GP_C, SystemBus::None),
            PUSH_REG_STEPS(GP_D, SystemBus::Done)
        }
    },
{
        .opcode = PUSH_BP_IDX,
        .instruction = "push bp[$xx]",
        .addressingMode = AddressingMode::IndexedWord,
        .subject = BP,
        .steps = {
            { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = MEM, .target = TX, .opflags = SystemBus::MSB },
            PUSH_ADDR_STEPS(TX, SystemBus::Done)
        }
    },
    {
        .opcode = POP_AB,
        .instruction = "pop ab",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            POP_REG_STEPS(GP_B, SystemBus::None),
            POP_REG_STEPS(GP_A, SystemBus::Done)
        }
    },
    {
        .opcode = POP_CD,
        .instruction = "pop cd",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            POP_REG_STEPS(GP_D, SystemBus::None),
            POP_REG_STEPS(GP_C, SystemBus::Done)
        }
    },
    {
        .opcode = POP_BP_IDX,
        .instruction = "pop bp[$xx]",
        .addressingMode = AddressingMode::IndexedWord,
        .subject = BP,
        .steps = {
            { MicroCode::XADDR, MEMADDR, GP_A, SystemBus::None },
            POP_ADDR_STEPS(TX, SystemBus::None),
            { MicroCode::XADDR, GP_A, MEMADDR, SystemBus::None },
            { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::INC },
            { .action = MicroCode::XDATA, .src = TX, .target = MEM, .opflags = SystemBus::MSB | SystemBus::Done },
        }
    },
    JUMP_IMM(JNC, SystemBus::C, MicroCode::Nand),
    JUMP_IMM_IND(JNC, SystemBus::C, MicroCode::Nand),

    { /* 222 */ }, { /* 223 */ }, { /* 224 */ }, { /* 225 */ }, { /* 226 */ }, { /* 227 */ }, { /* 228 */ }, { /* 229 */ }, { /* 230 */ }, { /* 231 */ }, { /* 232 */ }, { /* 233 */ }, { /* 234 */ }, { /* 235 */ }, { /* 236 */ }, { /* 237 */ }, { /* 238 */ }, { /* 239 */ }, { /* 240 */ }, { /* 241 */ }, { /* 242 */ }, { /* 243 */ }, { /* 244 */ }, { /* 245 */ }, { /* 246 */ }, { /* 247 */ }, { /* 248 */ }, { /* 249 */ }, { /* 250 */ }, { /* 251 */ }, { /* 252 */ },

    {
        .opcode = RTI,
        .instruction = "RTI",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            POP_ADDR_STEPS(PC, SystemBus::None),
            { .action = MicroCode::XADDR, .src = SP, .target = MEMADDR, .opflags = SystemBus::DEC },
            { .action = MicroCode::XADDR, .src = MEM, .target = RHS, .opflags = SystemBus::Done },
        }
    },
    {
        .opcode = NMIVEC,
        .instruction = "NMI #$xxxx",
        .addressingMode = ImmediateWord,
        .subject = TX,
        .steps = { { .action = MicroCode::XADDR, .src = TX, .target = CONTROLLER, .opflags = SystemBus::Done } }
    },
    {
        .opcode = HLT,
        .instruction = "HLT",
        .addressingMode = AddressingMode::Implied,
        .steps = {
            { .action = MicroCode::OTHER, .src = GP_A, .target = GP_A, .opflags = SystemBus::Halt | SystemBus::Done },
       }
    }
};

// clang-format on

}
