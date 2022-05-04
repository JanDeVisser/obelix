/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "obelix/Context.h"
#include "obelix/Syntax.h"
#include "obelix/Type.h"
#include <filesystem>
#include <memory>

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

ErrorOr<void, SyntaxError> load_variable(ARM64Context &ctx, std::shared_ptr<ObjectType> type, int offset)
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot push values of array elements of type {} yet", type) };

        auto offset_str = format("[fp,#{}]", offset);
        ctx.assembly().add_instruction(mm->load_mnemonic, "{}0,{}", mm->reg_width, offset_str);
    } else {
        if (type->fields().size() > 4) {
            return SyntaxError { ErrorCode::NotYetImplemented, Token {}, format("Cannot load structs with more than 4 fields yet") };
        }
        auto off = offset;
        int target = 0;
        for (auto const& field : type->fields()) {
            if (field.type->type() != PrimitiveType::Struct) {
                auto mm = get_type_mnemonic_map(field.type);
                if (mm == nullptr)
                    return SyntaxError { ErrorCode::NotYetImplemented, Token {}, format("Cannot load struct fields of type '{}' yet", field.type) };
                auto offset_str = format("[fp,#{}]", off);
                ctx.assembly().add_instruction(mm->load_mnemonic, "{}{},{}", mm->reg_width, target++, offset_str);
                off += mm->size;
            } else {
                return SyntaxError { ErrorCode::NotYetImplemented, Token {}, format("Cannot load structs with nested struct fields yet") };
            }
        }
    }
    return {};
}

ErrorOr<void, SyntaxError> assign_variable(ARM64Context &ctx, std::shared_ptr<ObjectType> type, int offset)
{
    if (type->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(type);
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                format("Cannot push values of array elements of type {} yet", type) };

        auto offset_str = format("[fp,#{}]", offset);
        ctx.assembly().add_instruction(mm->store_mnemonic, "{}0,{}", mm->reg_width, offset_str);
    } else {
        if (type->fields().size() > 4) {
            return SyntaxError { ErrorCode::NotYetImplemented, Token {}, format("Cannot assign structs with more than 4 fields yet") };
        }
        auto off = offset;
        auto target = 0;
        for (auto const& field : type->fields()) {
            if (field.type->type() != PrimitiveType::Struct) {
                auto mm = get_type_mnemonic_map(field.type);
                if (mm == nullptr)
                    return SyntaxError { ErrorCode::NotYetImplemented, Token {}, format("Cannot assign struct fields of type '{}' yet", field.type) };
                auto offset_str = format("[fp,#{}]", off);
                ctx.assembly().add_instruction(mm->store_mnemonic, "{}{},{}", mm->reg_width, target++, offset_str);
                off += mm->size;
            } else {
                return SyntaxError { ErrorCode::NotYetImplemented, Token {}, format("Cannot assign structs with nested struct fields yet") };
            }
        }
    }
    return {};
}

ErrorOr<void, SyntaxError> zero_initialize(ARM64Context &ctx, std::shared_ptr<ObjectType> type, int offset)
{
    switch (type->type()) {
    case PrimitiveType::Pointer:
    case PrimitiveType::SignedIntegerNumber:
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::Boolean:
        ctx.assembly().add_instruction("mov", "x0,xzr");
        ctx.assembly().add_instruction("str", "x0,[fp,#{}]", offset);
        break;
    case PrimitiveType::Struct: {
        if (type->fields().size() > 4) {
            return SyntaxError { ErrorCode::NotYetImplemented, Token { }, format("Cannot assign structs with more than 4 fields yet") };
        }
        auto off = offset;
        ctx.assembly().add_instruction("mov", "x0,xzr");
        for (auto const& field : type->fields()) {
            if (field.type->type() != PrimitiveType::Struct) {
                auto mm = get_type_mnemonic_map(field.type);
                if (mm == nullptr)
                    return SyntaxError { ErrorCode::NotYetImplemented, Token {}, format("Cannot assign struct fields of type '{}' yet", field.type) };
                auto offset = format("[fp,#{}]", off);
                ctx.assembly().add_instruction("str", "x0,[fp,#{}]", off);
                off += mm->size;
            } else {
                return SyntaxError { ErrorCode::NotYetImplemented, Token { }, format("Cannot assign structs with nested struct fields yet") };
            }
        }
        break;
    }
    default:
        return SyntaxError { ErrorCode::NotYetImplemented, Token { }, format("Cannot initialize variables of type {} yet", type) };
    }
    return {};
}

