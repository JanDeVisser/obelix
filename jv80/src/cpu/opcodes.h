/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

namespace Obelix::JV80::CPU {

enum OpCode {
    NOP = 0x00,
    MOV_A_CONST = 0x01,
    MOV_A_ADDR = 0x02,
    MOV_A_B = 0x03,
    MOV_A_C = 0x04,
    MOV_A_D = 0x05,

    MOV_B_CONST = 0x06,
    MOV_B_ADDR = 0x07,
    MOV_B_A = 0x08,
    MOV_B_C = 0x09,
    MOV_B_D = 0x0A,

    MOV_C_CONST = 0x0B,
    MOV_C_ADDR = 0x0C,
    MOV_C_A = 0x0D,
    MOV_C_B = 0x0E,
    MOV_C_D = 0x0F,

    MOV_D_CONST = 0x10,
    MOV_D_ADDR = 0x11,
    MOV_D_A = 0x12,
    MOV_D_B = 0x13,
    MOV_D_C = 0x14,

    MOV_SP_CONST = 0x15,
    MOV_SP_ADDR = 0x16,
    MOV_SP_SI = 0x17,

    MOV_SI_CONST = 0x18,
    MOV_SI_ADDR = 0x19,
    MOV_SI_CD = 0x1A,

    MOV_DI_CONST = 0x1B,
    MOV_DI_ADDR = 0x1C,
    MOV_DI_CD = 0x1D,

    MOV_A__SI = 0x1E,
    MOV_B__SI = 0x1F,
    MOV_C__SI = 0x20,
    MOV_D__SI = 0x21,

    MOV_A__DI = 0x22,
    MOV_B__DI = 0x23,
    MOV_C__DI = 0x24,
    MOV_D__DI = 0x25,

    MOV_DI_SI = 0x26,

    JMP = 0x27,
    JNZ = 0x28,
    JC = 0x29,
    JV = 0x2A,
    CALL = 0x2B,
    RET = 0x2C,

    PUSH_A = 0x2D,
    PUSH_B = 0x2E,
    PUSH_C = 0x2F,
    PUSH_D = 0x30,
    PUSH_SI = 0x31,
    PUSH_DI = 0x32,
    POP_A = 0x33,
    POP_B = 0x34,
    POP_C = 0x35,
    POP_D = 0x36,
    POP_SI = 0x37,
    POP_DI = 0x38,

    MOV_ADDR_A = 0x39,
    MOV__DI_A = 0x3A,
    MOV_ADDR_B = 0x3B,
    MOV__DI_B = 0x3C,
    MOV_ADDR_C = 0x3D,
    MOV__DI_C = 0x3E,
    MOV_ADDR_D = 0x3F,
    MOV__DI_D = 0x40,

    MOV_ADDR_SI = 0x41,
    MOV_ADDR_DI = 0x42,
    MOV_ADDR_CD = 0x43,
    MOV__SI_CD = 0x44,
    MOV__DI_CD = 0x45,

    ADD_A_B = 0x46,
    ADC_A_B = 0x47,
    SUB_A_B = 0x48,
    SBB_A_B = 0x49,
    AND_A_B = 0x4A,
    OR_A_B = 0x4B,
    XOR_A_B = 0x4C,
    NOT_A = 0x4D,
    SHL_A = 0x4E,
    SHR_A = 0x4F,

    ADD_A_C = 0x50,
    ADC_A_C = 0x51,
    SUB_A_C = 0x52,
    SBB_A_C = 0x53,
    AND_A_C = 0x54,
    OR_A_C = 0x55,
    XOR_A_C = 0x56,

    ADD_A_D = 0x57,
    ADC_A_D = 0x58,
    SUB_A_D = 0x59,
    SBB_A_D = 0x5A,
    AND_A_D = 0x5B,
    OR_A_D = 0x5C,
    XOR_A_D = 0x5D,

