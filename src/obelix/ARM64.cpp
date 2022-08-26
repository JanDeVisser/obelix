/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "obelix/Type.h"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>

#include <config.h>

#include <core/Error.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <core/ScopeGuard.h>
#include <obelix/ARM64.h>
#include <obelix/ARM64Context.h>
#include <obelix/ARM64Intrinsics.h>
#include <obelix/MaterializedSyntaxNode.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

logging_category(arm64);

struct TypeMnemonicMap {
    PrimitiveType type;
    bool is_signed;
    int size;
    char const* load_mnemonic;
    char const* store_mnemonic;
    char const* reg_width;
};

static TypeMnemonicMap mnemonic_map[] = {
    { PrimitiveType::SignedIntegerNumber, true, 8, "ldr", "str", "x"},
    { PrimitiveType::IntegerNumber, false, 8, "ldr", "str", "x"},
    { PrimitiveType::Pointer, false, 8, "ldr", "str", "x"},
    { PrimitiveType::SignedIntegerNumber, true, 4, "ldr", "str", "w"},
    { PrimitiveType::IntegerNumber, false, 4, "ldr", "str", "w"},
    { PrimitiveType::SignedIntegerNumber, true, 1, "ldrsb", "strsb", "w"},
    { PrimitiveType::IntegerNumber, false, 1, "ldrb", "strb", "w"},
};

TypeMnemonicMap const* get_type_mnemonic_map(std::shared_ptr<ObjectType> type)
{
    bool is_signed = (type->has_template_argument("signed")) && type->template_argument<bool>("signed");
    int size = (type->has_template_argument("size")) ? type->template_argument<long>("size") : type->size();
    for (auto const& mm : mnemonic_map) {
        if (mm.type != type->type())
            continue;
        if (mm.is_signed == is_signed && mm.size == size) {
            return &mm;
        }
    }
    return nullptr;
}

ErrorOr<void, SyntaxError> zero_initialize(ARM64Context &ctx, std::shared_ptr<ObjectType> type, int offset)
{
    switch (type->type()) {
    case PrimitiveType::Pointer:
    case PrimitiveType::SignedIntegerNumber:
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::Boolean:
        ctx.assembly().add_instruction("mov", "x0,xzr");
        ctx.assembly().add_instruction("str", "x0,[fp,#{}]", ctx.stack_depth() - offset);
        break;
    case PrimitiveType::Struct: {
        auto off = ctx.stack_depth() - offset;
        for (auto const& field : type->fields()) {
            TRY_RETURN(zero_initialize(ctx, field.type, off));
            off += field.type->size();
        }
        break;
    }
    case PrimitiveType::Array: {
        // Arrays are not initialized now. Maybe that should be fixed
        break;
    }
    default:
        return SyntaxError { ErrorCode::NotYetImplemented, Token { }, format("Cannot initialize variables of type {} yet", type) };
    }
    return {};
}

INIT_NODE_PROCESSOR(ARM64Context);

NODE_PROCESSOR(Compilation)
{
    auto compilation = std::dynamic_pointer_cast<Compilation>(tree);
    ctx.add_module(ARM64Context::ROOT_MODULE_NAME);
    return process_tree(tree, ctx, ARM64Context_processor);
}

NODE_PROCESSOR(Module)
{
    auto module = std::dynamic_pointer_cast<Module>(tree);
    ctx.add_module(join(split(module->name(), '/'), "-"));
    return process_tree(tree, ctx, ARM64Context_processor);
}

NODE_PROCESSOR(MaterializedFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<MaterializedFunctionDef>(tree);

    for (auto& param : func_def->declaration()->parameters()) {
        auto address = std::dynamic_pointer_cast<StackVariableAddress>(param->address());
        ctx.declare(param->name(), address->offset());
    }

    if (func_def->declaration()->node_type() == SyntaxNodeType::MaterializedFunctionDecl) {
        ctx.enter_function(func_def);
        TRY_RETURN(process(func_def->statement(), ctx));
        ctx.leave_function();
    }
    return tree;
}