ErrorOr<void, SyntaxError> calculate_array_element_offset(ARM64Context &ctx, std::shared_ptr<MaterializedArrayAccess> array_access)
{
    TRY_RETURN(process(array_access->index(), ctx));
    switch (array_access->element_size()) {
        case 1:
            ctx.assembly().add_instruction("add", "x8,x0,#{}", array_access->element_size());
            break;
        case 2:
            ctx.assembly().add_instruction("lsl", "x8,x0,#1");
            break;
        case 4:
            ctx.assembly().add_instruction("lsl", "x8,x0,#2");
            break;
        case 8:
            ctx.assembly().add_instruction("lsl", "x8,x0,#3");
            break;
        case 16:
            ctx.assembly().add_instruction("lsl", "x8,x0,#4");
            break;
        default:
            return SyntaxError { ErrorCode::InternalError, array_access->token(), "Cannot access arrays with elements of size {} yet", array_access->element_size()};
    }
    ctx.assembly().add_instruction("add", "x8,x8,#{}", array_access->array()->offset());
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
        ctx.declare(param->name(), param->offset());
    }

    if (func_def->declaration()->node_type() == SyntaxNodeType::MaterializedFunctionDecl) {
        ctx.enter_function(func_def);
        TRY_RETURN(process(func_def->statement(), ctx));
        ctx.leave_function();
    }
    return tree;
}

ErrorOr<void, SyntaxError> evaluate_arguments(ARM64Context& ctx, BoundExpressions const& arguments)
{
    int total_registers { 0 };
    for (auto const& arg : arguments) {
        total_registers += (arg->type()->type() == PrimitiveType::Struct) ? arg->type()->fields().size() : 1;
    }

    int pushed_registers { 0 };
    for (auto iter = arguments.rbegin(); iter != arguments.rend(); ++iter) {
        auto const& arg = *iter;
        TRY_RETURN(process(arg, ctx));
        int num_registers = (arg->type()->type() == PrimitiveType::Struct) ? arg->type()->fields().size() : 1;
        if (pushed_registers + num_registers < total_registers) {
            for (int r = num_registers - 1; r >= 0; --r) {
                push(ctx, format("x{}", r));
            }
            pushed_registers += num_registers;
        }
    }
    for (auto ix = 0; ix < pushed_registers; ++ix) {
        auto reg = format("x{}", total_registers - ix - 1);
        pop(ctx, reg);
    }
    return {};
}

NODE_PROCESSOR(BoundFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
    TRY_RETURN(evaluate_arguments(ctx, call->arguments()));
    ctx.assembly().add_instruction("bl", call->name());
    return tree;
}

NODE_PROCESSOR(BoundNativeFunctionCall)
{
    auto native_func_call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
    auto func_decl = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(native_func_call->declaration());
    TRY_RETURN(evaluate_arguments(ctx, native_func_call->arguments()));
    ctx.assembly().add_instruction("bl", func_decl->native_function_name());
    return tree;
}

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);

    TRY_RETURN(evaluate_arguments(ctx, call->arguments()));
    ARM64Implementation impl = get_arm64_intrinsic(call->intrinsic());
    if (!impl)
        return SyntaxError { ErrorCode::InternalError, call->token(), format("No ARM64 implementation for intrinsic {}", call->to_string()) };
    auto ret = impl(ctx);
    if (ret.is_error())
        return ret.error();
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
    std::string source = format("[fp,#{}]", identifier->offset());
    if (!identifier->label().empty()) {
        ctx.assembly().add_instruction("adrp", "x8,{}@PAGE", identifier->label());
        source = format("[x8,{}@PAGEOFF]", identifier->label());
    }

    auto mm = get_type_mnemonic_map(identifier->type());
    if (mm == nullptr)
        return SyntaxError { ErrorCode::NotYetImplemented, identifier->token(),
            format("Cannot push values of variables of type {} yet", identifier->type()) };

    ctx.assembly().add_instruction(mm->load_mnemonic, "{}0,{}", mm->reg_width, source);
    return tree;
}

