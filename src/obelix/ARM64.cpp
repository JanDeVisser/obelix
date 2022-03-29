/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <filesystem>

#include <config.h>
#include <core/Error.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <core/ScopeGuard.h>
#include <memory>
#include <obelix/ARM64.h>
#include <obelix/ARM64Context.h>
#include <obelix/ARM64Intrinsics.h>
#include <obelix/MaterializedSyntaxNode.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

logging_category(arm64);

INIT_NODE_PROCESSOR(ARM64Context);

NODE_PROCESSOR(Compilation)
{
    auto compilation = std::dynamic_pointer_cast<Compilation>(tree);
    ctx.add_module(ARM64Context::ROOT_MODULE_NAME);
    return process_tree(tree, ctx, ARM64Context_processor);
});

NODE_PROCESSOR(Module)
{
    auto module = std::dynamic_pointer_cast<Module>(tree);
    ctx.add_module(join(split(module->name(), '/'), "-"));
    return process_tree(tree, ctx, ARM64Context_processor);
});

NODE_PROCESSOR(MaterializedFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<MaterializedFunctionDef>(tree);

    for (auto& param : func_def->declaration()->parameters()) {
        ctx.declare(param->name(), param->offset());
    }

    if (func_def->declaration()->node_type() == SyntaxNodeType::MaterializedFunctionDecl) {
        ctx.enter_function(func_def);
        TRY_RETURN(ARM64Context_processor(func_def->statement(), ctx));
        ctx.leave_function();
    }
    return tree;
});

NODE_PROCESSOR(BoundFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);

    // Load arguments in registers:
    ctx.initialize_target_register();
    for (auto& argument : call->arguments()) {
        TRY_RETURN(ARM64Context_processor(argument, ctx));
        ctx.inc_target_register();
    }

    // Call function:
    ctx.assembly().add_instruction("bl", call->name());
    ctx.release_target_register(call->type()->type());
    return tree;
});

NODE_PROCESSOR(BoundNativeFunctionCall)
{
    auto native_func_call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
    auto func_decl = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(native_func_call->declaration());

    ctx.initialize_target_register();
    for (auto& arg : native_func_call->arguments()) {
        TRY_RETURN(ARM64Context_processor(arg, ctx));
        ctx.inc_target_register();
    }

    // Call the native function
    ctx.assembly().add_instruction("bl", func_decl->native_function_name());
    ctx.release_target_register(native_func_call->type()->type());
    return tree;
});

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);

    ctx.initialize_target_register();
    for (auto& arg : call->arguments()) {
        TRY_RETURN(ARM64Context_processor(arg, ctx));
        ctx.inc_target_register();
    }
    ARM64Implementation impl = get_arm64_intrinsic(call->intrinsic());
    if (!impl)
        return Error { ErrorCode::InternalError, format("No ARM64 implementation for intrinsic {}", call->to_string()) };
    auto ret = impl(ctx);
    if (ret.is_error())
        return ret.error();
    ctx.release_target_register(call->type()->type());
    return tree;
});

NODE_PROCESSOR(BoundLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundLiteral>(tree);
    auto target = ctx.target_register();
    switch (literal->type()->type()) {
    case PrimitiveType::Pointer:
    case PrimitiveType::Int:
    case PrimitiveType::Unsigned: {
        ctx.assembly().add_instruction("mov", "x{},#{}", target, literal->int_value());
        break;
    }
    case PrimitiveType::Char:
    case PrimitiveType::Byte:
    case PrimitiveType::Boolean: {
        ctx.assembly().add_instruction("mov", "w{},#{}", target, static_cast<uint8_t>(literal->int_value()));
        break;
    }
    case PrimitiveType::String: {
        auto str_id = ctx.assembly().add_string(literal->string_value());
        ctx.assembly().add_instruction("adr", "x{},str_{}", target, str_id);
        ctx.assembly().add_instruction("mov", "w{},#{}", target + 1, literal->string_value().length());
        ctx.inc_target_register();
        break;
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", literal->type()->to_string()));
    }
    return tree;
});