ErrorOr<void, SyntaxError> evaluate_arguments(ARM64Context& ctx, std::shared_ptr<MaterializedFunctionDecl> const& decl, BoundExpressions const& arguments)
{
    int nsaa = decl->nsaa();
    if (nsaa > 0) {
        push(ctx, "x10");
        ctx.assembly().add_instruction("mov", "x10,sp");
        ctx.assembly().add_instruction("sub", "sp,sp,#{}", nsaa);
    }
    auto param_defs = decl->parameters();
    int param_ix = 0;
    for (auto const& arg : arguments) {
        //auto sz = arg->type()->size();
        TRY_RETURN(process(arg, ctx));
        if (arguments.size() > 1) {
            auto type = param_defs[param_ix]->type();
            auto t = type->type();
            if (t == PrimitiveType::Compatible)
                t = param_defs[0]->type()->type();
            switch (param_defs[param_ix]->method()) {
                case MaterializedFunctionParameter::ParameterPassingMethod::Register:
                    switch (t) {
                        case PrimitiveType::Boolean:
                        case PrimitiveType::IntegerNumber:
                        case PrimitiveType::SignedIntegerNumber:
                        case PrimitiveType::Pointer:
                            push(ctx, "x0");
                            break;
                        case PrimitiveType::Struct: {
                            int reg = 0;
                            for (auto const& field : param_defs[param_ix]->type()->fields()) {
                                (void) field;
                                push(ctx, format("x{}", reg++));
                            }
                            break;
                        }
                        default:
                            fatal("Type '{}' cannot passed in a register in {}", param_defs[param_ix]->type(), __func__);
                    }
                    break;
                case MaterializedFunctionParameter::ParameterPassingMethod::Stack:
                    switch (t) {
                        case PrimitiveType::IntegerNumber:
                        case PrimitiveType::SignedIntegerNumber:
                        case PrimitiveType::Pointer:
                            ctx.assembly().add_instruction("str", "x0,[x10,#-{}]", param_defs[param_ix]->where());
                            break;
                        case PrimitiveType::Struct:
                        default:
                            ctx.assembly().add_text(
                            R"(

                            )"
                            );
                            fatal("Type '{}' cannot passed on the stack in {}", param_defs[param_ix]->type(), __func__);
                    }
                    break;
            }
        }
        param_ix++;
    }
    if (arguments.size() > 1) {
        for (auto it = param_defs.rbegin(); it != param_defs.rend(); ++it) {
            auto param = *it;
            if (param->method() != MaterializedFunctionParameter::ParameterPassingMethod::Register)
                continue;
            auto size_in_double_words = param->type()->size() / 8;
            if (param->type()->size() % 8 != 0)
                size_in_double_words++;
            for (int reg = (int) (size_in_double_words - 1); reg >= 0; --reg) {
                pop(ctx, format("x{}", param->where() + reg));
            }
        }
    }
    return {};
}

void reset_sp_after_call(ARM64Context& ctx, std::shared_ptr<MaterializedFunctionDecl> const& decl)
{
    if (decl->nsaa() > 0) {
        ctx.assembly().add_instruction("mov", "sp,x10");
        pop(ctx, "x10");
    }
}

NODE_PROCESSOR(MaterializedFunctionCall)
{
    auto call = std::dynamic_pointer_cast<MaterializedFunctionCall>(tree);
    TRY_RETURN(evaluate_arguments(ctx, call->declaration(), call->arguments()));
    ctx.assembly().add_instruction("bl", call->declaration()->label());
    reset_sp_after_call(ctx, call->declaration());
    return tree;
}

NODE_PROCESSOR(MaterializedNativeFunctionCall)
{
    auto native_func_call = std::dynamic_pointer_cast<MaterializedNativeFunctionCall>(tree);
    auto func_decl = std::dynamic_pointer_cast<MaterializedNativeFunctionDecl>(native_func_call->declaration());
    TRY_RETURN(evaluate_arguments(ctx, func_decl, native_func_call->arguments()));
    ctx.assembly().add_instruction("bl", func_decl->native_function_name());
    reset_sp_after_call(ctx, func_decl);
    return tree;
}

NODE_PROCESSOR(MaterializedIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<MaterializedIntrinsicCall>(tree);

    TRY_RETURN(evaluate_arguments(ctx, call->declaration(), call->arguments()));
    ARM64Implementation impl = get_arm64_intrinsic(call->intrinsic());
    if (!impl)
        return SyntaxError { ErrorCode::InternalError, call->token(), format("No ARM64 implementation for intrinsic {}", call->to_string()) };
    auto ret = impl(ctx);
    if (ret.is_error())
        return ret.error();
    reset_sp_after_call(ctx, call->declaration());
    return tree;
}