NODE_PROCESSOR(MaterializedStructIdentifier)
{
    auto identifier = std::dynamic_pointer_cast<MaterializedVariableAccess>(tree);
#if 0
    auto target = ctx.target_register();
    std::string source;

    if (!identifier->label().empty()) {
        ctx.inc_target_register();
        auto ptr_reg = ctx.target_register();
        ctx.assembly().add_instruction("adrp", "x{},{}@PAGE", ptr_reg, identifier->label());
        source = format("x{},{}@PAGEOFF", ptr_reg, identifier->label());
    } else {
        source = format("fp,#{}", identifier->offset());
    }
    ctx.assembly().add_instruction("add", "x{},{}", target, source);
#endif
    TRY_RETURN(load_variable(ctx, identifier->type(), identifier->offset()));
    return tree;
}

NODE_PROCESSOR(MaterializedMemberAccess)
{
    auto member_access = std::dynamic_pointer_cast<MaterializedMemberAccess>(tree);
    TRY_RETURN(process(member_access->member(), ctx));
    return tree;
}

NODE_PROCESSOR(MaterializedArrayAccess)
{
    auto array_access = std::dynamic_pointer_cast<MaterializedArrayAccess>(tree);
    TRY_RETURN(calculate_array_element_offset(ctx, array_access));

    if (array_access->type()->type() != PrimitiveType::Struct) {
        auto mm = get_type_mnemonic_map(array_access->type());
        if (mm == nullptr)
            return SyntaxError { ErrorCode::NotYetImplemented, array_access->token(),
                format("Cannot push values of array elements of type {} yet", array_access->type()) };

        ctx.assembly().add_instruction(mm->load_mnemonic, "{}0,[fp,x8]", mm->reg_width);
    } else {
        ctx.assembly().add_instruction("add", "x0,fp,x8");
    }
    return tree;
}

NODE_PROCESSOR(BoundAssignment)
{
    auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
    auto assignee = std::dynamic_pointer_cast<MaterializedVariableAccess>(assignment->assignee());
    if (assignee == nullptr)
        return SyntaxError { ErrorCode::InternalError, assignment->token(), format("Variable access '{}' not materialized", assignment->assignee()) };

    TRY_RETURN(process(assignment->expression(), ctx));

    auto offset = format("[fp,#{}]", assignee->offset());
    auto array_access = std::dynamic_pointer_cast<MaterializedArrayAccess>(assignee);
    if (array_access) {
        TRY_RETURN(calculate_array_element_offset(ctx, array_access));
        offset = "[fp,x8]";
    }
    TRY_RETURN(assign_variable(ctx, assignee->type(), assignee->offset()));
    return tree;
}

NODE_PROCESSOR(MaterializedVariableDecl)
{
    auto var_decl = std::dynamic_pointer_cast<MaterializedVariableDecl>(tree);
    ctx.assembly().add_comment(var_decl->to_string());

    if (var_decl->type()->type() == PrimitiveType::Array)
        return tree;

    if (!var_decl->label().empty()) {
        int initial_value = 0;
        auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(var_decl->expression());
        if (literal != nullptr)
            initial_value = literal->value();
        ctx.assembly().add_data(var_decl->label(), true, ".long", initial_value);
        return tree;
    }

    if (var_decl->expression() != nullptr) {
        TRY_RETURN(process(var_decl->expression(), ctx));
        TRY_RETURN(assign_variable(ctx, var_decl->type(), var_decl->offset()));
    } else {
        TRY_RETURN(zero_initialize(ctx, var_decl->type(), var_decl->offset()));
    }
    return tree;
}

NODE_PROCESSOR(BoundExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
    debug(parser, "{}", expr_stmt->to_string());
    ctx.assembly().add_comment(expr_stmt->to_string());
    TRY_RETURN(process(expr_stmt->expression(), ctx));
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
            auto cond = TRY_AND_CAST(Expression, process(branch->condition(), ctx));
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
    if (config.show_tree)
        std::cout << "\n\nMaterialized:\n" << processed->to_xml() << "\n";
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

            std::cout << bare_file_name << ".s:"
                      << "\n";
            std::cout << assembly.to_string();

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
