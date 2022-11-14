/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <unistd.h>
#include <filesystem>
#include <memory>

#include <core/Error.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <core/ScopeGuard.h>
#include <obelix/Processor.h>
#include <obelix/arm64/ARM64.h>
#include <obelix/arm64/ARM64Context.h>
#include <obelix/arm64/ARM64Intrinsics.h>
#include <obelix/arm64/MaterializedSyntaxNode.h>
#include <obelix/arm64/Mnemonic.h>

namespace Obelix {

logging_category(arm64);

INIT_NODE_PROCESSOR(ARM64Context)

NODE_PROCESSOR(BoundCompilation)
{
    auto compilation = std::dynamic_pointer_cast<BoundCompilation>(tree);
    ctx.add_module(ARM64Context::ROOT_MODULE_NAME);
    return process_tree(tree, ctx, result, ARM64Context_processor);
}

NODE_PROCESSOR(BoundModule)
{
    auto module = std::dynamic_pointer_cast<BoundModule>(tree);
    auto name = module->name();
    if (name.starts_with("./"))
        name = name.substr(2);
    ctx.add_module(join(split(name, '/'), "-"));
    TRY_RETURN(process_tree(module->block(), ctx, result, ARM64Context_processor));
    return tree;
}

NODE_PROCESSOR(MaterializedFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<MaterializedFunctionDef>(tree);

    for (auto& param : func_def->declaration()->parameters()) {
        auto address = std::dynamic_pointer_cast<StackVariableAddress>(param->address());
        ctx.declare(param->name(), address->offset());
    }

    if (func_def->declaration()->node_type() == SyntaxNodeType::MaterializedFunctionDecl) {
        TRY_RETURN(ctx.enter_function(func_def));
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
        ctx.assembly()->add_instruction("mov", "x10,sp");
        ctx.assembly()->add_instruction("sub", "sp,sp,#{}", nsaa);
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
                            ctx.assembly()->add_instruction("str", "x0,[x10,#-{}]", param_defs[param_ix]->where());
                            break;
                        case PrimitiveType::Struct:
                        default:
                            ctx.assembly()->add_text(
                                R"(

                            )");
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
        ctx.assembly()->add_instruction("mov", "sp,x10");
        pop(ctx, "x10");
    }
}

NODE_PROCESSOR(MaterializedFunctionCall)
{
    auto call = std::dynamic_pointer_cast<MaterializedFunctionCall>(tree);
    TRY_RETURN(evaluate_arguments(ctx, call->declaration(), call->arguments()));
    ctx.assembly()->add_instruction("bl", call->declaration()->label());
    reset_sp_after_call(ctx, call->declaration());
    return tree;
}

NODE_PROCESSOR(MaterializedNativeFunctionCall)
{
    auto native_func_call = std::dynamic_pointer_cast<MaterializedNativeFunctionCall>(tree);
    auto func_decl = std::dynamic_pointer_cast<MaterializedNativeFunctionDecl>(native_func_call->declaration());
    TRY_RETURN(evaluate_arguments(ctx, func_decl, native_func_call->arguments()));
    ctx.assembly()->add_instruction("bl", func_decl->native_function_name());
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

NODE_PROCESSOR(BoundCastExpression)
{
    auto cast = std::dynamic_pointer_cast<BoundCastExpression>(tree);
    auto expr = TRY_AND_CAST(BoundExpression, cast->expression(), ctx);
    assert(expr->type()->can_cast_to(cast->type()) != CanCast::Never);

    auto from_pt = expr->type()->type();
    auto to_pt = cast->type()->type();
    switch (to_pt) {
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::SignedIntegerNumber:
    case PrimitiveType::Boolean: {
        switch (from_pt) {
        case PrimitiveType::IntegerNumber:
        case PrimitiveType::SignedIntegerNumber:
        case PrimitiveType::Pointer:
        case PrimitiveType::Enum:
        case PrimitiveType::Boolean: {
            if (expr->type()->can_cast_to(cast->type()) == CanCast::Sometimes) {
                // Dynamically check that value can be casted
            }
            if (cast->type()->size() < 8) {
                std::string mask = "#0x";
                for (auto i = 0; i < cast->type()->size(); ++i)
                    mask += "ff";
                ctx.assembly()->add_instruction("and", "x0,x0,{}", mask);
            }
            return tree;
        }
        default:
            break;
        }
    }
    case PrimitiveType::Pointer:
        if (from_pt == PrimitiveType::Pointer || ((from_pt == PrimitiveType::IntegerNumber) && (expr->type()->size() == cast->type()->size())))
            return tree;
        break;
    default:
        break;
    }
    fatal("Can't cast from {} to {} (yet)", expr->type(), cast->type());
}

NODE_PROCESSOR(BoundIntLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(tree);
    TRY_RETURN(ctx.load_immediate(literal->type(), literal->int_value(), 0));
    return tree;
}

NODE_PROCESSOR(BoundEnumValue)
{
    auto enum_value = std::dynamic_pointer_cast<BoundEnumValue>(tree);
    TRY_RETURN(ctx.load_immediate(enum_value->type(), enum_value->value(), 0));
    return tree;
}

NODE_PROCESSOR(BoundStringLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundStringLiteral>(tree);
    auto str_id = ctx.assembly()->add_string(literal->value());
    TRY_RETURN(ctx.load_immediate(literal->type()->field("size").type, literal->value().length(), 0));
    ctx.assembly()->add_instruction("adr", "x1,str_{}", str_id);
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
    TRY_RETURN(process(array_access->index(), ctx));
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
        TRY_RETURN(process(array_access->index(), ctx));
    }
    TRY_RETURN(assignee->address()->store_variable(assignment->type(), ctx, 0));
    return tree;
}

NODE_PROCESSOR(MaterializedVariableDecl)
{
    auto var_decl = std::dynamic_pointer_cast<MaterializedVariableDecl>(tree);
    ctx.assembly()->add_comment(var_decl->to_string());

    ctx.assembly()->add_comment("Initializing variable");
    if (var_decl->expression() != nullptr) {
        TRY_RETURN(process(var_decl->expression(), ctx));
        auto error_maybe = ctx.store_variable(var_decl->type(), var_decl->offset(), 0);
        if (error_maybe.is_error()) {
            return error_maybe.error();
        }
    } else {
        TRY_RETURN(ctx.zero_initialize(var_decl->type(), var_decl->offset()));
    }
    return tree;
}

NODE_PROCESSOR(MaterializedStaticVariableDecl)
{
    auto var_decl = std::dynamic_pointer_cast<MaterializedStaticVariableDecl>(tree);
    ctx.assembly()->add_comment(var_decl->to_string());
    TRY_RETURN(ctx.define_static_storage(var_decl->label(), var_decl->type(), false, var_decl->expression()));

    ctx.assembly()->add_comment("Initializing variable");
    if (var_decl->expression() != nullptr) {
        auto skip_label = Obelix::Label::reserve_id();
        ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", var_decl->label());
        ctx.assembly()->add_instruction("ldr", "w0,[x8,{}@PAGEOFF+{}]", var_decl->label(), var_decl->type()->size());
        ctx.assembly()->add_instruction("cmp", "w0,0x00");
        ctx.assembly()->add_instruction("b.ne", "lbl_{}", skip_label);
        TRY_RETURN(process(var_decl->expression(), ctx));
        if (var_decl->type()->type() != PrimitiveType::Struct) {
            auto mm = get_type_mnemonic_map(var_decl->type());
            if (mm == nullptr)
                return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                    format("Cannot store values of type {} yet", var_decl->type()) };

            ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", var_decl->label());
            ctx.assembly()->add_instruction(mm->store_mnemonic, "{}0,[x8,{}@PAGEOFF]", mm->reg_width, var_decl->label());
        } else {
            ctx.assembly()->add_comment("Storing static struct variable");
            ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", var_decl->label());
            auto from = 0u;
            for (auto const& field : var_decl->type()->fields()) {
                auto reg_width = "w";
                if (field.type->size() > 4)
                    reg_width = "x";
                ctx.assembly()->add_instruction("str", format("{}{},[x8,{}@PAGEOFF+{}]", reg_width, from++, var_decl->label(), var_decl->type()->offset_of(field.name)));
            }
        }
        ctx.assembly()->add_instruction("mov", "w0,1");
        ctx.assembly()->add_instruction("str", "w0,[x8,{}@PAGEOFF+{}]", var_decl->label(), var_decl->type()->size());
        ctx.assembly()->add_label(format("lbl_{}", skip_label));
    }
    return tree;
}