NODE_PROCESSOR(MaterializedIdentifier)
{
    auto identifier = std::dynamic_pointer_cast<MaterializedIdentifier>(tree);
    auto target = ctx.target_register();

    switch (identifier->type()->type()) {
    case PrimitiveType::Pointer:
    case PrimitiveType::Int:
        ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", target, identifier->offset());
        break;
    case PrimitiveType::Unsigned:
        ctx.assembly().add_instruction("ldr", "x0,[fp,#{}]", target, identifier->offset());
        break;
    case PrimitiveType::Byte:
        ctx.assembly().add_instruction("ldrbs", "w{},[fp,#{}]", target, identifier->offset());
        break;
    case PrimitiveType::Char:
    case PrimitiveType::Boolean:
        ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", target, identifier->offset());
        break;
    case PrimitiveType::String: {
        ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", target, identifier->offset());
        ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", target + 1, identifier->offset()+ 8);
        ctx.inc_target_register();
        break;
    }
    case PrimitiveType::Struct: {
        ctx.assembly().add_instruction("add", "x{},fp,#{}", target, identifier->offset());
        ctx.inc_target_register();
        break;
    }
    default:
        return Error { ErrorCode::NotYetImplemented, format("Cannot push values of variables of type {} yet", identifier->type()) };
    }
    return tree;
});

NODE_PROCESSOR(MaterializedMemberAccess)
{
    auto member_access = std::dynamic_pointer_cast<MaterializedMemberAccess>(tree);
    auto target = ctx.target_register();

    switch (member_access->type()->type()) {
    case PrimitiveType::Pointer:
    case PrimitiveType::Int:
        ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", target, member_access->offset());
        break;
    case PrimitiveType::Unsigned:
        ctx.assembly().add_instruction("ldr", "x0,[fp,#{}]", target, member_access->offset());
        break;
    case PrimitiveType::Byte:
        ctx.assembly().add_instruction("ldrbs", "w{},[fp,#{}]", target, member_access->offset());
        break;
    case PrimitiveType::Char:
    case PrimitiveType::Boolean:
        ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", target, member_access->offset());
        break;
    case PrimitiveType::String: {
        ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", target, member_access->offset());
        ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", target + 1, member_access->offset()+ 8);
        ctx.inc_target_register();
        break;
    }
    case PrimitiveType::Struct: {
        ctx.assembly().add_instruction("add", "x{},fp,#{}", target, member_access->offset());
        ctx.inc_target_register();
        break;
    }
    default:
        return Error { ErrorCode::NotYetImplemented, format("Cannot push values of struct members of type {} yet", member_access->type()) };
    }
    return tree;
});

NODE_PROCESSOR(BoundAssignment)
{
    auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
    auto assignee = std::dynamic_pointer_cast<MaterializedVariableAccess>(assignment->assignee());
    if (assignee == nullptr)
        return Error { ErrorCode::InternalError, format("Variable access '{}' not materialized", assignment->assignee()) };
    ctx.initialize_target_register();
    TRY_RETURN(ARM64Context_processor(assignment->expression(), ctx));

    switch (assignment->type()->type()) {
    case PrimitiveType::Pointer:
    case PrimitiveType::Int:
        ctx.assembly().add_instruction("str", "x0,[fp,#{}]", assignee->offset());
        break;
    case PrimitiveType::Unsigned:
        ctx.assembly().add_instruction("str", "x0,[fp,#{}]", assignee->offset());
        break;
    case PrimitiveType::Byte:
        ctx.assembly().add_instruction("strbs", "x0,[fp,#{}]", assignee->offset());
        break;
    case PrimitiveType::Char:
    case PrimitiveType::Boolean:
        ctx.assembly().add_instruction("strb", "x0,[fp,#{}]", assignee->offset());
        break;
    case PrimitiveType::String: {
        ctx.assembly().add_instruction("str", "x0,[fp,#{}]", assignee->offset());
        ctx.assembly().add_instruction("str", "w1,[fp,#{}]", assignee->offset( )+ 8);
        break;
    }
    default:
        return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()->to_string()) };
    }
    ctx.release_target_register(assignment->type()->type());
    return tree;
});

