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
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <obelix/transpile/c/CTranspiler.h>
#include <obelix/transpile/c/CTranspilerIntrinsics.h>
#include <obelix/transpile/c/CTranspilerContext.h>

namespace fs = std::filesystem;

namespace Obelix {

logging_category(c_transpiler);

void type_to_c_type(CTranspilerContext &ctx, std::shared_ptr<ObjectType> const& type)
{
    std::string c_type;
    switch (type->type()) {
    case PrimitiveType::SignedIntegerNumber:
        c_type = format("int{}_t", 8*type->size());
        break;
    case PrimitiveType::IntegerNumber:
        c_type = format("uint{}_t", 8*type->size());
        break;
    default:
        fatal("Implement type_to_c_type for PrimitiveType {}", type->type());
    }
    ctx.write(c_type);
}

ErrorOr<void, SyntaxError> evaluate_arguments(CTranspilerContext& ctx, std::shared_ptr<BoundFunctionDecl> const& decl, BoundExpressions const& arguments)
{
    auto first { true };
    for (auto const& arg : arguments) {
        if (!first)
            ctx.write(", ");
        TRY(process(arg, ctx));
        first = false;
    }
    return {};
}

INIT_NODE_PROCESSOR(CTranspilerContext)

NODE_PROCESSOR(BoundCompilation)
{
    auto compilation = std::dynamic_pointer_cast<BoundCompilation>(tree);
    return process_tree(tree, ctx, CTranspilerContext_processor);
}

NODE_PROCESSOR(BoundModule)
{
    auto module = std::dynamic_pointer_cast<BoundModule>(tree);
    auto name = module->name();
    if (name.empty())
        return tree;

    if (name.starts_with("./"))
        name = name.substr(2);

    fs::path path { join(split(name, '/'), "-") };
    TRY_RETURN(ctx.open_output_file(path.replace_extension(fs::path("c"))));
    ctx.writeln(
R"(#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
)");
    TRY_RETURN(process_tree(module->block(), ctx, CTranspilerContext_processor));
    TRY_RETURN(ctx.flush());
    return tree;
}

NODE_PROCESSOR(BoundFunctionDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(tree);

    type_to_c_type(ctx, func_decl->type());
    ctx.write(format(" {}(", func_decl->name()));
    auto first { true };
    for (auto& param : func_decl->parameters()) {
        if (!first)
            ctx.write(", ");
        type_to_c_type(ctx, param->type());
        ctx.write(param->name());
        first = false;
    }
    ctx.writeln(")");
    return tree;
}

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    TRY_RETURN(process(func_def->declaration(), ctx));
    ctx.writeln("{");
    ctx.indent();
    TRY_RETURN(process(func_def->statement(), ctx));
    ctx.dedent();
    ctx.writeln("}");
    return tree;
}

NODE_PROCESSOR(BoundFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
    ctx.write(format("{}(", call->name()));
    TRY_RETURN(evaluate_arguments(ctx, call->declaration(), call->arguments()));
    ctx.write(")");
    return tree;
}

NODE_PROCESSOR(BoundNativeFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
    ctx.write(format("{}(", call->name()));
    TRY_RETURN(evaluate_arguments(ctx, call->declaration(), call->arguments()));
    ctx.write(")");
    return tree;
}

NODE_PROCESSOR(MaterializedIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<MaterializedIntrinsicCall>(tree);

    ctx.writeln("({");
    ctx.indent();
    auto count { 0 };
    for (auto const& arg : call->arguments()) {
        type_to_c_type(ctx, arg->type());
        ctx.write(format(" arg{} = ", count++));
        TRY(process(arg, ctx));
        ctx.writeln(";");
    }
    CTranspilerFunctionType impl = get_c_transpiler_intrinsic(call->intrinsic());
    if (!impl)
        return SyntaxError { ErrorCode::InternalError, call->token(), format("No C Transpiler implementation for intrinsic {}", call->to_string()) };
    auto ret = impl(ctx);
    if (ret.is_error())
        return ret.error();
    return tree;
}

NODE_PROCESSOR(BoundCastExpression)
{
    auto cast = std::dynamic_pointer_cast<BoundCastExpression>(tree);
    return tree;
}

NODE_PROCESSOR(BoundIntLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(tree);
    ctx.write(format("{}", literal->int_value()));
    return tree;
}

NODE_PROCESSOR(BoundEnumValue)
{
    auto enum_value = std::dynamic_pointer_cast<BoundEnumValue>(tree);
    ctx.write(format("{}", enum_value->label()));
    return tree;
}

NODE_PROCESSOR(BoundStringLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundStringLiteral>(tree);
    return tree;
}

NODE_PROCESSOR(BoundVariable)
{
    auto variable = std::dynamic_pointer_cast<BoundVariable>(tree);
    ctx.write(variable->name());
    return tree;
}

