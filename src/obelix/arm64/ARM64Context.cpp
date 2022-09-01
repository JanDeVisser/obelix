/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>
#include <obelix/arm64/ARM64Context.h>

namespace Obelix {

std::vector<std::shared_ptr<MaterializedFunctionDef>> ARM64Context::s_function_stack {};
std::unordered_map<std::string, Assembly> ARM64Context::s_assemblies {};
unsigned long ARM64Context::s_counter { 0 };

ARM64Context::ARM64Context(ARM64Context& parent)
    : Context<int>(parent)
    , m_assembly(parent.m_assembly)
    , m_stack_depth(parent.m_stack_depth)
{
    auto offset_maybe = get("#offset");
    assert(offset_maybe.has_value());
    declare("#offset", get("#offset").value());
}

ARM64Context::ARM64Context(ARM64Context* parent)
    : Context<int>(parent)
    , m_assembly(parent->m_assembly)
    , m_stack_depth(parent->m_stack_depth)
{
    auto offset_maybe = get("#offset");
    assert(offset_maybe.has_value());
    declare("#offset", get("#offset").value());
}

ARM64Context::ARM64Context()
    : Context()
    , m_assembly()
{
    add_module(ROOT_MODULE_NAME);
    declare("#offset", 0);
}

void ARM64Context::enter_function(std::shared_ptr<MaterializedFunctionDef> const& func)
{
    s_function_stack.push_back(func);
    auto decl = func->declaration();
    stack_depth(func->stack_depth());
    assembly().add_comment(format("{} nsaa {} stack depth {}", decl->to_string(), decl->nsaa(), func->stack_depth()));
    assembly().add_directive(".global", func->label());
    assembly().add_label(func->label());

    // fp, lr, and sp have been set be the calling function

    // Copy parameters from registers to their spot in the stack.
    // @improve Do this lazily, i.e. when we need the registers
    int nsaa = decl->nsaa();
    assembly().add_instruction("stp", "fp,lr,[sp,#-16]!");
    if (func->stack_depth())
        assembly().add_instruction("sub", "sp,sp,#{}", func->stack_depth());
    assembly().add_instruction("mov", "fp,sp");
    for (auto& param : func->declaration()->parameters()) {
        switch (param->type()->type()) {
            case PrimitiveType::IntegerNumber:
            case PrimitiveType::SignedIntegerNumber:
            case PrimitiveType::Pointer:
                switch (param->method()) {
                    case MaterializedFunctionParameter::ParameterPassingMethod::Register:
                        assembly().add_comment(format("Register parameter {}: x{} -> {}", param->name(), param->where(), std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset()));
                        assembly().add_instruction("str", "x{},[fp,#{}]", param->where(), func->stack_depth() - std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset());
                        break;
                    case MaterializedFunctionParameter::ParameterPassingMethod::Stack:
                        assembly().add_comment(format("Stack parameter {}: nsaa {} -> {}", param->name(), param->where(), std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset()));
                        assembly().add_instruction("ldr", "x9,[fp,#{}]", 16 + nsaa - param->where());
                        assembly().add_instruction("str", "x9,[fp,#{}]", func->stack_depth() - std::dynamic_pointer_cast<StackVariableAddress>(param->address())->offset());
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
                            assembly().add_instruction("str", "{}{},[fp,#-{}]", width, reg++,
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
}

void ARM64Context::function_return() const
{
    assert(!s_function_stack.empty());
    auto func_def = s_function_stack.back();
    assembly().add_instruction("b", format("__{}__return", func_def->label()));
}

void ARM64Context::leave_function()
{
    assert(!s_function_stack.empty());
    auto func_def = s_function_stack.back();
    assembly().add_label(format("__{}__return", func_def->label()));
    assembly().add_instruction("mov", "sp,fp");
    if (stack_depth())
        assembly().add_instruction("add", "sp,sp,#{}", stack_depth());
    assembly().add_instruction("ldp", "fp,lr,[sp],16");
    assembly().add_instruction("ret");
    pop_stack_depth();
    s_function_stack.pop_back();
}

void ARM64Context::reserve_on_stack(size_t bytes)
{
    if (bytes % 16)
        bytes = bytes + (16 - (bytes % 16));
    assembly().add_instruction("sub", "sp,sp,#{}", bytes);
    m_stack_allocated += bytes;
}

void ARM64Context::release_stack()
{
    assembly().add_instruction("add", "sp,sp,#{}", m_stack_allocated);
    m_stack_allocated = 0;
}

}
