/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
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

static std::string obl_dir;

std::string type_to_c_type(std::shared_ptr<ObjectType> const& type)
{
    std::string c_type;
    switch (type->type()) {
    case PrimitiveType::SignedIntegerNumber:
        c_type = format("int{}_t", 8*type->size());
        break;
    case PrimitiveType::IntegerNumber:
        c_type = format("uint{}_t", 8*type->size());
        break;
    case PrimitiveType::Boolean:
        c_type = "bool";
        break;
    case PrimitiveType::Pointer: {
        std::string ref_type = "void";
        if (type->is_template_specialization()) {
            auto target_type = type->template_argument<std::shared_ptr<ObjectType>>("target");
            ref_type = type_to_c_type(target_type);
        }
        c_type = format("{}*", ref_type);
        break;
    }
    case PrimitiveType::Array: {
        assert(type->is_template_specialization());
        auto base_type = type->template_argument<std::shared_ptr<ObjectType>>("base_type");
        c_type = type_to_c_type(base_type);
        break;
    }
    case PrimitiveType::Struct:
    case PrimitiveType::Enum:
        c_type = type->name();
        break;
    default:
        fatal("Can't convert {} types to C types yet", type->type());
    }
    return c_type;
}

void type_to_c_type(CTranspilerContext &ctx, std::shared_ptr<ObjectType> const& type)
{
    return ctx.write(type_to_c_type(type));
}

std::string type_initialize(std::shared_ptr<ObjectType> const& type)
{
    std::string initial_value;
    switch (type->type()) {
    case PrimitiveType::SignedIntegerNumber:
        initial_value = "0";
        break;
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::Enum:
        initial_value = "0u";
        break;
    case PrimitiveType::Boolean:
        initial_value = "false";
        break;
    case PrimitiveType::Pointer: {
        initial_value = "NULL";
        break;
    }
    case PrimitiveType::Struct:
    case PrimitiveType::Array:
        initial_value = "{ 0 }";
        break;
    default:
        fatal("Can't initialize {} variables in C yet", type->type());
    }
    return initial_value;
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
R"(#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <crt.h>
)");
    static std::unordered_set<std::string> runtime_symbols;
    if (runtime_symbols.empty()) {
        runtime_symbols.insert("cputln");
        runtime_symbols.insert("cputs");
        runtime_symbols.insert("putint");
        runtime_symbols.insert("putln");
        runtime_symbols.insert("puts");
        runtime_symbols.insert("string");
    }

    std::unordered_set<std::string> struct_types;
    std::unordered_set<std::string> enum_types;
    for (auto const& import : module->imports()) {
        for (auto const& param : import->parameters()) {
            if (param->type()->type() == PrimitiveType::Struct) {
                struct_types.insert(param->type()->name());
            }
            if (param->type()->type() == PrimitiveType::Enum) {
                enum_types.insert(param->type()->name());
            }
        }
    }

    auto num_structs = 0;
    for (auto const& type : struct_types) {
        if (runtime_symbols.contains(type))
            continue;
        num_structs++;
        auto t = ObjectType::get(type);
        ctx.writeln(format("typedef struct _{} {", type));
        ctx.indent();
        for (auto const& f : t->fields()) {
            type_to_c_type(ctx, f.type);
            ctx.writeln(format(" {};", f.name));
        }
        ctx.dedent();
        ctx.writeln(format("} {};\n", type));
    }
    if (num_structs > 0)
        ctx.writeln("");

    auto num_enums = 0;
    for (auto const& type : enum_types) {
        if (runtime_symbols.contains(type))
            continue;
        num_enums++;
        auto t = ObjectType::get(type);
        ctx.writeln(format("typedef enum _{} {", type));
        ctx.indent();
        for (auto const& v : t->template_argument_values<NVP>("values")) {
            ctx.writeln(format(" {};", v.first));
        }
        ctx.dedent();
        ctx.writeln(format("} {};\n", type));
    }
    if (num_enums > 0)
        ctx.writeln("");

    auto num_imports = 0;
    for (auto const& import : module->imports()) {
        auto import_name = import->name();
        if (auto native = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(import); native != nullptr)
            import_name = native->native_function_name();
        if (runtime_symbols.contains(import_name))
            continue;
        num_imports++;
        ctx.write("extern ");
        type_to_c_type(ctx, import->type());
        ctx.write(format(" {}(", import_name));
        auto first { true };
        for (auto const& param : import->parameters()) {
            if (!first)
                ctx.write(", ");
            type_to_c_type(ctx, param->type());
            first = false;
        }
        ctx.writeln(");");
    }
    if (num_imports > 0)
        ctx.writeln("");

    TRY_RETURN(process_tree(module->block(), ctx, CTranspilerContext_processor));
    TRY_RETURN(ctx.flush());
    return tree;
}