NODE_PROCESSOR(BoundIntLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(tree);
    auto mm = get_type_mnemonic_map(literal->type());
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, literal->token(),
            format("Cannot push values of variables of type {} yet", literal->type()) };

    ctx.assembly().add_instruction("mov", "{}0,#{}", mm->reg_width, literal->value());
    return tree;
}

ErrorOr<void, SyntaxError> StackVariableAddress::load_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int target) const
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot load values of variables of type {} yet", type) };
        ctx.assembly().add_comment(format("Loading variable: stack_depth {} offset {}", ctx.stack_depth(), offset()));
        ctx.assembly().add_instruction(mm->load_mnemonic, "{}{},[fp,#{}]", mm->reg_width, target, ctx.stack_depth() - offset());
        return {};
    }
    ctx.assembly().add_comment(format("Loading struct variable: stack_depth {} offset {}", ctx.stack_depth(), offset()));
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        ctx.assembly().add_instruction("ldr", format("{}{},[fp,#{}]", reg_width, target++, ctx.stack_depth() - offset() + type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> StackVariableAddress::store_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int from) const
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot store values type {} yet", type) };
        ctx.assembly().add_comment(format("Storing to variable: stack_depth {} offset {}", ctx.stack_depth(), offset()));
        ctx.assembly().add_instruction(mm->store_mnemonic, "{}{},[fp,#{}]", mm->reg_width, from, ctx.stack_depth() - offset());
        return {};
    }
    ctx.assembly().add_comment(format("Storing struct variable: stack_depth {} offset {}", ctx.stack_depth(), offset()));
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        ctx.assembly().add_instruction("str", format("{}{},[fp,#{}]", reg_width, from++, ctx.stack_depth() - offset() + type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> StackVariableAddress::prepare_pointer(ARM64Context& ctx) const
{
    ctx.assembly().add_instruction("add", "x8,fp,#{}", ctx.stack_depth() - offset());
    return {};
}

ErrorOr<void, SyntaxError> StructMemberAddress::load_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int target) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot push values of variables of type {} yet", type) };

    if (auto error_maybe = structure()->prepare_pointer(ctx); error_maybe.is_error()) {
        return error_maybe.error();
    }
    if (offset() > 0)
        ctx.assembly().add_instruction("sub", "x8,x8,#{}", ctx.stack_depth() - offset());
    ctx.assembly().add_instruction(mm->load_mnemonic, "{}{},[x8]", mm->reg_width, target);
    return {};
}

ErrorOr<void, SyntaxError> StructMemberAddress::store_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int from) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot store values type {} yet", type) };
    if (auto error_maybe = structure()->prepare_pointer(ctx); error_maybe.is_error()) {
        return error_maybe.error();
    }
    if (offset() > 0)
        ctx.assembly().add_instruction("add", "x8,x8,#{}", ctx.stack_depth() - offset());
    ctx.assembly().add_instruction(mm->store_mnemonic, "{}{},[x8]", mm->reg_width, from);
    return {};
}

ErrorOr<void, SyntaxError> StructMemberAddress::prepare_pointer(ARM64Context& ctx) const
{
    ctx.assembly().add_instruction("add", "x8,x8,#{}", ctx.stack_depth() - offset());
    return {};
}

ErrorOr<void, SyntaxError> ArrayElementAddress::load_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int target) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot push values of variables of type {} yet", type) };

    TRY_RETURN(array()->prepare_pointer(ctx));
    TRY_RETURN(prepare_pointer(ctx));
    pop(ctx, "x0");
    ctx.assembly().add_instruction(mm->load_mnemonic, "{}{},[x8]", mm->reg_width, target);
    return {};
}

ErrorOr<void, SyntaxError> ArrayElementAddress::store_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int from) const
{
    auto mm = get_type_mnemonic_map(type);
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, Token {},
            format("Cannot store values type {} yet", type) };
    TRY_RETURN(array()->prepare_pointer(ctx));
    TRY_RETURN(prepare_pointer(ctx));
    pop(ctx, "x0");
    ctx.assembly().add_instruction(mm->store_mnemonic, "{}{},[x8]", mm->reg_width, from);
    return {};
}