    ADD_B_C = 0x5E,
    ADC_B_C = 0x5F,
    SUB_B_C = 0x60,
    SBB_B_C = 0x61,
    AND_B_C = 0x62,
    OR_B_C = 0x63,
    XOR_B_C = 0x64,
    NOT_B = 0x65,
    SHL_B = 0x66,
    SHR_B = 0x67,

    ADD_B_D = 0x68,
    ADC_B_D = 0x69,
    SUB_B_D = 0x6A,
    SBB_B_D = 0x6B,
    AND_B_D = 0x6C,
    OR_B_D = 0x6D,
    XOR_B_D = 0x6E,

    ADD_C_D = 0x6F,
    ADC_C_D = 0x70,
    SUB_C_D = 0x71,
    SBB_C_D = 0x72,
    AND_C_D = 0x73,
    OR_C_D = 0x74,
    XOR_C_D = 0x75,
    NOT_C = 0x76,
    SHL_C = 0x77,
    SHR_C = 0x78,

    NOT_D = 0x79,
    SHL_D = 0x7A,
    SHR_D = 0x7B,

    CLR_A = 0x7C,
    CLR_B = 0x7D,
    CLR_C = 0x7E,
    CLR_D = 0x7F,

    SWP_A_B = 0x80,
    SWP_A_C = 0x81,
    SWP_A_D = 0x82,
    SWP_B_C = 0x83,
    SWP_B_D = 0x84,
    SWP_C_D = 0x85,

    ADD_AB_CD = 0x86,
    ADC_AB_CD = 0x87,
    SUB_AB_CD = 0x88,
    SBB_AB_CD = 0x89,

    JMP_ABS = 0x8A,
    JNZ_ABS = 0x8B,
    JC_ABS = 0x8C,
    JV_ABS = 0x8D,
    CALL_ABS = 0x8E,

    CMP_A_B = 0x8F,
    CMP_A_C = 0x90,
    CMP_A_D = 0x91,
    CMP_B_C = 0x92,
    CMP_B_D = 0x93,
    CMP_C_D = 0x94,

    INC_A = 0x95,
    INC_B = 0x96,
    INC_C = 0x97,
    INC_D = 0x98,
    DEC_A = 0x99,
    DEC_B = 0x9A,
    DEC_C = 0x9B,
    DEC_D = 0x9C,

    INC_SI = 0x9D,
    INC_DI = 0x9E,
    DEC_SI = 0x9F,
    DEC_DI = 0xA0,

    OUT_A = 0xA1,
    OUT_B = 0xA2,
    OUT_C = 0xA3,
    OUT_D = 0xA4,

    IN_A = 0xA5,
    IN_B = 0xA6,
    IN_C = 0xA7,
    IN_D = 0xA8,

    PUSH_FLAGS = 0xA9,
    POP_FLAGS = 0xAA,
    CLR_FLAGS = 0xAB,
    JZ = 0xAC,
    JZ_ABS = 0xAD,

    MOV__CD_A = 0xAE,
    MOV__CD_B = 0xAF,

    CMP_A_CONST = 0xB0,
    CMP_B_CONST = 0xB1,
    CMP_C_CONST = 0xB2,
    CMP_D_CONST = 0xB3,

    AND_A_CONST = 0xB4,
    AND_B_CONST = 0xB5,
    AND_C_CONST = 0xB6,
    AND_D_CONST = 0xB7,

    OR_A_CONST = 0xB8,
    OR_B_CONST = 0xB9,
    OR_C_CONST = 0xBA,
    OR_D_CONST = 0xBB,

    MOV_A__CD = 0xBC,
    MOV_B__CD = 0xBD,

    MOV__SI_CONST = 0xBE,
    MOV__DI_CONST = 0xBF,
    MOV__CD_CONST = 0xC0,
    MOV_CD_CONST = 0xC1,

    RTI = 0xFD,
    NMIVEC = 0xFE,
    HLT = 0xFF
};

}
