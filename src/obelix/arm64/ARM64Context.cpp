/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>
#include <obelix/arm64/ARM64Context.h>
#include <obelix/arm64/Mnemonic.h>

namespace Obelix {

std::vector<std::shared_ptr<MaterializedFunctionDef>> ARM64ContextPayload::s_function_stack {};
std::unordered_map<std::string, std::shared_ptr<Assembly>> ARM64ContextPayload::s_assemblies {};
unsigned long ARM64ContextPayload::s_counter { 0 };

ARM64Context::ARM64Context(Config const& config)
    : Context(config)
{
    add_module(ROOT_MODULE_NAME);
}

ErrorOr<void, SyntaxError> ARM64Context::zero_initialize(std::shared_ptr<ObjectType> const& type, int offset)
{
    switch (type->type()) {
    case PrimitiveType::Pointer:
    case PrimitiveType::SignedIntegerNumber:
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::Boolean: {
        auto mm = get_type_mnemonic_map(type);
        assert(mm != nullptr);
        assembly()->add_instruction("mov", "{}0,{}zr", mm->reg_width, mm->reg_width);
        assembly()->add_instruction("str", "{}0,[fp,#{}]", mm->reg_width, stack_depth() - offset);
        break;
    }
    case PrimitiveType::Struct: {
        for (auto const& field : type->fields()) {
            TRY_RETURN(zero_initialize(field.type, offset - type->offset_of(field.name)));
        }
        break;
    }
    case PrimitiveType::Array: {
        // Arrays are not initialized now. Maybe that should be fixed
        break;
    }
    default:
        return SyntaxError { "Cannot initialize variables of type {} yet", type };
    }
    return {};
}

ErrorOr<void, SyntaxError> ARM64Context::load_variable(std::shared_ptr<ObjectType> const& type, size_t offset, int target)
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { "Cannot load values of variables of type {} yet", type };
        assembly()->add_comment(format("Loading variable: stack_depth {} offset {}", stack_depth(), offset));
        if (type->size() < 8)
            assembly()->add_instruction("mov", "x{},xzr", target);
        assembly()->add_instruction(mm->load_mnemonic, "{}{},[fp,#{}]", mm->reg_width, target, stack_depth() - offset);
        return {};
    }
    assembly()->add_comment(format("Loading struct variable: stack_depth {} offset {}", stack_depth(), offset));
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        assembly()->add_instruction("ldr", format("{}{},[fp,#{}]", reg_width, target++, stack_depth() - offset + type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> ARM64Context::store_variable(std::shared_ptr<ObjectType> const& type, size_t offset, int from)
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { "Cannot store values of type {} yet", type };
        assembly()->add_comment(format("Storing to variable: stack_depth {} offset {}", stack_depth(), offset));
        assembly()->add_instruction(mm->store_mnemonic, "{}{},[fp,#{}]", mm->reg_width, from, stack_depth() - offset);
        return {};
    }
    assembly()->add_comment(format("Storing struct variable: stack_depth {} offset {}", stack_depth(), offset));
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        assembly()->add_instruction("str", format("{}{},[fp,#{}]", reg_width, from++, stack_depth() - offset + type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> ARM64Context::define_static_storage(std::string const& label, std::shared_ptr<ObjectType> const& type, bool global, std::shared_ptr<BoundExpression> const& expression)
{
    switch (type->type()) {
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::SignedIntegerNumber:
    case PrimitiveType::Boolean:
    case PrimitiveType::Pointer: {
        int initial_value = 0;
        auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(expression);
        if (literal != nullptr)
            initial_value = literal->value();
        assembly()->add_data(label, global, ".long", true, initial_value);
        break;
    }
    case PrimitiveType::Array: {
        assembly()->add_data(label, global, ".space",
            type->template_argument<std::shared_ptr<ObjectType>>("base_type")->size() * true, type->template_argument<long>("size"));
        break;
    }
    case PrimitiveType::Struct: {
        // assembly()->add_comment("Reserving space for static struct");
        assembly()->add_data(label, global, ".space", true, type->size());
        break;
    }
    default:
        fatal("Can't emit static variables of type {} yet", type->type());
    }
    return {};
}

ErrorOr<void, SyntaxError> ARM64Context::load_immediate(std::shared_ptr<ObjectType> const& type, uint64_t value, int target)
{
    auto width = (type->size() == 8) ? "x" : "w";
    auto words = (type->size() + 1) / 2;
    assembly()->add_instruction("mov", "{}{},{}zr", width, target, width);
    for (auto ix = 0u; value && ix < words; ix++) {
        uint16_t w = value & 0xFFFF;
        value >>= 16;
        assembly()->add_instruction("movk", "{}{},#{},lsl #{}", width, target, w, ix*16);
    }
    return {};
}

ErrorOr<void, SyntaxError> ARM64Context::enter_function(std::shared_ptr<MaterializedFunctionDef> const& func)
{
    ARM64ContextPayload::s_function_stack.push_back(func);
    auto decl = func->declaration();
    stack_depth(func->stack_depth());
    assembly()->add_comment(format("{} nsaa {} stack depth {}", decl->to_string(), decl->nsaa(), func->stack_depth()));
    assembly()->add_directive(".global", func->label());
    assembly()->add_label(func->label());

    // fp, lr, and sp have been set be the calling function

    // Copy parameters from registers to their spot in the stack.
    // @improve Do this lazily, i.e. when we need the registers
    int nsaa = decl->nsaa();
    assembly()->add_instruction("stp", "fp,lr,[sp,#-16]!");
    if (func->stack_depth())
        assembly()->add_instruction("sub", "sp,sp,#{}", func->stack_depth());
    assembly()->add_instruction("mov", "fp,sp");
    for (auto& param : func->declaration()->parameters()) {
        switch (param->type()->type()) {
        case PrimitiveType::IntegerNumber:
        case PrimitiveType::SignedIntegerNumber:
        case PrimitiveType::Pointer:
            switch (param->method()) {
            case MaterializedFunctionParameter::ParameterPassingMethod::Register:
                assembly()->add_comment(format("Register parameter {}: x{} -> {}", param->name(), param->where(), std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset()));
                assembly()->add_instruction("str", "x{},[fp,#{}]", param->where(), func->stack_depth() - std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset());
                break;
            case MaterializedFunctionParameter::ParameterPassingMethod::Stack:
                assembly()->add_comment(format("Stack parameter {}: nsaa {} -> {}", param->name(), param->where(), std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset()));
                assembly()->add_instruction("ldr", "x9,[fp,#{}]", 16 + nsaa - param->where());
                assembly()->add_instruction("str", "x9,[fp,#{}]", func->stack_depth() - std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset());
                break;
            }
            break;
        case PrimitiveType::Struct:
            switch (param->method()) {
            case MaterializedFunctionParameter::ParameterPassingMethod::Register: {
                int reg = param->where();
                for (auto const& field : param->type()->fields()) {
                    std::string width = "w";
                    if (field.type->size() == 8)
                        width = "x";
                    assembly()->add_instruction("str", "{}{},[fp,#-{}]", width, reg++,
                        std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset() + param->type()->offset_of(field.name));
                }
                break;
            }
            case MaterializedFunctionParameter::ParameterPassingMethod::Stack:
                break;
            }
            // Fall through:
        default:
            fatal("Type '{}' not yet implemented in {}", param->type(), __func__);
        }
    }
    return {};
}

void ARM64Context::function_return() const
{
    assert(!ARM64ContextPayload::s_function_stack.empty());
    auto func_def = ARM64ContextPayload::s_function_stack.back();
    assembly()->add_instruction("b", format("__{}__return", func_def->label()));
}

void ARM64Context::leave_function()
{
    assert(!ARM64ContextPayload::s_function_stack.empty());
    auto func_def = ARM64ContextPayload::s_function_stack.back();
    assembly()->add_label(format("__{}__return", func_def->label()));
    assembly()->add_instruction("mov", "sp,fp");
    if (stack_depth())
        assembly()->add_instruction("add", "sp,sp,#{}", stack_depth());
    assembly()->add_instruction("ldp", "fp,lr,[sp],16");
    assembly()->add_instruction("ret");
    pop_stack_depth();
    ARM64ContextPayload::s_function_stack.pop_back();
}

void ARM64Context::reserve_on_stack(size_t bytes)
{
    if (bytes % 16)
        bytes = bytes + (16 - (bytes % 16));
    assembly()->add_instruction("sub", "sp,sp,#{}", bytes);
    data().m_stack_allocated += bytes;
}

void ARM64Context::release_stack()
{
    assembly()->add_instruction("add", "sp,sp,#{}", data().m_stack_allocated);
    data().m_stack_allocated = 0;
}

}