ErrorOr<void, SyntaxError> ArrayElementAddress::prepare_pointer(ARM64Context& ctx) const
{
    // x0 will hold the array index. Here we add that index, multiplied by the element size,
    // to x8, which should hold the array base address
    
    switch (element_size()) {
        case 1:
            ctx.assembly().add_instruction("add", "x8,x8,x0");
            break;
        case 2:
            ctx.assembly().add_instruction("add", "x8,x8,x0,lsl #1");
            break;
        case 4:
            ctx.assembly().add_instruction("add", "x8,x8,x0,lsl #2");
            break;
        case 8:
            ctx.assembly().add_instruction("add", "x8,x8,x0,lsl #3");
            break;
        case 16:
            ctx.assembly().add_instruction("add", "x8,x8,x0,lsl #4");
            break;
        default:
            return SyntaxError { ErrorCode::InternalError, Token {}, "Cannot access arrays with elements of size {} yet", element_size()};
    }
    return {};
}

ErrorOr<void, SyntaxError> StaticVariableAddress::load_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int target) const
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot push values of variables of type {} yet", type) };

        ctx.assembly().add_instruction("adrp", "x8,{}@PAGE", label());
        ctx.assembly().add_instruction(mm->load_mnemonic, "{}{},[x8,{}@PAGEOFF]", mm->reg_width, target, label());
        return {};
    }
    ctx.assembly().add_comment("Loading static struct variable");
    ctx.assembly().add_instruction("adrp", "x8,{}@PAGE", label());
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        ctx.assembly().add_instruction("ldr", format("{}{},[x8,{}@PAGEOFF+{}]", reg_width, target++, label(), type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> StaticVariableAddress::store_variable(std::shared_ptr<ObjectType> type, ARM64Context& ctx, int from) const
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot store values of type {} yet", type) };

        ctx.assembly().add_instruction("adrp", "x8,{}@PAGE", label());
        ctx.assembly().add_instruction(mm->store_mnemonic, "{}{},[x8,{}@PAGEOFF]", mm->reg_width, from, label());
        return {};
    }
    ctx.assembly().add_comment("Storing static struct variable");
    ctx.assembly().add_instruction("adrp", "x8,{}@PAGE", label());
    for (auto const& field : type->fields()) {
        auto reg_width = "w";
        if (field.type->size() > 4)
            reg_width = "x";
        ctx.assembly().add_instruction("str", format("{}{},[x8,{}@PAGEOFF+{}]", reg_width, from++, label(), type->offset_of(field.name)));
    }
    return {};
}

ErrorOr<void, SyntaxError> StaticVariableAddress::prepare_pointer(ARM64Context& ctx) const
{
    ctx.assembly().add_instruction("adrp", "x8,{}@PAGE", label());
    ctx.assembly().add_instruction("add", "x8,x8,{}@PAGEOFF", label());
    return {};
}

NODE_PROCESSOR(BoundStringLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundStringLiteral>(tree);
    auto str_id = ctx.assembly().add_string(literal->value());
    ctx.assembly().add_instruction("mov", "w0,#{}", literal->value().length());
    ctx.assembly().add_instruction("adr", "x1,str_{}", str_id);
    return tree;
}

NODE_PROCESSOR(MaterializedIntIdentifier)
{
    auto identifier = std::dynamic_pointer_cast<MaterializedVariableAccess>(tree);
    TRY_RETURN(identifier->address()->load_variable(identifier->type(), ctx, 0));
    return tree;
}

NODE_PROCESSOR(MaterializedStructIdentifier)
{
    auto identifier = std::dynamic_pointer_cast<MaterializedVariableAccess>(tree);
    TRY_RETURN(identifier->address()->load_variable(identifier->type(), ctx, 0));
    return tree;
}

ALIAS_NODE_PROCESSOR(MaterializedArrayIdentifier, MaterializedStructIdentifier)

NODE_PROCESSOR(MaterializedMemberAccess)
{
    auto member_access = std::dynamic_pointer_cast<MaterializedMemberAccess>(tree);
    TRY_RETURN(process(member_access->member(), ctx));
    return tree;
}

NODE_PROCESSOR(MaterializedArrayAccess)
{
    auto array_access = std::dynamic_pointer_cast<MaterializedArrayAccess>(tree);
    push(ctx, "x0");
    TRY_RETURN(process(array_access->index(), ctx));;
    TRY_RETURN(array_access->address()->load_variable(array_access->type(), ctx, 0));
    return tree;
}

