/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <sys/stat.h>

#include <obelix/ARM64Intrinsics.h>
#include <obelix/Intrinsics.h>

namespace Obelix {

extern_logging_category(arm64);

static std::array<ARM64FunctionType, IntrinsicType::count> s_intrinsics = {};

bool register_arm64_intrinsic(IntrinsicType type, ARM64FunctionType intrinsic)
{
    s_intrinsics[type] = move(intrinsic);
    return true;
}

ARM64FunctionType const& get_arm64_intrinsic(IntrinsicType type)
{
    assert(type > IntrinsicType::NotIntrinsic && type < IntrinsicType::count);
    return s_intrinsics[type];
}

#define INTRINSIC(intrinsic)                                                                      \
    ErrorOr<void> arm64_intrinsic_##intrinsic(ARM64Context& ctx);                                 \
    auto s_arm64_##intrinsic##_decl = register_arm64_intrinsic(intrinsic, arm64_intrinsic_##intrinsic); \
    ErrorOr<void> arm64_intrinsic_##intrinsic(ARM64Context& ctx)

#define INTRINSIC_ALIAS(intrinsic, alias) \
    auto s_##intrinsic##_decl = register_arm64_intrinsic(intrinsic, arm64_intrinsic_##alias); \

INTRINSIC(add_str_str)
{
    ctx.assembly().add_instruction("mov", "x4,x0");
    ctx.assembly().add_instruction("mov", "w5,w1");
    ctx.assembly().add_instruction("mov", "x6,x2");
    ctx.assembly().add_instruction("mov", "w7,w3");
    ctx.assembly().add_instruction("add", "w1,w1,w3");
    ctx.assembly().add_instruction("bl", "string_alloc");
    ctx.assembly().add_instruction("mov", "w1,w5");
    ctx.assembly().add_instruction("mov", "x2,x6");
    ctx.assembly().add_instruction("mov", "w3,w7");
    ctx.assembly().add_instruction("bl", "string_concat");
    ctx.assembly().add_instruction("add", "w1,w5,w7");
    return {};
}


// This is a mmap syscall
INTRINSIC(allocate)
{
    ctx.assembly().add_instruction("mov", "x1,x0");
    ctx.assembly().add_instruction("mov", "x0,xzr");
    ctx.assembly().add_instruction("mov", "w2,#3");
    ctx.assembly().add_instruction("mov", "w3,#0x1002");
    ctx.assembly().add_instruction("mov", "w4,#-1");
    ctx.assembly().add_instruction("mov", "x5,xzr");
    ctx.assembly().syscall(0xC5);
    return {};
}

INTRINSIC(eputs)
{
    ctx.assembly().add_instruction("mov", "x2,x1");
    ctx.assembly().add_instruction("mov", "x1,x0");
    ctx.assembly().add_instruction("mov", "x0,#2");
    ctx.assembly().syscall(0x04);
    return {};
}

INTRINSIC(fputs)
{
    ctx.assembly().syscall(0x04);
    return {};
}

INTRINSIC(fsize)
{
    auto sz = sizeof(struct stat);
    if (sz % 16)
        sz += 16 - (sz % 16);
    ctx.assembly().add_instruction("sub", "sp,sp,#{}", sz);
    ctx.assembly().add_instruction("mov", "x1,sp");
    ctx.assembly().syscall(339);
    auto lbl_ok = format("lbl_{}", Label::reserve_id());
    auto lbl_done = format("lbl_{}", Label::reserve_id());
    ctx.assembly().add_instruction("b.lo", lbl_ok);
    ctx.assembly().add_instruction("neg", "x0,x0");
    ctx.assembly().add_instruction("b", lbl_done);
    ctx.assembly().add_label(lbl_ok);
    ctx.assembly().add_instruction("ldr", "x0,[sp,#{}]", offsetof(struct stat, st_size));
    ctx.assembly().add_label(lbl_done);
    ctx.assembly().add_instruction("add", "sp,sp,#{}", sz);
    return {};
}

INTRINSIC(int_to_string)
{
    ctx.assembly().add_instruction("mov", "x2,x0");
    ctx.assembly().add_instruction("sub", "sp,sp,32");
    ctx.assembly().add_instruction("mov", "x0,sp");
    ctx.assembly().add_instruction("mov", "x1,#32");
    ctx.assembly().add_instruction("mov", "w3,#10");
    ctx.assembly().add_instruction("bl", "to_string");
    ctx.assembly().add_instruction("bl", "string_alloc");
    ctx.assembly().add_instruction("add", "sp,sp,32");
    return {};
}

INTRINSIC(putchar)
{
    ctx.assembly().add_instruction("strb", "w0,[sp,-16]!");
    ctx.assembly().add_instruction("mov", "x0,#1"); // x0: stdin
    ctx.assembly().add_instruction("mov", "x1,sp"); // x1: SP
    ctx.assembly().add_instruction("mov", "x2,#1"); // x2: Number of characters
    ctx.assembly().syscall(0x04);
    ctx.assembly().add_instruction("add", "sp,sp,16");
    return {};
}

INTRINSIC(puts)
{
    ctx.assembly().add_instruction("mov", "x2,x1");
    ctx.assembly().add_instruction("mov", "x1,x0");
    ctx.assembly().add_instruction("mov", "x0,#1");
    ctx.assembly().syscall(0x04);
    return {};
}

INTRINSIC(ptr_math)
{
    ctx.assembly().add_instruction("add", "x0,x0,x1");
    return {};
}

INTRINSIC(dereference)
{
    ctx.assembly().add_instruction("ldr", "x1,[x0]");
    ctx.assembly().add_instruction("mov", "x0,x1");
    return {};
}

INTRINSIC(add_int_int)
{
    ctx.assembly().add_instruction("add", "x0,x0,x1");
    return {};
}

INTRINSIC(subtract_int_int)
{
    ctx.assembly().add_instruction("sub", "x0,x0,x1");
    return {};
}

INTRINSIC(multiply_int_int)
{
    ctx.assembly().add_instruction("mul", "x0,x0,x1");
    return {};
}

INTRINSIC(divide_int_int)
{
    ctx.assembly().add_instruction("sdiv", "x0,x0,x1");
    return {};
}

INTRINSIC(equals_int_int)
{
    ctx.assembly().add_instruction("cmp", "x0,x1");
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("bne", set_false);
    ctx.assembly().add_instruction("mov", "w0,#0x01");
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w0,wzr");
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(greater_int_int)
{
    ctx.assembly().add_instruction("cmp", "x0,x1");
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.le", set_false);
    ctx.assembly().add_instruction("mov", "w0,#0x01");
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w0,wzr");
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(less_int_int)
{
    ctx.assembly().add_instruction("cmp", "x0,x1");
    auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.lt", set_true);
    ctx.assembly().add_instruction("mov", "w0,wzr");
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_true);
    ctx.assembly().add_instruction("mov", "w0,#0x01");
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(negate_int)
{
    ctx.assembly().add_instruction("neg", "x0,x0");
    return {};
}

INTRINSIC(invert_int)
{
    ctx.assembly().add_instruction("mvn", "x0,x0");
    return {};
}

INTRINSIC(invert_bool)
{
    ctx.assembly().add_instruction("eor", "w0,w0,#0x01"); // a is 0b00000001 (a was true) or 0b00000000 (a was false
    return {};
}

INTRINSIC(and_bool_bool)
{
    ctx.assembly().add_instruction("and", "w0,w0,w1");
    return {};
}

INTRINSIC(or_bool_bool)
{
    ctx.assembly().add_instruction("orr", "w0,w0,w1");
    return {};
}

INTRINSIC(xor_bool_bool)
{
    ctx.assembly().add_instruction("eor", "w0,w0,w1");
    return {};
}

INTRINSIC(equals_bool_bool)
{
    ctx.assembly().add_instruction("eor", "w0,w0,w1");    // a is 0b00000000 (a == b) or 0b00000001 (a != b)
    ctx.assembly().add_instruction("eor", "w0,w0,#0x01"); // a is 0b00000001 (a == b) or 0b00000000 (a != b)
    return {};
}

INTRINSIC(negate_byte)
{
    ctx.assembly().add_instruction("neg", "w0,w0");
    return {};
}

INTRINSIC(invert_byte)
{
    ctx.assembly().add_instruction("mvn", "w0,w0");
    return {};
}

INTRINSIC(add_byte_byte)
{
    ctx.assembly().add_instruction("add", "w0,w0,w1");
    return {};
}

INTRINSIC(subtract_byte_byte)
{
    ctx.assembly().add_instruction("sub", "w0,w0,w1");
    return {};
}

INTRINSIC(multiply_byte_byte)
{
    ctx.assembly().add_instruction("smull", "w0,w0,w1");
    return {};
}

INTRINSIC(divide_byte_byte)
{
    ctx.assembly().add_instruction("sdiv", "w0,w0,w1");
    return {};
}

INTRINSIC(equals_byte_byte)
{
    ctx.assembly().add_instruction("cmp", "w0,w1");
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("bne", set_false);
    ctx.assembly().add_instruction("mov", "w0,#0x01");
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w0,wzr");
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(greater_byte_byte)
{
    ctx.assembly().add_instruction("cmp", "w0,w1");
    auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.le", set_false);
    ctx.assembly().add_instruction("mov", "w0,#0x01");
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_false);
    ctx.assembly().add_instruction("mov", "w0,wzr");
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(less_byte_byte)
{
    ctx.assembly().add_instruction("cmp", "w0,w1");
    auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b.lt", set_true);
    ctx.assembly().add_instruction("mov", "w0,wzr");
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly().add_instruction("b", done);
    ctx.assembly().add_label(set_true);
    ctx.assembly().add_instruction("mov", "w0,#0x01");
    ctx.assembly().add_label(done);
    return {};
}

INTRINSIC(greater_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(less_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(equals_str_str)
{
    fatal("Not implemented");
}

INTRINSIC(multiply_str_int)
{
    fatal("Not implemented");
}

}