NODE_PROCESSOR(MaterializedGlobalVariableDecl)
{
    auto var_decl = std::dynamic_pointer_cast<MaterializedStaticVariableDecl>(tree);
    TRY_RETURN(ctx.define_static_storage(var_decl->label(), var_decl->type(),
        var_decl->node_type() == SyntaxNodeType::MaterializedGlobalVariableDecl, var_decl->expression()));
    ctx.assembly()->target_static();
    ctx.assembly()->add_comment(var_decl->to_string());
    ScopeGuard sg([&ctx]() { ctx.assembly()->target_code(); });
    ctx.assembly()->add_comment("Initializing variable");
    if (var_decl->expression() != nullptr) {
        TRY_RETURN(process(var_decl->expression(), ctx));
        if (var_decl->type()->type() != PrimitiveType::Struct) {
            auto mm = get_type_mnemonic_map(var_decl->type());
            if (mm == nullptr)
                return SyntaxError { ErrorCode::NotYetImplemented, Token {},
                    format("Cannot store values of type {} yet", var_decl->type()) };

            ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", var_decl->label());
            ctx.assembly()->add_instruction(mm->store_mnemonic, "{}0,[x8,{}@PAGEOFF]", mm->reg_width, var_decl->label());
        } else {
            ctx.assembly()->add_comment("Storing static struct variable");
            ctx.assembly()->add_instruction("adrp", "x8,{}@PAGE", var_decl->label());
            auto from = 0u;
            for (auto const& field : var_decl->type()->fields()) {
                auto reg_width = "w";
                if (field.type->size() > 4)
                    reg_width = "x";
                ctx.assembly()->add_instruction("str", format("{}{},[x8,{}@PAGEOFF+{}]", reg_width, from++, var_decl->label(), var_decl->type()->offset_of(field.name)));
            }
        }
    }
    return tree;
}

