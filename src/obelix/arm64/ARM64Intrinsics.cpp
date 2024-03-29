/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/arm64/ARM64Intrinsics.h>
#include <obelix/Intrinsics.h>

namespace Obelix {

extern_logging_category(arm64);

static std::array<ARM64FunctionType, IntrinsicType::count> s_intrinsics = {};

bool register_arm64_intrinsic(IntrinsicType type, ARM64FunctionType intrinsic)
{
    s_intrinsics[type] = std::move(intrinsic);
    return true;
}

ARM64FunctionType const& get_arm64_intrinsic(IntrinsicType type)
{
    assert(type > IntrinsicType::NotIntrinsic && type < IntrinsicType::count);
    return s_intrinsics[type];
}

#define INTRINSIC(intrinsic)                                                                            \
    ErrorOr<void, SyntaxError> arm64_intrinsic_##intrinsic(ARM64Context& ctx);                          \
    auto s_arm64_##intrinsic##_decl = register_arm64_intrinsic(intrinsic, arm64_intrinsic_##intrinsic); \
    ErrorOr<void, SyntaxError> arm64_intrinsic_##intrinsic(ARM64Context& ctx)

#define INTRINSIC_ALIAS(intrinsic, alias) \
    auto s_##intrinsic##_decl = register_arm64_intrinsic(intrinsic, arm64_intrinsic_##alias); \

// This is a mmap syscall
INTRINSIC(allocate)
{
    ctx.assembly()->add_text(
        R"(
    mov     x1,x0
    mov     x0,xzr
    mov     w2,#3
    mov     w3,#0x1002
    mov     w4,#-1
    mov     x5,xzr
    mov     x16,#0xC5
    svc     #0x00
)");
    return {};
}

INTRINSIC(eputs)
{
    ctx.assembly()->add_text(
        R"(
    mov     x2,x0
    mov     x0,#2
    mov     x16,#0x04
    svc     #0x00
)");
    return {};
}

INTRINSIC(fputs)
{
    ctx.assembly()->add_text(
        R"(
    mov     x4,x2
    mov     x2,x1
    mov     x2,x4
    mov     x16,#0x04
    svc     #0x00
)");
    return {};
}

INTRINSIC(int_to_string)
{
    ctx.assembly()->add_text(
        R"(
    mov     x2,x0
    sub     sp,sp,32
    mov     x1,sp
    mov     x0,#32
    mov     w3,#10
    bl      to_string
    bl      string_alloc
    add     sp,sp,32
)");
    return {};
}

INTRINSIC(putchar)
{
    ctx.assembly()->add_instruction("strb", "w0,[sp,-16]!");
    ctx.assembly()->add_instruction("mov", "x0,#1"); // x0: stdin
    ctx.assembly()->add_instruction("mov", "x1,sp"); // x1: SP
    ctx.assembly()->add_instruction("mov", "x2,#1"); // x2: Number of characters
    ctx.assembly()->syscall(0x04);
    ctx.assembly()->add_instruction("add", "sp,sp,16");
    return {};
}

INTRINSIC(ptr_math)
{
    ctx.assembly()->add_instruction("add", "x0,x0,x1");
    return {};
}

INTRINSIC(dereference)
{
    ctx.assembly()->add_instruction("ldr", "x1,[x0]");
    ctx.assembly()->add_instruction("mov", "x0,x1");
    return {};
}

INTRINSIC(add_int_int)
{
    ctx.assembly()->add_instruction("add", "x0,x0,x1");
    return {};
}

INTRINSIC(subtract_int_int)
{
    ctx.assembly()->add_instruction("sub", "x0,x0,x1");
    return {};
}

INTRINSIC(multiply_int_int)
{
    ctx.assembly()->add_instruction("mul", "x0,x0,x1");
    return {};
}

INTRINSIC(divide_int_int)
{
    ctx.assembly()->add_instruction("sdiv", "x0,x0,x1");
    return {};
}

void relational_op(ARM64Context& ctx, std::string branch)
{
    auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
    auto done = format("lbl_{}", Obelix::Label::reserve_id());
    ctx.assembly()->add_text(format(
        R"(
    cmp     x0,x1
    {}      {}
    mov     w0,wzr
    b       {}
{}:
    mov     w0,#0x01
{}:
)",
        branch, set_true, done, set_true, done));
}

INTRINSIC(equals_int_int)
{
    relational_op(ctx, "b.eq");
    return {};
}

INTRINSIC(greater_int_int)
{
    relational_op(ctx, "b.gt");
    return {};
}

INTRINSIC(less_int_int)
{
    relational_op(ctx, "b.lt");
    return {};
}

INTRINSIC(negate_s64)
{
    ctx.assembly()->add_instruction("neg", "x0,x0");
    return {};
}

INTRINSIC(negate_s32)
{
    ctx.assembly()->add_instruction("neg", "w0,w0");
    return {};
}

INTRINSIC(negate_s16)
{
    ctx.assembly()->add_instruction("neg", "w0,w0");
    return {};
}

INTRINSIC(negate_s8)
{
    ctx.assembly()->add_instruction("neg", "w0,w0");
    return {};
}

INTRINSIC(invert_int)
{
    ctx.assembly()->add_instruction("mvn", "x0,x0");
    return {};
}

INTRINSIC(invert_bool)
{
    ctx.assembly()->add_instruction("eor", "w0,w0,#0x01"); // a is 0b00000001 (a was true) or 0b00000000 (a was false
    return {};
}

INTRINSIC(and_bool_bool)
{
    ctx.assembly()->add_instruction("and", "w0,w0,w1");
    return {};
}

INTRINSIC(or_bool_bool)
{
    ctx.assembly()->add_instruction("orr", "w0,w0,w1");
    return {};
}

INTRINSIC(xor_bool_bool)
{
    ctx.assembly()->add_instruction("eor", "w0,w0,w1");
    return {};
}

INTRINSIC(equals_bool_bool)
{
    ctx.assembly()->add_instruction("eor", "w0,w0,w1");    // a is 0b00000000 (a == b) or 0b00000001 (a != b)
    ctx.assembly()->add_instruction("eor", "w0,w0,#0x01"); // a is 0b00000001 (a == b) or 0b00000000 (a != b)
    return {};
}

INTRINSIC(add_str_str)
{
    ctx.assembly()->add_text(
        R"(
    mov     w9,w0
    mov     x10,x1
    mov     w11,w2
    mov     x12,x3
    add     w0,w0,w2
    bl      string_alloc
    cmp     x1,0
    b.eq    __add_str_str_done
    mov     w0,w9
    mov     w2,w11
    mov     x3,x12
    bl      string_concat
__add_str_str_done:
)");
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