NODE_PROCESSOR(MaterializedVariableDecl)
{
    auto var_decl = std::dynamic_pointer_cast<MaterializedVariableDecl>(tree);
    ctx.assembly().add_comment(var_decl->to_string());
    if (var_decl->type()->type() == PrimitiveType::Struct)
        return tree;

    ctx.initialize_target_register();
    if (var_decl->expression() != nullptr) {
        TRY_RETURN(ARM64Context_processor(var_decl->expression(), ctx));
    } else {
        switch (var_decl->type()->type()) {
        case PrimitiveType::String: {
            ctx.assembly().add_instruction("mov", "w1,wzr");
        } // fall through
        case PrimitiveType::Pointer:
        case PrimitiveType::Int:
        case PrimitiveType::Unsigned:
        case PrimitiveType::Byte:
        case PrimitiveType::Char:
        case PrimitiveType::Boolean:
            ctx.assembly().add_instruction("mov", "x0,xzr");
            break;
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot initialize variables of type {} yet", var_decl->expression()->type()->to_string()) };
        }
    }
    ctx.assembly().add_instruction("str", "x0,[fp,#{}]", var_decl->offset());
    if (var_decl->type()->type() == PrimitiveType::String) {
        ctx.assembly().add_instruction("str", "x1,[fp,#{}]", var_decl->offset() + 8);
    }
    ctx.release_target_register(var_decl->type()->type());
    return tree;
});

NODE_PROCESSOR(BoundExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
    debug(parser, "{}", expr_stmt->to_string());
    ctx.assembly().add_comment(expr_stmt->to_string());
    ctx.initialize_target_register();
    TRY_RETURN(ARM64Context_processor(expr_stmt->expression(), ctx));
    ctx.release_target_register();
    return tree;
});

NODE_PROCESSOR(BoundReturn)
{
    auto ret = std::dynamic_pointer_cast<BoundReturn>(tree);
    debug(parser, "{}", ret->to_string());
    ctx.assembly().add_comment(ret->to_string());
    ctx.initialize_target_register();
    TRY_RETURN(ARM64Context_processor(ret->expression(), ctx));
    ctx.release_target_register();
    ctx.function_return();

    return tree;
});

NODE_PROCESSOR(Label)
{
    auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
    debug(parser, "{}", label->to_string());
    ctx.assembly().add_comment(label->to_string());
    ctx.assembly().add_label(format("lbl_{}", label->label_id()));
    return tree;
});

NODE_PROCESSOR(Goto)
{
    auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
    debug(parser, "{}", goto_stmt->to_string());
    ctx.assembly().add_comment(goto_stmt->to_string());
    ctx.assembly().add_instruction("b", "lbl_{}", goto_stmt->label_id());
    return tree;
});

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
            ctx.initialize_target_register();
            auto cond = TRY_AND_CAST(Expression, ARM64Context_processor(branch->condition(), ctx));
            ctx.release_target_register();
            ctx.assembly().add_instruction("cmp", "w0,0x00");
            ctx.assembly().add_instruction("b.eq", "lbl_{}", else_label);
        } else {
            ctx.assembly().add_comment("else");
        }
        auto stmt = TRY_AND_CAST(Statement, ARM64Context_processor(branch->statement(), ctx));
        if (count) {
            ctx.assembly().add_instruction("b", "lbl_{}", end_label);
            ctx.assembly().add_label(format("lbl_{}", else_label));
        }
        count--;
    }
    ctx.assembly().add_label(format("lbl_{}", end_label));
    return tree;
});

ErrorOrNode output_arm64(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name, bool run)
{
    auto processed = TRY(materialize_arm64(tree));
    std::cout << "\n\nMaterialized:\n" << processed->to_xml() << "\n";

    ARM64Context root;
    auto ret = processor_for_context<ARM64Context>(processed, root);

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
                return exit_code_or_error.error();
            sdk_path = strip(p.standard_out());
        }

        std::vector<std::string> ld_args = { "-o", bare_file_name, "-loblrt", "-lSystem", "-syslibroot", sdk_path, "-e", "_start", "-arch", "arm64",
            format("-L{}/lib", obl_dir) };
        for (auto& m : modules)
            ld_args.push_back(m);

        if (auto code = execute("ld", ld_args); code.is_error())
            return code.error();
        if (run) {
            auto run_cmd = format("./{}", bare_file_name);
            auto exit_code = execute(run_cmd);
            if (exit_code.is_error())
                return exit_code.error();
            ret = make_node<BoundLiteral>(Token { TokenCode::Integer, format("{}", exit_code.value()) }, (long) exit_code.value());
        }
    }
    return ret;
}

}