NODE_PROCESSOR(BoundStructDefinition)
{
    auto struct_def = std::dynamic_pointer_cast<BoundStructDefinition>(tree);
    ctx.writeln(format("typedef struct _{} {", struct_def->name()));
    ctx.indent();
    for (auto const& f : struct_def->type()->fields()) {
        type_to_c_type(ctx, f.type);
        ctx.writeln(format(" {};", f.name));
    }
    ctx.dedent();
    ctx.writeln(format("} {};\n", struct_def->name()));
    return tree;
}

NODE_PROCESSOR(BoundEnumDef)
{
    auto enum_def = std::dynamic_pointer_cast<BoundEnumDef>(tree);
    if (enum_def->extend())
        return tree;

    ctx.writeln(format("typedef enum _{} {",  enum_def->type()->name()));
    ctx.indent();
    for (auto const& [label, value] : enum_def->type()->template_argument_values<NVP>("values")) {
        ctx.writeln(format(" {} = {},", label, value));
    }
    ctx.dedent();
    ctx.writeln(format("} {};\n", enum_def->type()->name()));
    return tree;
}

NODE_PROCESSOR(BoundEnumValueDef)
{
    auto enum_value = std::dynamic_pointer_cast<BoundEnumValueDef>(tree);
    ctx.writeln(format("{} = {},", enum_value->label(), enum_value->value()));
    return tree;
}

NODE_PROCESSOR(BoundFunctionDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(tree);

    type_to_c_type(ctx, func_decl->type());
    auto name = func_decl->name();
    if (name == "main")
        name = "obelix_main";
    ctx.write(format(" {}(", name));
    auto first { true };
    for (auto& param : func_decl->parameters()) {
        if (!first)
            ctx.write(", ");
        ctx.write(format("{} {}", type_to_c_type(param->type()), param->name()));
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
    ctx.writeln("}\n");
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
    ctx.write(format("{}(", std::dynamic_pointer_cast<BoundNativeFunctionDecl>(call->declaration())->native_function_name()));
    TRY_RETURN(evaluate_arguments(ctx, call->declaration(), call->arguments()));
    ctx.write(")");
    return tree;
}

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);

    ctx.writeln("({");
    ctx.indent();
    auto count { 0 };
    for (auto const& arg : call->arguments()) {
        type_to_c_type(ctx, arg->type());
        ctx.write(format(" arg{} = ", count++));
        TRY_RETURN(process(arg, ctx));
        ctx.writeln(";");
    }
    CTranspilerFunctionType impl = get_c_transpiler_intrinsic(call->intrinsic());
    if (!impl)
        return SyntaxError { ErrorCode::InternalError, call->token(), format("No C Transpiler implementation for intrinsic {}", call->to_string()) };
    auto ret = impl(ctx);
    if (ret.is_error())
        return ret.error();
    ctx.writeln(";");
    ctx.dedent();
    ctx.write("})");
    return tree;
}

NODE_PROCESSOR(BoundCastExpression)
{
    auto cast = std::dynamic_pointer_cast<BoundCastExpression>(tree);
    ctx.write(format("({}) ", type_to_c_type(cast->type())));
    TRY_RETURN(process(cast->expression(), ctx));
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
    auto s = literal->value();
    ctx.write(format(R"((string) {{ {}, (uint8_t *) "{}" })", s.length(), s));
    return tree;
}

NODE_PROCESSOR(BoundVariable)
{
    auto variable = std::dynamic_pointer_cast<BoundVariable>(tree);
    ctx.write(variable->name());
    return tree;
}