NODE_PROCESSOR(BoundAssignment)
{
    auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
    auto assignee = std::dynamic_pointer_cast<BoundVariable>(assignment->assignee());
    if (assignee == nullptr)
        return SyntaxError { ErrorCode::InternalError, assignment->token(), format("Variable access '{}' not materialized", assignment->assignee()) };
    ctx.write(format("{} = ", assignee->name()));
    TRY_RETURN(process(assignment->expression(), ctx));
    ctx.writeln(";");
    return tree;
}

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);

    if (std::dynamic_pointer_cast<BoundStaticVariableDeclaration>(var_decl))
        ctx.write("static ");
    type_to_c_type(ctx, var_decl->type());
    ctx.write(format(" {}", var_decl->name()));
    if (var_decl->expression() != nullptr) {
        ctx.write(" = ");
        TRY_RETURN(process(var_decl->expression(), ctx));
    }
    ctx.writeln(";");
    return tree;
}

ALIAS_NODE_PROCESSOR(BoundStaticVariableDeclaration, BoundVariableDeclaration);

NODE_PROCESSOR(MaterializedGlobalVariableDecl)
{
    return tree;
}

ALIAS_NODE_PROCESSOR(MaterializedLocalVariableDecl, MaterializedGlobalVariableDecl)

NODE_PROCESSOR(BoundExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
    TRY_RETURN(process(expr_stmt->expression(), ctx));
    ctx.writeln(";");
    return tree;
}

NODE_PROCESSOR(BoundReturn)
{
    auto ret = std::dynamic_pointer_cast<BoundReturn>(tree);
    ctx.write("return");
    if (ret->expression())
        ctx.write(" ");
        TRY_RETURN(process(ret->expression(), ctx));
    ctx.writeln(";");
    return tree;
}

NODE_PROCESSOR(BoundWhileStatement)
{
    auto while_stmt = std::dynamic_pointer_cast<BoundWhileStatement>(tree);

    ctx.write("while (");
    TRY_RETURN(process(while_stmt->condition(), ctx));
    ctx.writeln(") {");
    ctx.indent();
    TRY_RETURN(process(while_stmt->statement(), ctx));
    ctx.dedent();
    ctx.writeln("}");
    return tree;
}

NODE_PROCESSOR(BoundIfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);

    auto first { true };
    for (auto& branch : if_stmt->branches()) {
        if (!first)
            ctx.write("else ");
        if (branch->condition()) {
            ctx.write("if (");
            TRY_RETURN(process(branch->condition(), ctx));
            ctx.write(") ");
        }
        ctx.writeln(" {");
        ctx.indent();
        TRY_RETURN(process(branch->statement(), ctx));
        ctx.dedent();
        ctx.writeln("}");
        first = false;
    }
    return tree;
}

ErrorOrNode transpile_to_c(std::shared_ptr<SyntaxNode> const& tree, Config const& config, std::string const& file_name)
{
    CTranspilerContext root;
    auto ret = process(tree, root);

    if (ret.is_error()) {
        return ret;
    }

    fs::create_directory(".obelix");

#if 0
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
#endif

#if 0
    main->enter_function("static_initializer");
    for (auto& module_assembly : ARM64Context::assemblies()) {
        auto& module = module_assembly.first;
        auto& assembly = module_assembly.second;
        if (assembly->static_initializer().empty() || !assembly->has_exports())
            continue;
        main->add_instruction("bl", format("static_{}", module));
    }
    main->leave_function();
#endif

    std::string obl_dir = config.obelix_directory();

    std::vector<std::string> modules;
    for (auto& module_file : root.files()) {
        auto p = fs::path(".obelix/" + module_file->name());

        if (config.cmdline_flag("show-c-file")) {
            std::cout << module_file->to_string();
        }

        std::vector<std::string> clang_args = { p.string(), "-c", "-o", p.replace_extension("o"), format("-I{}/include", obl_dir) };
        for (auto& m : modules)
            clang_args.push_back(m);

        if (auto code = execute("clang", clang_args); code.is_error())
            return SyntaxError { code.error(), Token {} };

        if (!config.cmdline_flag("keep-c-file")) {
            auto c_file = p.replace_extension("c");
            unlink(c_file.c_str());
        }
        modules.push_back(p.replace_extension("o"));
    }

    if (!modules.empty()) {
        auto file_parts = split(file_name, '/');
        auto p = fs::path { ".obelix/" + join(split(file_name, '/'), "-") };

        static std::string sdk_path; // "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk";
        if (sdk_path.empty()) {
            Process process("xcrun", "-sdk", "macosx", "--show-sdk-path");
            if (auto exit_code_or_error = process.execute(); exit_code_or_error.is_error())
                return SyntaxError { exit_code_or_error.error(), Token {} };
            sdk_path = strip(process.standard_out());
        }

        auto p_here = fs::path { join(split(file_name, '/'), "-") };
        std::vector<std::string> ld_args = { "-o", p_here.replace_extension(""), "-loblrt", "-lSystem", "-syslibroot", sdk_path,
            format("-L{}/lib", obl_dir) };
        for (auto& m : modules)
            ld_args.push_back(m);

        if (auto code = execute("ld", ld_args); code.is_error())
            return SyntaxError { code.error(), Token {} };
        if (config.run) {
            auto run_cmd = format("./{}", p.string());
            auto exit_code = execute(run_cmd);
            if (exit_code.is_error())
                return SyntaxError { exit_code.error(), Token {} };
            ret = make_node<BoundIntLiteral>(Token { TokenCode::Integer, format("{}", exit_code.value()) }, (long) exit_code.value());
        }
    }
    return ret;
}

}