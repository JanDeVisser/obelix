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
#include <obelix/ARM64.h>
#include <obelix/ARM64Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/MaterializedSyntaxNode.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

logging_category(arm64);

ErrorOrNode output_arm64_processor(std::shared_ptr<SyntaxNode> const& tree, ARM64Context& ctx)
{
    if (tree == nullptr)
        return tree;

    debug(parser, "output_arm64_processor({} = {})", tree->node_type(), tree);
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
            ctx.declare(param->name(), param->offset());
        }

        debug(parser, "func {}", func_def->name());
        if (func_def->declaration()->node_type() == SyntaxNodeType::MaterializedFunctionDecl) {
            ctx.enter_function(func_def);
            TRY_RETURN(output_arm64_processor(func_def->statement(), ctx));
            ctx.leave_function();
        }
        return tree;
    }

    case SyntaxNodeType::BoundFunctionCall: {
        auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);

        // Load arguments in registers:
        ctx.new_enclosing_context();
        for (auto& argument : call->arguments()) {
            ctx.new_inherited_context();
            TRY_RETURN(output_arm64_processor(argument, ctx));
            ctx.release_register_context();
        }

        ctx.clear_context();
        ctx.reserve_register(0);
        if (call->type()->type() == ObelixType::TypeString)
            ctx.reserve_register(1);
        // Call function:
        ctx.assembly().add_instruction("bl", call->name());
        // Add x0 to the register context
        ctx.add_register();
        if (call->type()->type() == ObelixType::TypeString)
            ctx.add_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::BoundNativeFunctionCall: {
        auto native_func_call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
        auto func_decl = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(native_func_call->declaration());

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
        if (native_func_call->type()->type() == ObelixType::TypeString)
            ctx.add_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::BoundIntrinsicCall: {
        auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
        ctx.new_enclosing_context();
        for (auto& arg : call->arguments()) {
            TRY_RETURN(output_arm64_processor(arg, ctx));
        }
        auto decl = call->declaration();
        ARM64Implementation impl = Intrinsics::get_arm64_implementation(decl);
        if (!impl)
            return Error { ErrorCode::InternalError, format("No ARM64 implementation for intrinsic {}", call->to_string()) };
        auto ret = impl(ctx);
        if (ret.is_error())
            return ret.error();
        ctx.clear_context();
        ctx.add_register();
        if (call->type()->type() == ObelixType::TypeString)
            ctx.add_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::BoundLiteral: {
        auto literal = std::dynamic_pointer_cast<BoundLiteral>(tree);
        auto obj = literal->literal();
        switch (obj.type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
        case ObelixType::TypeUnsigned: {
            ctx.assembly().add_instruction("mov", "x{},#{}", ctx.add_register(), obj->to_long().value());
            break;
        }
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean: {
            ctx.assembly().add_instruction("mov", "w{},#{}", ctx.add_register(), static_cast<uint8_t>(obj->to_long().value()));
            break;
        }
        case ObelixType::TypeString: {
            auto str_id = ctx.assembly().add_string(obj->to_string());
            ctx.assembly().add_instruction("adr", "x{},str_{}", ctx.add_register(), str_id);
            ctx.assembly().add_instruction("mov", "w{},#{}", ctx.add_register(), obj->to_string().length());
            break;
        }
        default:
            return Error(ErrorCode::NotYetImplemented, format("Cannot emit literals of type {} yet", ObelixType_name(obj.type())));
        }
        return tree;
    }

    case SyntaxNodeType::BoundIdentifier: {
        auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(tree);
        auto idx_maybe = ctx.get(identifier->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", identifier->name()) };
        auto idx = idx_maybe.value();

        switch (identifier->type()->type()) {
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

    case SyntaxNodeType::BoundAssignment: {
        auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);

        auto idx_maybe = ctx.get(assignment->name());
        if (!idx_maybe.has_value())
            return Error { ErrorCode::InternalError, format("Undeclared variable '{}' during code generation", assignment->name()) };
        auto idx = idx_maybe.value();

        TRY_RETURN(output_arm64_processor(assignment->expression(), ctx));

        switch (assignment->type()->type()) {
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
            ctx.assembly().add_instruction("str", "w{},[fp,#{}]", reg, idx + 8);
            break;
        }
        default:
            return Error { ErrorCode::NotYetImplemented, format("Cannot emit assignments of type {} yet", assignment->type()) };
        }
        return tree;
    }

    case SyntaxNodeType::MaterializedVariableDecl: {
        auto var_decl = std::dynamic_pointer_cast<MaterializedVariableDecl>(tree);
        debug(parser, "{}", var_decl->to_string());
        ctx.assembly().add_comment(var_decl->to_string());
        ctx.declare(var_decl->name(), var_decl->offset());
        ctx.release_all();
        ctx.new_targeted_context();
        if (var_decl->expression() != nullptr) {
            TRY_RETURN(output_arm64_processor(var_decl->expression(), ctx));
        } else {
            auto reg = ctx.add_register();
            switch (var_decl->type()->type()) {
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
        if (var_decl->type()->type() == ObelixType::TypeString) {
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_register(1), var_decl->offset() + 8);
        }
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::BoundExpressionStatement: {
        auto expr_stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
        debug(parser, "{}", expr_stmt->to_string());
        ctx.assembly().add_comment(expr_stmt->to_string());
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_arm64_processor(expr_stmt->expression(), ctx));
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::BoundReturn: {
        auto ret = std::dynamic_pointer_cast<BoundReturn>(tree);
        debug(parser, "{}", ret->to_string());
        ctx.assembly().add_comment(ret->to_string());
        ctx.release_all();
        ctx.new_targeted_context();
        TRY_RETURN(output_arm64_processor(ret->expression(), ctx));
        ctx.release_register_context();
        ctx.function_return();

        return tree;
    }

    case SyntaxNodeType::Label: {
        auto label = std::dynamic_pointer_cast<Obelix::Label>(tree);
        debug(parser, "{}", label->to_string());
        ctx.assembly().add_comment(label->to_string());
        ctx.assembly().add_label(format("lbl_{}", label->label_id()));
        return tree;
    }

    case SyntaxNodeType::Goto: {
        auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
        debug(parser, "{}", goto_stmt->to_string());
        ctx.assembly().add_comment(goto_stmt->to_string());
        ctx.assembly().add_instruction("b", "lbl_{}", goto_stmt->label_id());
        return tree;
    }

    case SyntaxNodeType::BoundIfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);
        ctx.release_all();

        auto end_label = Obelix::Label::reserve_id();
        auto count = if_stmt->branches().size() - 1;
        for (auto& branch : if_stmt->branches()) {
            auto else_label = (count) ? Obelix::Label::reserve_id() : end_label;
            if (branch->condition()) {
                debug(parser, "if ({})", branch->condition()->to_string());
                ctx.assembly().add_comment(format("if ({})", branch->condition()->to_string()));
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

    debug(parser, "prepare_arm64_processor({} = {})", tree->node_type(), tree);
    switch (tree->node_type()) {
    case SyntaxNodeType::BoundFunctionDef: {
        auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
        auto func_decl = func_def->declaration();
        Context<int> func_ctx(ctx);
        int offset = 16;
        MaterializedFunctionParameters function_parameters;
        for (auto& parameter : func_decl->parameters()) {
            function_parameters.push_back(make_node<MaterializedFunctionParameter>(parameter, offset));
            switch (parameter->type()->type()) {
            case ObelixType::TypeString:
                offset += 16;
                break;
            default:
                offset += 8;
            }
        }

        auto materialized_function_decl = make_node<MaterializedFunctionDecl>(func_decl, function_parameters);
        switch (func_decl->node_type()) {
        case SyntaxNodeType::BoundNativeFunctionDecl: {
            auto native_decl = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(func_decl);
            materialized_function_decl = make_node<MaterializedNativeFunctionDecl>(native_decl);
            return make_node<MaterializedFunctionDef>(func_def, materialized_function_decl, nullptr, 0);
        }
        case SyntaxNodeType::BoundIntrinsicDecl: {
            auto intrinsic_decl = std::dynamic_pointer_cast<BoundIntrinsicDecl>(func_decl);
            materialized_function_decl = make_node<MaterializedIntrinsicDecl>(intrinsic_decl);
            return make_node<MaterializedFunctionDef>(func_def, materialized_function_decl, nullptr, 0);
        }
        case SyntaxNodeType::BoundFunctionDecl: {
            func_ctx.declare("#offset", offset);
            std::shared_ptr<Block> block;
            assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
            block = TRY_AND_CAST(FunctionBlock, prepare_arm64_processor(func_def->statement(), func_ctx));
            return make_node<MaterializedFunctionDef>(func_def, materialized_function_decl, block, func_ctx.get("#offset").value());
        }
        default:
            fatal("Unreachable");
        }
    }

    case SyntaxNodeType::BoundVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
        auto offset = ctx.get("#offset").value();
        auto expression = TRY_AND_CAST(BoundExpression, prepare_arm64_processor(var_decl->expression(), ctx));
        auto ret = make_node<MaterializedVariableDecl>(var_decl, offset, expression);
        switch (var_decl->type()->type()) {
        case ObelixType::TypeString:
            offset += 16;
            break;
        default:
            offset += 8;
        }
        ctx.set("#offset", offset);
        return ret;
    }

    case SyntaxNodeType::BoundFunctionCall: {
        auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
        auto xform_arguments = [&call, &ctx]() -> ErrorOr<BoundExpressions> {
            BoundExpressions ret;
            for (auto& expr : call->arguments()) {
                auto new_expr = prepare_arm64_processor(expr, ctx);
                if (new_expr.is_error())
                    return new_expr.error();
                ret.push_back(std::dynamic_pointer_cast<BoundExpression>(new_expr.value()));
            }
            return ret;
        };

        auto arguments = TRY(xform_arguments());
        return make_node<BoundFunctionCall>(call, arguments);
    }

    case SyntaxNodeType::BoundUnaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
        auto operand = TRY_AND_CAST(BoundExpression, prepare_arm64_processor(expr->operand(), ctx));

        std::shared_ptr<BoundFunctionDecl> declaration;
        switch (expr->op()) {
        case UnaryOperator::Dereference:
            declaration = Intrinsics::get_intrinsic(intrinsic_dereference).declaration;
            declaration = make_node<BoundFunctionDecl>(expr,
                std::make_shared<BoundIdentifier>(expr->token(), declaration->name(), expr->type()),
                declaration->parameters());
            break;
        default: {
            Signature s { UnaryOperator_name(expr->op()), expr->type(), { operand->type() } };
            if (!Intrinsics::is_intrinsic(s))
                return Error { ErrorCode::InternalError, format("No intrinsic defined for {}", s.name) };
            declaration = Intrinsics::get_intrinsic(s).declaration;
            break;
        }
        }
        return make_node<BoundIntrinsicCall>(expr->token(), declaration, BoundExpressions { operand });
    }

    case SyntaxNodeType::BoundBinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
        auto lhs = TRY_AND_CAST(BoundExpression, prepare_arm64_processor(expr->lhs(), ctx));
        auto rhs = TRY_AND_CAST(BoundExpression, prepare_arm64_processor(expr->rhs(), ctx));

        if (lhs->type()->type() == TypePointer && (expr->op() == BinaryOperator::Add || expr->op() == BinaryOperator::Subtract)) {
            std::shared_ptr<BoundExpression> offset { nullptr };
            auto target_type = (lhs->type()->is_template_instantiation()) ? lhs->type()->template_arguments()[0] : ObjectType::get(TypeChar);

            if (rhs->node_type() == SyntaxNodeType::BoundLiteral) {
                auto offset_literal = std::dynamic_pointer_cast<BoundLiteral>(rhs);
                auto offset_val = offset_literal->literal()->to_long().value();
                if (expr->op() == BinaryOperator::Subtract)
                    offset_val *= -1;
                offset = make_node<BoundLiteral>(rhs->token(), make_obj<Integer>(target_type->size() * offset_val));
            } else {
                offset = rhs;
                if (expr->op() == BinaryOperator::Subtract)
                    offset = make_node<BoundUnaryExpression>(expr->token(), offset, UnaryOperator::Negate, expr->type());
                auto size = make_node<BoundLiteral>(rhs->token(), make_obj<Integer>(target_type->size()));
                offset = make_node<BoundBinaryExpression>(expr->token(), size, BinaryOperator::Multiply, offset, ObjectType::get(TypeInt));
                offset = TRY_AND_CAST(BoundExpression, prepare_arm64_processor(offset, ctx));
            }
            auto intrinsic = Intrinsics::get_intrinsic(IntrinsicType::intrinsic_ptr_math);
            auto decl = make_node<BoundFunctionDecl>(expr,
                std::make_shared<BoundIdentifier>(expr->token(), intrinsic.declaration->name(), expr->type()),
                intrinsic.declaration->parameters());
            return make_node<BoundIntrinsicCall>(expr->token(), decl, BoundExpressions { lhs, offset });
        }

        Signature s { BinaryOperator_name(expr->op()), expr->type(), { lhs->type(), rhs->type() } };
        if (!Intrinsics::is_intrinsic(s))
            return Error { ErrorCode::InternalError, format("No intrinsic defined for {}", s.name) };
        auto intrinsic = Intrinsics::get_intrinsic(s);
        return make_node<BoundIntrinsicCall>(expr->token(), intrinsic.declaration, BoundExpressions { lhs, rhs });
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

ErrorOrNode output_arm64(std::shared_ptr<SyntaxNode> const& tree, std::string const& file_name, bool run)
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
            std::cout << "[RUN] " << run_cmd << "\n";
            auto exit_code = execute(run_cmd);
            if (exit_code.is_error())
                return exit_code.error();
            ret = make_node<BoundLiteral>(Token { TokenCode::Integer, format("{}", exit_code.value()) });
        }
    }
    return ret;
}

}
