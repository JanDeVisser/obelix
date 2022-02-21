/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <filesystem>

#include <config.h>
#include <core/Error.h>
#include <core/Logging.h>
#include <core/ScopeGuard.h>
#include <obelix/ARM64.h>
#include <obelix/ARM64Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

ErrorOrNode output_arm64_processor(std::shared_ptr<SyntaxNode> const& tree, ARM64Context& ctx)
{
    switch (tree->node_type()) {

    case SyntaxNodeType::Compilation: {
        auto compilation = std::dynamic_pointer_cast<Compilation>(tree);
        ctx.add_module(ARM64Context::ROOT_MODULE_NAME);
        return process_tree(tree, ctx, output_arm64_processor);
    }

    case SyntaxNodeType::Module: {
        auto module = std::dynamic_pointer_cast<Module>(tree);
        ctx.add_module(join(split(module->name(), '/'), "-"));
        return process_tree(tree, ctx, output_arm64_processor);
    }

    case SyntaxNodeType::MaterializedFunctionDef: {
        auto func_def = std::dynamic_pointer_cast<MaterializedFunctionDef>(tree);

        for (auto& param : func_def->declaration()->parameters()) {
            ctx.declare(param->identifier().identifier(), param->offset());
        }

        debug(parser, "func {}", func_def->name());
        if (func_def->declaration()->node_type() == SyntaxNodeType::MaterializedFunctionDecl) {
            ctx.enter_function(func_def);
            TRY_RETURN(output_arm64_processor(func_def->statement(), ctx));
            ctx.leave_function();
        }
        return tree;
    }

    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);

        // Load arguments in registers:
        ctx.new_enclosing_context();
        for (auto& argument : call->arguments()) {
            ctx.new_inherited_context();
            TRY_RETURN(output_arm64_processor(argument, ctx));
            ctx.release_register_context();
        }

        ctx.clear_context();
        ctx.reserve_register(0);
        if (call->type()->type_id() == ObelixType::TypeString)
            ctx.reserve_register(1);
        // Call function:
        ctx.assembly().add_instruction("bl", call->name());
        // Add x0 to the register context
        ctx.add_register();
        if (call->type()->type_id() == ObelixType::TypeString)
            ctx.add_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::NativeFunctionCall: {
        auto native_func_call = std::dynamic_pointer_cast<NativeFunctionCall>(tree);

        auto func_decl = native_func_call->declaration();

        ctx.new_enclosing_context();
        for (auto& arg : native_func_call->arguments()) {
            ctx.new_inherited_context();
            TRY_RETURN(output_arm64_processor(arg, ctx));
            ctx.release_register_context();
        }

        // Call the native function
        ctx.assembly().add_instruction("bl", func_decl->native_function_name());
        // Add x0 to the register context
        ctx.clear_context();
        ctx.add_register();
        if (native_func_call->type()->type_id() == ObelixType::TypeString)
            ctx.add_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::CompilerIntrinsic: {
        auto call = std::dynamic_pointer_cast<CompilerIntrinsic>(tree);
        ctx.new_enclosing_context();
        for (auto& arg : call->arguments()) {
            TRY_RETURN(output_arm64_processor(arg, ctx));
        }
        ARM64Implementation impl = Intrinsics::get_arm64_implementation(Signature { call->name(), call->type(), call->argument_types() });
        if (!impl)
            return Error { ErrorCode::InternalError, format("No ARM64 implementation for intrinsic {}", call->to_string()) };
        auto ret = impl(ctx);
        if (ret.is_error())
            return ret.error();
        ctx.clear_context();
        ctx.add_register();
        if (call->type()->type_id() == ObelixType::TypeString)
            ctx.add_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::Literal: {
        auto literal = std::dynamic_pointer_cast<Literal>(tree);
        auto val_maybe = TRY(literal->to_object());
        assert(val_maybe.has_value());
        auto val = val_maybe.value();
        switch (val.type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
        case ObelixType::TypeUnsigned: {
            ctx.assembly().add_instruction("mov", "x{},#{}", ctx.add_register(), val->to_long().value());
            break;
        }
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean: {
            ctx.assembly().add_instruction("mov", "w{},#{}", ctx.add_register(), static_cast<uint8_t>(val->to_long().value()));
            break;
        }
        case ObelixType::TypeString: {
            auto str_id = ctx.assembly().add_string(val->to_string());
            ctx.assembly().add_instruction("adr", "x{},str_{}", ctx.add_register(), str_id);
            ctx.assembly().add_instruction("mov", "w{},#{}", ctx.add_register(), val->to_string().length());
            break;
        }
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", ObelixType_name(val.type())));
        }
        return tree;
    }

    case SyntaxNodeType::Identifier: {
        auto identifier = std::dynamic_pointer_cast<Identifier>(tree);
        auto idx_maybe = ctx.get(identifier->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", identifier->name()) };
        auto idx = idx_maybe.value();

        switch (identifier->type()->type_id()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.add_register(), idx);
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("ldr", "x0,[fp,#{}]", ctx.add_register(), idx);
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("ldrbs", "w{},[fp,#{}]", ctx.add_register(), idx);
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", ctx.add_register(), idx);
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.add_register(), idx);
            auto reg = ctx.add_register();
            ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", reg, idx + 8);
            break;
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot push values of variables of type {} yet", identifier->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::Assignment: {
        auto assignment = std::dynamic_pointer_cast<Assignment>(tree);

        auto idx_maybe = ctx.get(assignment->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", assignment->name()) };
        auto idx = idx_maybe.value();

        TRY_RETURN(output_arm64_processor(assignment->expression(), ctx));

        switch (assignment->type()->type_id()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_register(), idx);
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_register(), idx);
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("strbs", "x{},[fp,#{}]", ctx.get_register(), idx);
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("strb", "x{},[fp,#{}]", ctx.get_register(), idx);
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_register(), idx);
            auto reg = ctx.get_register(1);
            ctx.assembly().add_instruction("strw", "w{},[fp,#{}]", reg, idx + 8);
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::MaterializedVariableDecl: {
        auto var_decl = std::dynamic_pointer_cast<MaterializedVariableDecl>(tree);
        debug(parser, "{}", var_decl->to_string(0));
        ctx.assembly().add_comment(var_decl->to_string(0));
        ctx.declare(var_decl->variable().identifier(), var_decl->offset());
        ctx.release_all();
        ctx.new_targeted_context();
        if (var_decl->expression() != nullptr) {
            TRY_RETURN(output_arm64_processor(var_decl->expression(), ctx));
        } else {
            auto reg = ctx.add_register();
            switch (var_decl->expression()->type()->type_id()) {
            case ObelixType::TypeString: {
                ctx.assembly().add_instruction("mov", "w{},wzr", ctx.add_register());
            } // fall through
            case ObelixType::TypePointer:
            case ObelixType::TypeInt:
            case ObelixType::TypeUnsigned:
            case ObelixType::TypeByte:
            case ObelixType::TypeChar:
            case ObelixType::TypeBoolean:
                ctx.assembly().add_instruction("mov", "x{},xzr", reg);
                break;
            default:
                return Error { ErrorCode::NotYetImplemented, format("Cannot initialize variables of type {} yet", var_decl->expression()->type()) };
            }
        }
        ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_register(0), var_decl->offset());
        if (var_decl->type()->type_id() == ObelixType::TypeString) {
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_register(1), var_decl->offset() + 8);
        }
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        debug(parser, "{}", expr_stmt->to_string(0));
        ctx.assembly().add_comment(expr_stmt->to_string(0));
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_arm64_processor(expr_stmt->expression(), ctx));
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::Return: {
        auto ret = std::dynamic_pointer_cast<Return>(tree);
        debug(parser, "{}", ret->to_string(0));
        ctx.assembly().add_comment(ret->to_string(0));
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_arm64_processor(ret->expression(), ctx));
        ctx.release_register_context();
        ctx.function_return();

        return tree;
    }

    case SyntaxNodeType::Label: {
        auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
        debug(parser, "{}", label->to_string(0));
        ctx.assembly().add_comment(label->to_string(0));
        ctx.assembly().add_label(format("lbl_{}", label->label_id()));
        return tree;
    }

    case SyntaxNodeType::Goto: {
        auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
        debug(parser, "{}", goto_stmt->to_string(0));
        ctx.assembly().add_comment(goto_stmt->to_string(0));
        ctx.assembly().add_instruction("b", "lbl_{}", goto_stmt->label_id());
        return tree;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        ctx.release_all();

        auto end_label = Obelix::Label::reserve_id();
        auto count = if_stmt->branches().size() - 1;
        for (auto& branch : if_stmt->branches()) {
            auto else_label = (count) ? Obelix::Label::reserve_id() : end_label;
            if (branch->condition()) {
                debug(parser, "if ({})", branch->condition()->to_string(0));
                ctx.assembly().add_comment(format("if ({})", branch->condition()->to_string(0)));
                ctx.new_targeted_context();
                auto cond = TRY_AND_CAST(Expression, output_arm64_processor(branch->condition(), ctx));
                ctx.assembly().add_instruction("cmp", "w{},0x00", ctx.get_register());
                ctx.assembly().add_instruction("b.eq", "lbl_{}", else_label);
            } else {
                debug(parser, "else");
                ctx.assembly().add_comment("else");
            }
            auto stmt = TRY_AND_CAST(Statement, output_arm64_processor(branch->statement(), ctx));
            if (count) {
                ctx.assembly().add_instruction("b", "lbl_{}", end_label);
                ctx.assembly().add_label(format("lbl_{}", else_label));
            }
            count--;
        }
        ctx.assembly().add_label(format("lbl_{}", end_label));
        return tree;
    }

    default:
        return process_tree(tree, ctx, output_arm64_processor);
    }
}