NODE_PROCESSOR(BoundMemberAccess)
{
    auto access = std::dynamic_pointer_cast<BoundMemberAccess>(tree);
    TRY_RETURN(process(access->structure(), ctx));
    ctx.write(format(".{}", access->member()->name()));
    return tree;
}

NODE_PROCESSOR(BoundArrayAccess)
{
    auto access = std::dynamic_pointer_cast<BoundArrayAccess>(tree);
    TRY_RETURN(process(access->array(), ctx));
    ctx.write("[");
    TRY_RETURN(process(access->subscript(), ctx));
    ctx.write("]");
    return tree;
}

NODE_PROCESSOR(BoundAssignment)
{
    auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
    TRY_RETURN(process(assignment->assignee(), ctx));
    ctx.write(" = ");
    TRY_RETURN(process(assignment->expression(), ctx));
    return tree;
}

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);

    if (std::dynamic_pointer_cast<BoundStaticVariableDeclaration>(var_decl) || std::dynamic_pointer_cast<BoundLocalVariableDeclaration>(var_decl))
        ctx.write("static ");
    type_to_c_type(ctx, var_decl->type());
    ctx.write(format(" {}", var_decl->name()));
    if (var_decl->type()->type() == PrimitiveType::Array) {
        assert(var_decl->type()->is_template_specialization());
        auto size = var_decl->type()->template_argument<long>("size");
        ctx.write(format("[{}]", size));
    }
    ctx.write(" = ");
    if (var_decl->expression() != nullptr) {
        TRY_RETURN(process(var_decl->expression(), ctx));
    } else {
        ctx.write(type_initialize(var_decl->type()));
    }
    ctx.writeln(";");
    return tree;
}

ALIAS_NODE_PROCESSOR(BoundStaticVariableDeclaration, BoundVariableDeclaration)
ALIAS_NODE_PROCESSOR(BoundGlobalVariableDeclaration, BoundVariableDeclaration)
ALIAS_NODE_PROCESSOR(BoundLocalVariableDeclaration, BoundVariableDeclaration)

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

NODE_PROCESSOR(BoundForStatement)
{
    auto for_stmt = std::dynamic_pointer_cast<BoundForStatement>(tree);
    auto range = std::dynamic_pointer_cast<BoundBinaryExpression>(for_stmt->range());
    assert(range != nullptr && range->op() == BinaryOperator::Range);
    ctx.write(format("for ({} {} = ", type_to_c_type(for_stmt->variable()->type()), for_stmt->variable()->name()));
    TRY_RETURN(process(range->lhs(), ctx));
    ctx.write(format("; {} < ", for_stmt->variable()->name()));
    TRY_RETURN(process(range->rhs(), ctx));
    ctx.writeln(format("; ++{}) {", for_stmt->variable()->name()));
    ctx.indent();
    TRY_RETURN(process(for_stmt->statement(), ctx));
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
    obl_dir = config.obelix_directory();
    fs::create_directory(".obelix");

    auto ret = process(tree, root);

    if (ret.is_error()) {
        return ret;
    }

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

    std::vector<std::string> modules;
    for (auto& module_file : root.files()) {
        auto p = fs::path(".obelix/" + module_file->name());

        if (config.cmdline_flag("show-c-file")) {
            std::cout << module_file->to_string();
        }

        std::vector<std::string> cc_args = { p.string(), "-c", "-o", p.replace_extension("o"), format("-I{}/include", obl_dir), "-O3" };
        if (auto code = execute("cc", cc_args); code.is_error())
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

#if 0
        static std::string sdk_path; // "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk";
        if (sdk_path.empty()) {
            Process process("xcrun", "-sdk", "macosx", "--show-sdk-path");
            if (auto exit_code_or_error = process.execute(); exit_code_or_error.is_error())
                return SyntaxError { exit_code_or_error.error(), Token {} };
            sdk_path = strip(process.standard_out());
        }
#endif

        auto p_here = fs::path { join(split(file_name, '/'), "-") };
        std::vector<std::string> ld_args = { "-o", p_here.replace_extension(""), "-loblcrt", format("-L{}/lib", obl_dir) };
        for (auto& m : modules)
            ld_args.push_back(m);

        if (auto code = execute("cc", ld_args); code.is_error())
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