NODE_PROCESSOR(BoundAssignment)
{
    auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
    auto assignee = std::dynamic_pointer_cast<MaterializedVariableAccess>(assignment->assignee());
    if (assignee == nullptr)
        return SyntaxError { ErrorCode::InternalError, assignment->token(), format("Variable access '{}' not materialized", assignment->assignee()) };

    TRY_RETURN(process(assignment->expression(), ctx));
    auto array_access = std::dynamic_pointer_cast<MaterializedArrayAccess>(assignee);
    if (array_access) {
        push(ctx, "x0");
        TRY_RETURN(process(array_access->index(), ctx));;
    }
    TRY_RETURN(assignee->address()->store_variable(assignment->type(), ctx, 0));
    return tree;
}

NODE_PROCESSOR(MaterializedVariableDecl)
{
    auto var_decl = std::dynamic_pointer_cast<MaterializedVariableDecl>(tree);
    ctx.assembly().add_comment(var_decl->to_string());

    auto static_address = std::dynamic_pointer_cast<StaticVariableAddress>(var_decl->address());
    if (static_address) {
        switch (var_decl->type()->type()) {
            case PrimitiveType::IntegerNumber: {
                int initial_value = 0;
                auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(var_decl->expression());
                if (literal != nullptr)
                    initial_value = literal->value();
                ctx.assembly().add_data(static_address->label(), true, ".long", true, initial_value);
                break;
            }
            case PrimitiveType::Array: {
                ctx.assembly().add_data(static_address->label(), true, ".space", 
                    var_decl->type()->template_argument<std::shared_ptr<ObjectType>>("base_type")->size() * 
                    true, var_decl->type()->template_argument<long>("size"));
                break;
            }
            case PrimitiveType::Struct: {
                // ctx.assembly().add_comment("Reserving space for static struct");
                ctx.assembly().add_data(static_address->label(), true, ".space", true, var_decl->type()->size());
                break;
            }
            default:
                fatal("Can't emit static variables of type {} yet", var_decl->type()->type());
        }
    }

    // ctx.assembly().add_comment("Initializing variable");
    if (var_decl->expression() != nullptr) {
        auto skip_label = Obelix::Label::reserve_id();
        if (static_address) {
            ctx.assembly().add_instruction("adrp", "x8,{}@PAGE", static_address->label());
            ctx.assembly().add_instruction("ldr", "w0,[x8,{}@PAGEOFF+{}]", static_address->label(), var_decl->type()->size());
            ctx.assembly().add_instruction("cmp", "w0,0x00");
            ctx.assembly().add_instruction("b.ne", "lbl_{}", skip_label);
        }
        TRY_RETURN(process(var_decl->expression(), ctx));
        auto error_maybe = var_decl->address()->store_variable(var_decl->type(), ctx, 0);
        if (error_maybe.is_error()) {
            ctx.add_error(error_maybe.error());
            return error_maybe.error();
        }
        if (static_address) {
            ctx.assembly().add_instruction("mov", "w0,1");
            ctx.assembly().add_instruction("str", "w0,[x8,{}@PAGEOFF+{}]", static_address->label(), var_decl->type()->size());
            ctx.assembly().add_label(format("lbl_{}", skip_label));
        }
    } else {
        auto stack_address = std::dynamic_pointer_cast<StackVariableAddress>(var_decl->address());
        TRY_RETURN(zero_initialize(ctx, var_decl->type(), stack_address->offset()));
    }
    return tree;
}

NODE_PROCESSOR(BoundExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
    debug(parser, "{}", expr_stmt->to_string());
    ctx.assembly().add_comment(expr_stmt->to_string());
    if (expr_stmt->expression()->type()->type() == PrimitiveType::Struct) {
        ctx.assembly().add_instruction("sub", "sp,sp,#{}", expr_stmt->expression()->type()->size());
        ctx.assembly().add_instruction("mov", "x8,sp");
    }
    TRY_RETURN(process(expr_stmt->expression(), ctx));
    if (expr_stmt->expression()->type()->type() == PrimitiveType::Struct) {
        ctx.assembly().add_instruction("add", "sp,sp,#{}", expr_stmt->expression()->type()->size());
    }
    return tree;
}

NODE_PROCESSOR(BoundReturn)
{
    auto ret = std::dynamic_pointer_cast<BoundReturn>(tree);
    debug(parser, "{}", ret->to_string());
    ctx.assembly().add_comment(ret->to_string());
    TRY_RETURN(process(ret->expression(), ctx));
    ctx.function_return();

    return tree;
}