ALIAS_NODE_PROCESSOR(MaterializedLocalVariableDecl, MaterializedGlobalVariableDecl)

NODE_PROCESSOR(BoundExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
    debug(parser, "{}", expr_stmt->to_string());
    ctx.assembly()->add_comment(expr_stmt->to_string());
    if (expr_stmt->expression()->type()->type() == PrimitiveType::Struct) {
        ctx.assembly()->add_instruction("sub", "sp,sp,#{}", expr_stmt->expression()->type()->size());
        ctx.assembly()->add_instruction("mov", "x8,sp");
    }
    TRY_RETURN(process(expr_stmt->expression(), ctx));
    if (expr_stmt->expression()->type()->type() == PrimitiveType::Struct) {
        ctx.assembly()->add_instruction("add", "sp,sp,#{}", expr_stmt->expression()->type()->size());
    }
    return tree;
}

NODE_PROCESSOR(BoundReturn)
{
    auto ret = std::dynamic_pointer_cast<BoundReturn>(tree);
    debug(parser, "{}", ret->to_string());
    ctx.assembly()->add_comment(ret->to_string());
    TRY_RETURN(process(ret->expression(), ctx));
    ctx.function_return();

    return tree;
}

NODE_PROCESSOR(Label)
{
    auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
    debug(parser, "{}", label->to_string());
    ctx.assembly()->add_comment(label->to_string());
    ctx.assembly()->add_label(format("lbl_{}", label->label_id()));
    return tree;
}

