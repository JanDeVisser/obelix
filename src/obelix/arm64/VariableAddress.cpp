/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <core/Error.h>
#include <obelix/Processor.h>
#include <obelix/arm64/ARM64Context.h>
#include <obelix/arm64/Mnemonic.h>
#include <obelix/arm64/VariableAddress.h>

namespace Obelix {

ErrorOr<void, SyntaxError> StackVariableAddress::load_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int target) const
{
    return ctx.load_variable(type, offset(), target);
}

ErrorOr<void, SyntaxError> StackVariableAddress::store_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int from) const
{
    return ctx.store_variable(type, offset(), from);
}

ErrorOr<void, SyntaxError> StackVariableAddress::prepare_pointer(ARM64Context& ctx) const
{
    ctx.assembly()->add_instruction("add", "x8,fp,#{}", ctx.stack_depth() - offset());
    return {};
}

ErrorOr<void, SyntaxError> StructMemberAddress::load_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int target) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot push values of variables of type {} yet", type) };

    if (auto error_maybe = structure()->prepare_pointer(ctx); error_maybe.is_error()) {
        return error_maybe.error();
    }
    if (offset() > 0)
        ctx.assembly()->add_instruction("sub", "x8,x8,#{}", ctx.stack_depth() - offset());
    ctx.assembly()->add_instruction(mm->load_mnemonic, "{}{},[x8]", mm->reg_width, target);
    return {};
}

ErrorOr<void, SyntaxError> StructMemberAddress::store_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int from) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot store values type {} yet", type) };
    if (auto error_maybe = structure()->prepare_pointer(ctx); error_maybe.is_error()) {
        return error_maybe.error();
    }
    if (offset() > 0)
        ctx.assembly()->add_instruction("add", "x8,x8,#{}", ctx.stack_depth() - offset());
    ctx.assembly()->add_instruction(mm->store_mnemonic, "{}{},[x8]", mm->reg_width, from);
    return {};
}

ErrorOr<void, SyntaxError> StructMemberAddress::prepare_pointer(ARM64Context& ctx) const
{
    ctx.assembly()->add_instruction("add", "x8,x8,#{}", ctx.stack_depth() - offset());
    return {};
}

ErrorOr<void, SyntaxError> ArrayElementAddress::load_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int target) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot push values of variables of type {} yet", type) };

    TRY_RETURN(array()->prepare_pointer(ctx));
    TRY_RETURN(prepare_pointer(ctx));
    pop(ctx, "x0");
    ctx.assembly()->add_instruction(mm->load_mnemonic, "{}{},[x8]", mm->reg_width, target);
    return {};
}

ErrorOr<void, SyntaxError> ArrayElementAddress::store_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int from) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot store values type {} yet", type) };
    TRY_RETURN(array()->prepare_pointer(ctx));
    TRY_RETURN(prepare_pointer(ctx));
    pop(ctx, "x0");
    ctx.assembly()->add_instruction(mm->store_mnemonic, "{}{},[x8]", mm->reg_width, from);
    return {};
}

ErrorOr<void, SyntaxError> ArrayElementAddress::prepare_pointer(ARM64Context& ctx) const
{
    // x0 will hold the array index. Here we add that index, multiplied by the element size,
    // to x8, which should hold the array base address

    switch (element_size()) {
    case 1:
        ctx.assembly()->add_instruction("add", "x8,x8,x0");
        break;
    case 2:
        ctx.assembly()->add_instruction("add", "x8,x8,x0,lsl #1");
        break;
    case 4:
        ctx.assembly()->add_instruction("add", "x8,x8,x0,lsl #2");
        break;
    case 8:
        ctx.assembly()->add_instruction("add", "x8,x8,x0,lsl #3");
        break;
    case 16:
        ctx.assembly()->add_instruction("add", "x8,x8,x0,lsl #4");
        break;
    default:
        return SyntaxError { ErrorCode::InternalError, Token {}, "Cannot access arrays with elements of size {} yet", element_size() };
    }
    return {};
}

ErrorOr<void, SyntaxError> StaticVariableAddress::load_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int target) const
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot push values of variables of type {} yet", type) };

        ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", label());
        ctx.assembly()->add_instruction(mm->load_mnemonic, "{}{},[x8,{}@PAGEOFF]", mm->reg_width, target, label());
        return {};
    }
    ctx.assembly()->add_comment("Loading static struct variable");
    ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", label());
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        ctx.assembly()->add_instruction("ldr", format("{}{},[x8,{}@PAGEOFF+{}]", reg_width, target++, label(), type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> StaticVariableAddress::store_variable(std::shared_ptr<ObjectType> const& type, ARM64Context& ctx, int from) const
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot store values of type {} yet", type) };

        ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", label());
        ctx.assembly()->add_instruction(mm->store_mnemonic, "{}{},[x8,{}@PAGEOFF]", mm->reg_width, from, label());
        return {};
    }
    ctx.assembly()->add_comment("Storing static struct variable");
    ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", label());
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        ctx.assembly()->add_instruction("str", format("{}{},[x8,{}@PAGEOFF+{}]", reg_width, from++, label(), type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> StaticVariableAddress::prepare_pointer(ARM64Context& ctx) const
{
    ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", label());
    ctx.assembly()->add_instruction("add", "x8,x8,{}@PAGEOFF", label());
    return {};
}

}