NODE_PROCESSOR(Label)
{
    auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
    debug(parser, "{}", label->to_string());
    ctx.assembly().add_comment(label->to_string());
    ctx.assembly().add_label(format("lbl_{}", label->label_id()));
    return tree;
}

NODE_PROCESSOR(Goto)
{
    auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
    debug(parser, "{}", goto_stmt->to_string());
    ctx.assembly().add_comment(goto_stmt->to_string());
    ctx.assembly().add_instruction("b", "lbl_{}", goto_stmt->label_id());
    return tree;
}

NODE_PROCESSOR(BoundIfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);

    auto end_label = Obelix::Label::reserve_id();
    auto count = if_stmt->branches().size() - 1;
    for (auto& branch : if_stmt->branches()) {
        auto else_label = (count) ? Obelix::Label::reserve_id() : end_label;
        if (branch->condition()) {
            debug(parser, "if ({})", branch->condition()->to_string());
            ctx.assembly().add_comment(format("if ({})", branch->condition()->to_string()));
            auto cond = TRY_AND_CAST(BoundExpression, process(branch->condition(), ctx));
            ctx.assembly().add_instruction("cmp", "w0,0x00");
            ctx.assembly().add_instruction("b.eq", "lbl_{}", else_label);
        } else {
            ctx.assembly().add_comment("else");
        }
        auto stmt = TRY_AND_CAST(Statement, process(branch->statement(), ctx));
        if (count) {
            ctx.assembly().add_instruction("b", "lbl_{}", end_label);
            ctx.assembly().add_label(format("lbl_{}", else_label));
        }
        count--;
    }
    ctx.assembly().add_label(format("lbl_{}", end_label));
    return tree;
}

ErrorOrNode output_arm64(std::shared_ptr<SyntaxNode> const& tree, Config const& config, std::string const& file_name)
{
    auto processed = TRY(materialize_arm64(tree));
    if (config.cmdline_flag("show-tree"))
        std::cout << "\n\nMaterialized:\n" 
            << std::dynamic_pointer_cast<Compilation>(processed)->root_to_xml()
            << "\n"
            << processed->to_xml() 
            << "\n";
    if (!config.compile)
        return processed;

    ARM64Context root;
    auto ret = process(processed, root);

    if (ret.is_error()) {
        return ret;
    }

    namespace fs = std::filesystem;
    fs::create_directory(".obelix");

    std::vector<std::string> modules;
    for (auto& module_assembly : ARM64Context::assemblies()) {
        auto& module = module_assembly.first;
        auto& assembly = module_assembly.second;

        if (assembly.has_exports()) {
            auto file_name_parts = split(module, '.');
            auto bare_file_name = ".obelix/" + file_name_parts.front();

            if (config.cmdline_flag("show-assembly")) {
                std::cout << bare_file_name << ".s:"
                        << "\n";
                std::cout << assembly.to_string();
            }

            TRY_RETURN(assembly.save_and_assemble(bare_file_name));
            modules.push_back(bare_file_name + ".o");
        }
    }

    if (!modules.empty()) {
        std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;

        auto file_parts = split(file_name, '/');
        auto file_name_parts = split(file_parts.back(), '.');
        auto bare_file_name = file_name_parts.front();

        static std::string sdk_path; // "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk";
        if (sdk_path.empty()) {
            Process p("xcrun", "-sdk", "macosx", "--show-sdk-path");
            if (auto exit_code_or_error = p.execute(); exit_code_or_error.is_error())
                return SyntaxError { exit_code_or_error.error(), Token {} };
            sdk_path = strip(p.standard_out());
        }

        std::vector<std::string> ld_args = { "-o", bare_file_name, "-loblrt", "-lSystem", "-syslibroot", sdk_path, "-e", "_start", "-arch", "arm64",
            format("-L{}/lib", obl_dir) };
        for (auto& m : modules)
            ld_args.push_back(m);

        if (auto code = execute("ld", ld_args); code.is_error())
            return SyntaxError { code.error(), Token {} };
        if (config.run) {
            auto run_cmd = format("./{}", bare_file_name);
            auto exit_code = execute(run_cmd);
            if (exit_code.is_error())
                return SyntaxError { exit_code.error(), Token {} };
            ret = make_node<BoundIntLiteral>(Token { TokenCode::Integer, format("{}", exit_code.value()) }, (long) exit_code.value());
        }
    }
    return ret;
}

}