NODE_PROCESSOR(Goto)
{
    auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
    debug(parser, "{}", goto_stmt->to_string());
    ctx.assembly()->add_comment(goto_stmt->to_string());
    ctx.assembly()->add_instruction("b", "lbl_{}", goto_stmt->label_id());
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
            ctx.assembly()->add_comment(format("if ({})", branch->condition()->to_string()));
            auto cond = TRY_AND_CAST(BoundExpression, branch->condition(), ctx);
            ctx.assembly()->add_instruction("cmp", "w0,0x00");
            ctx.assembly()->add_instruction("b.eq", "lbl_{}", else_label);
        } else {
            ctx.assembly()->add_comment("else");
        }
        auto stmt = TRY_AND_CAST(Statement, branch->statement(), ctx);
        if (count) {
            ctx.assembly()->add_instruction("b", "lbl_{}", end_label);
            ctx.assembly()->add_label(format("lbl_{}", else_label));
        }
        count--;
    }
    ctx.assembly()->add_label(format("lbl_{}", end_label));
    return tree;
}

ProcessResult output_arm64(std::shared_ptr<SyntaxNode> const& tree, Config const& config)
{
    auto processed = TRY(materialize_arm64(tree));
    if (config.cmdline_flag<bool>("show-tree"))
        std::cout << "\n\nMaterialized:\n"
                  << std::dynamic_pointer_cast<BoundCompilation>(processed)->root_to_xml()
                  << "\n"
                  << processed->to_xml()
                  << "\n";
    if (!config.compile)
        return processed;

    ARM64Context root(config);
    auto ret = process(processed, root);

    if (ret.is_error()) {
        return ret;
    }

    namespace fs = std::filesystem;
    fs::create_directory(".obelix");

    std::shared_ptr<Assembly> main { nullptr };
    for (auto& module_assembly : ARM64Context::assemblies()) {
        auto& assembly = module_assembly.second;
        if (assembly->has_main()) {
            main = assembly;
        }
    }
    if (main == nullptr) {
        return SyntaxError { ErrorCode::FunctionUndefined, Token {}, "No main() function found" };
    }

    main->enter_function("static_initializer");
    for (auto& module_assembly : ARM64Context::assemblies()) {
        auto& module = module_assembly.first;
        auto& assembly = module_assembly.second;
        if (assembly->static_initializer().empty() || !assembly->has_exports())
            continue;
        main->add_instruction("bl", format("static_{}", module));
    }
    main->leave_function();

    std::vector<std::string> modules;
    for (auto& module_assembly : ARM64Context::assemblies()) {
        auto& module = module_assembly.first;
        auto& assembly = module_assembly.second;

        if (assembly->has_exports()) {
            auto file_name_parts = split(module, '.');
            auto bare_file_name = ".obelix/" + file_name_parts.front();

            if (config.cmdline_flag<bool>("show-assembly")) {
                std::cout << bare_file_name << ".s:"
                          << "\n";
                std::cout << assembly->to_string();
            }

            TRY_RETURN(assembly->save_and_assemble(bare_file_name));
            if (!config.cmdline_flag<bool>("keep-assembly")) {
                auto assembly_file = bare_file_name + ".s";
                unlink(assembly_file.c_str());
            }
            modules.push_back(bare_file_name + ".o");
        }
    }

    if (!modules.empty()) {
        std::string obl_dir = config.obelix_directory();

        static std::string sdk_path; // "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk";
        if (sdk_path.empty()) {
            Process p("xcrun", "-sdk", "macosx", "--show-sdk-path");
            if (auto exit_code_or_error = p.execute(); exit_code_or_error.is_error())
                return SyntaxError { exit_code_or_error.error(), Token {} };
            sdk_path = strip(p.standard_out());
        }

        std::vector<std::string> ld_args = { "-o", config.main(), "-loblrt", "-lSystem", "-syslibroot", sdk_path, "-e", "_start", "-arch", "arm64",
            format("-L{}/lib", obl_dir) };
        for (auto& m : modules)
            ld_args.push_back(m);

        if (auto code = execute("ld", ld_args); code.is_error())
            return SyntaxError { code.error(), Token {} };
        if (config.run) {
            auto run_cmd = format("./{}", config.main());
            auto exit_code = execute(run_cmd);
            if (exit_code.is_error())
                return SyntaxError { exit_code.error(), Token {} };
            ret = make_node<BoundIntLiteral>(Token { TokenCode::Integer, format("{}", exit_code.value()) }, (long) exit_code.value());
        }
    }
    return ret;
}

}