ErrorOrNode prepare_arm64_processor(std::shared_ptr<SyntaxNode> const& tree, Context<int>& ctx)
{
    if (tree == nullptr)
        return tree;

    switch (tree->node_type()) {
    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto func_decl = func_def->declaration();
        Context<int> func_ctx(ctx);
        int offset = 16;
        FunctionParameters function_parameters;
        for (auto& parameter : func_decl->parameters()) {
            function_parameters.push_back(std::make_shared<FunctionParameter>(parameter, offset));
            switch (parameter.type()->type_id()) {
            case ObelixType::TypeString:
                offset += 16;
                break;
            default:
                offset += 8;
            }
        }

        auto materialized_function_decl = std::make_shared<MaterializedFunctionDecl>(func_decl->identifier(), function_parameters);
        switch (func_decl->node_type()) {
        case SyntaxNodeType::NativeFunctionDecl: {
            auto native_decl = std::dynamic_pointer_cast<NativeFunctionDecl>(func_decl);
            materialized_function_decl = std::make_shared<MaterializedNativeFunctionDecl>(materialized_function_decl, native_decl->native_function_name());
            return std::make_shared<MaterializedFunctionDef>(materialized_function_decl, nullptr, 0);
        }
        case SyntaxNodeType::IntrinsicDecl: {
            auto intrinsic_decl = std::dynamic_pointer_cast<IntrinsicDecl>(func_decl);
            materialized_function_decl = std::make_shared<MaterializedIntrinsicDecl>(materialized_function_decl);
            return std::make_shared<MaterializedFunctionDef>(materialized_function_decl, nullptr, 0);
        }
        case SyntaxNodeType::FunctionDecl: {
            func_ctx.declare("#offset", offset);
            std::shared_ptr<Block> block;
            assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
            block = TRY_AND_CAST(FunctionBlock, prepare_arm64_processor(func_def->statement(), func_ctx));
            return std::make_shared<MaterializedFunctionDef>(materialized_function_decl, block, func_ctx.get("#offset").value());
        }
        default:
            fatal("Unreachable");
        }
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto offset = ctx.get("#offset").value();
        auto expression = TRY_AND_CAST(Expression, prepare_arm64_processor(var_decl->expression(), ctx));
        auto ret = std::make_shared<MaterializedVariableDecl>(var_decl->variable(), offset, expression, var_decl->is_const());
        switch (var_decl->type()->type_id()) {
        case ObelixType::TypeString:
            offset += 16;
            break;
        default:
            offset += 8;
        }
        ctx.set("#offset", offset);
        return ret;
    }

    case SyntaxNodeType::FunctionCall: {
        auto call = std::dynamic_pointer_cast<FunctionCall>(tree);
        auto arguments = TRY(xform_expressions(call->arguments(), ctx, prepare_arm64_processor));
        call = make_node<FunctionCall>(call->function(), arguments);
        if (Intrinsics::is_intrinsic(call))
            return std::make_shared<CompilerIntrinsic>(call);
        return call;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, prepare_arm64_processor(expr->operand(), ctx));
        auto call = std::make_shared<FunctionCall>(Symbol { expr->op().code_name(), expr->type() }, Expressions { operand });
        if (!Intrinsics::is_intrinsic(Signature { expr->op().code_name(), expr->type(), { operand->type() } }))
            return Error { ErrorCode::InternalError, format("No intrinsic defined for {}", call->to_string()) };
        return std::make_shared<CompilerIntrinsic>(call);
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        auto lhs = TRY_AND_CAST(Expression, prepare_arm64_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(Expression, prepare_arm64_processor(expr->rhs(), ctx));
        auto call = std::make_shared<FunctionCall>(Symbol { TokenCode_to_string(expr->op().code()), expr->type() }, Expressions { lhs, rhs });
        if (!Intrinsics::is_intrinsic(Signature { TokenCode_to_string(expr->op().code()), expr->type(), { lhs->type(), rhs->type() } }))
            return Error { ErrorCode::InternalError, format("No intrinsic defined for {}", call->to_string()) };
        return std::make_shared<CompilerIntrinsic>(call);
    }

    default:
        return process_tree(tree, ctx, prepare_arm64_processor);
    }
}

ErrorOrNode prepare_arm64(std::shared_ptr<SyntaxNode> const& tree)
{
    Context<int> root;
    return prepare_arm64_processor(tree, root);
}

ErrorOrNode output_arm64(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name)
{
    auto processed = TRY(prepare_arm64(tree));

    ARM64Context root;

    auto ret = output_arm64_processor(processed, root);

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

        auto objects = join(modules, " ");
        auto ld_cmd = format("ld -o {} {} -loblrt -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64 -L{}/lib",
            bare_file_name, objects, obl_dir);

        std::cout << "[CMD] " << ld_cmd << "\n";
        if (auto code = system(ld_cmd.c_str()))
            return Error { ErrorCode::ExecutionError, ld_cmd, code };
    }
    return ret;
}

}
