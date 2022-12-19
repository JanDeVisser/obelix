/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <unistd.h>
#include <filesystem>
#include <memory>
#include <set>

#include <core/Error.h>
#include <core/Logging.h>
#include <core/Process.h>
#include <obelix/Processor.h>
#include <obelix/transpile/c/CTranspiler.h>
#include <obelix/transpile/c/CTranspilerIntrinsics.h>
#include <obelix/transpile/c/CTranspilerContext.h>

namespace fs = std::filesystem;

namespace Obelix {

logging_category(c_transpiler);

static std::string obl_dir;

std::string c_function_name(pBoundFunctionDecl const& function, CTranspilerContext &ctx)
{
    static std::map<std::string, std::string> name_for_function;
    static std::set<size_t> hashes;

    if (name_for_function.contains(function->to_string()))
        return name_for_function.at(function->to_string());

    if (std::dynamic_pointer_cast<BoundIntrinsicDecl>(function) != nullptr)
        return function->name();

    if (auto native = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(function); native != nullptr)
        return native->native_function_name();

    if (function->name() == "main")
        return "$main";

    std::string function_name;
    if (auto method = std::dynamic_pointer_cast<BoundMethodDecl>(function); method != nullptr) {
        function_name = format("${}${}", method->method()->method_of()->name(), function->name());
    } else {
        function_name = format("{}${}", function->module(), function->name());
    }
    replace_all(function_name, "/", "$");

    if (function->parameters().empty())
        return function_name;

    size_t hash = 0u;
    size_t shift = 1;
    do {
        for (auto const& param : function->parameters()) {
            hash = (hash << shift++) ^ std::hash<ObjectType> {}(*param->type());
        }
        hash %= 4096;
    } while (hashes.contains(hash) && (shift < 62));
    auto ret = format("{}_{}", function_name, hash);
    name_for_function[function->to_string()] = ret;
    hashes.insert(hash);
    return ret;
}

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
    case PrimitiveType::String:
    case PrimitiveType::Struct:
    case PrimitiveType::Enum:
    case PrimitiveType::Conditional:
    case PrimitiveType::Void:
        c_type = type->name();
        break;
    default:
        fatal("Can't convert {} types to C types yet", type->type());
    }
    return c_type;
}

void type_to_c_type(CTranspilerContext &ctx, std::shared_ptr<ObjectType> const& type)
{
    write(ctx, type_to_c_type(type));
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
    case PrimitiveType::String:
        initial_value = "str_view_for(\"\")";
        break;
    case PrimitiveType::Struct:
    case PrimitiveType::Array:
    case PrimitiveType::Conditional:
        initial_value = "{ 0 }";
        break;
    default:
        fatal("Can't initialize {} variables in C yet", type->type());
    }
    return initial_value;
}

void function_decl(CTranspilerContext& ctx, pBoundFunctionDecl const& function, bool parameter_names = false)
{
    type_to_c_type(ctx, function->type());
    write(ctx, format(" {}(", c_function_name(function, ctx)));
    auto method = std::dynamic_pointer_cast<BoundMethodDecl>(function);
    if (method != nullptr) {
        type_to_c_type(ctx, method->method()->method_of());
        if (parameter_names)
            write(ctx, " $this");
    }
    for (auto const& param : function->parameters()) {
        write(ctx, ", ");
        type_to_c_type(ctx, param->type());
        if (parameter_names)
            write(ctx, param->name());
    }
    write(ctx, ")");
}

ErrorOr<void, SyntaxError> evaluate_arguments(CTranspilerContext& ctx, ProcessResult& result, std::shared_ptr<BoundFunctionDecl> const& decl, BoundExpressions const& arguments)
{
    auto first { true };
    for (auto const& arg : arguments) {
        if (!first)
            write(ctx, ", ");
        TRY_RETURN(process(arg, ctx, result));
        first = false;
    }
    return {};
}

template <typename EmitFunction>
ErrorOr<void, SyntaxError> function_call(CTranspilerContext& ctx, ProcessResult& result, pBoundFunctionCall const& call, EmitFunction const& emitter)
{
    writeln(ctx, "({");
    indent(ctx);
    if (auto method = std::dynamic_pointer_cast<BoundMethodDecl>(call->declaration()); method != nullptr) {
        type_to_c_type(ctx, method->method()->method_of());
        write(ctx, " $this = ");
        TRY_RETURN(process(std::dynamic_pointer_cast<BoundMethodCall>(call)->self(), ctx));
        writeln(ctx, ";");
    }
    auto count { 0 };
    for (auto const& arg : call->arguments()) {
        type_to_c_type(ctx, arg->type());
        write(ctx, format(" $arg{} = ", count++));
        TRY_RETURN(process(arg, ctx));
        writeln(ctx, ";");
    }
    if (call->type()->type() != PrimitiveType::Void)
        write(ctx, format("{} $eval_result = ", type_to_c_type(call->type())));
    TRY_RETURN(emitter());
    writeln(ctx, ";");
    count = 0;
    for (auto const& arg : call->arguments()) {
        auto method_descr = arg->type()->get_method(Operator::Destructor, {});
        if (method_descr != nullptr) {
            writeln(ctx, "({");
            indent(ctx);
            writeln(ctx, format("{} $self = $arg{};", type_to_c_type(arg->type()), count));
            CTranspilerFunctionType impl = get_c_transpiler_intrinsic(method_descr->implementation().intrinsic);
            if (!impl)
                return SyntaxError { ErrorCode::InternalError, call->token(), format("No C Transpiler implementation for destructor of {}", arg->type()) };
            TRY_RETURN(impl(ctx, {}));
            dedent(ctx);
            writeln(ctx, "});");
        }
        count++;
    }
    if (call->type()->type() != PrimitiveType::Void)
        writeln(ctx, "$eval_result;");
    dedent(ctx);
    write(ctx, "})");
    return {};
}

ErrorOr<void,SyntaxError> transpile_block(pBlock const& block, CTranspilerContext& ctx, ProcessResult& result)
{
    CTranspilerContext& block_ctx = make_subcontext<CTranspilerContext>(ctx);
    for (auto& stmt : block->statements()) {
        TRY_RETURN(process(stmt, block_ctx, result));
        if (auto child_block = std::dynamic_pointer_cast<Block>(stmt); child_block != nullptr) {
            writeln(ctx, format("if ($return_triggered) goto {};", exit_label(block_ctx)));
        }
    }
    writeln(ctx, format("{}: ;", exit_label(block_ctx)));
    for (auto const& name_var : block_ctx.names()) {
        auto variable = std::dynamic_pointer_cast<BoundVariableDeclaration>(name_var.second);
        if (variable->is_static())
            continue;
        auto method_descr = variable->type()->get_method(Operator::Destructor, {});
        if (method_descr == nullptr)
            continue;
        writeln(ctx, "({");
        indent(ctx);
        writeln(ctx, format("{} $self = {};", type_to_c_type(variable->type()), variable->name()));
        CTranspilerFunctionType impl = get_c_transpiler_intrinsic(method_descr->implementation().intrinsic);
        if (!impl)
            return SyntaxError { ErrorCode::InternalError, variable->token(), format("No C Transpiler implementation for destructor of {}", variable->type()) };
        TRY_RETURN(impl(ctx, {}));
        dedent(ctx);
        writeln(ctx, "});");
    }
    return {};
}

INIT_NODE_PROCESSOR(CTranspilerContext)

NODE_PROCESSOR(BoundCompilation)
{
    auto compilation = std::dynamic_pointer_cast<BoundCompilation>(tree);
    TRY_RETURN(open_header(ctx, compilation->main_module()));
    writeln(ctx, format(
R"(/*
 * This is generated code. Modify at your peril.
 */

#ifndef __OBELIX_{}_H__
#define __OBELIX_{}_H__

)", to_upper(compilation->main_module()), to_upper(compilation->main_module())));
    for (auto const& bound_type : compilation->custom_types()) {
        auto type = bound_type->type();
        switch (type->type()) {
        case PrimitiveType::Conditional: {
            writeln(ctx, format("typedef struct _{} {", type->name()));
            writeln(ctx, "  bool success;");
            writeln(ctx, "  union {");
            writeln(ctx, format("    {} value;", type_to_c_type(type->template_argument<std::shared_ptr<ObjectType>>("success_type"))));
            writeln(ctx, format("    {} error;", type_to_c_type(type->template_argument<std::shared_ptr<ObjectType>>("error_type"))));
            writeln(ctx, "  };");
            writeln(ctx, format("} {};\n", type->name()));
            break;
        }
        case PrimitiveType::Struct: {
            writeln(ctx, format("typedef struct _{} {", type->name()));
            for (auto const& f : type->fields()) {
                writeln(ctx, format("  {} {};", type_to_c_type(f.type), f.name));
            }
            writeln(ctx, format("} {};\n", type->name()));
            break;
        }
        case PrimitiveType::Enum: {
            writeln(ctx, format("typedef enum _{} {", type->name()));
            indent(ctx);
            for (auto const& v : type->template_argument_values<NVP>("values")) {
                writeln(ctx, format("{} = {},", v.first, v.second));
            }
            dedent(ctx);
            writeln(ctx, format("} {};\n", type->name()));
            writeln(ctx, format("extern $enum_value $_{}_values[];", type->name()));
            break;
        }
        case PrimitiveType::Array: {
            break;
        }
        default:
            fatal("Cannot declare custom type {}", type);
        }
    }
    for (auto const& module : compilation->modules()) {
        if (module->name() == "/")
            continue;
        auto num_exports = 0u;
        for (auto const& exprt : module->exports()) {
            if (num_exports == 0)
                writeln(ctx, format("\n/* Exported by {}: */\n", module->name()));
            num_exports++;
            if (auto function = std::dynamic_pointer_cast<BoundFunctionDecl>(exprt); function != nullptr) {
                write(ctx, "extern ");
                function_decl(ctx, function, false);
                writeln(ctx, ";");
            }
            if (auto variable = std::dynamic_pointer_cast<BoundGlobalVariableDeclaration>(exprt); variable != nullptr) {
                write(ctx, "extern ");
                type_to_c_type(ctx, variable->type());
                writeln(ctx, format(" {};", variable->name()));
            }
        }
    }
    for (auto const& module : compilation->modules()) {
        if (module->name() == "/")
            continue;
        for (auto stmt : module->block()->statements()) {
            auto bound_type = std::dynamic_pointer_cast<BoundStructDefinition>(stmt);
            if (bound_type == nullptr)
                continue;
            auto num_methods = 0u;
            auto struct_ctx = ctx.make_subcontext();
            for (auto const& method : bound_type->methods()) {
                auto bound_method = std::dynamic_pointer_cast<BoundFunctionDef>(method);
                if (bound_method == nullptr)
                    continue;
                if (num_methods == 0)
                    writeln(ctx, format("\n/* Methods of {}: */\n", bound_type->name()));
                num_methods++;
                write(ctx, "extern ");
                function_decl(struct_ctx, bound_method->declaration(), false);
                writeln(ctx, ";");
            }
        }
    }
    writeln(ctx, format(
R"(
#endif /* __OBELIX_{}_H__ */
)", to_upper(compilation->main_module())));
    TRY_RETURN(flush(ctx));
    return process_tree(tree, ctx, result, CTranspilerContext_processor);
}

NODE_PROCESSOR(BoundModule)
{
    auto module = std::dynamic_pointer_cast<BoundModule>(tree);
    auto name = module->name();
    if (name == "/")
        return tree;

    if (name.starts_with("./"))
        name = name.substr(2);
    if (to_lower(name).ends_with(".obl"))
        name = name.substr(0, name.length() - 4);

    fs::path path { join(split(name, '/'), "-") };
    TRY_RETURN(open_output_file(ctx, path.replace_extension(fs::path("c"))));
    writeln(ctx, format(
R"(/*
 * This is generated code. Modify at your peril.
 */

#include <obelix.h>
#include "{}"

)", ctx.root_data().header_name()));
    TRY_RETURN(process_tree(module->block(), ctx, result, CTranspilerContext_processor));
    TRY_RETURN(flush(ctx));
    return tree;
}

NODE_PROCESSOR(Block)
{
    auto block = std::dynamic_pointer_cast<Block>(tree);
    writeln(ctx, format("{{ // {}", exit_label(ctx)));
    indent(ctx);
    TRY_RETURN(transpile_block(block, ctx, result));
    dedent(ctx);
    writeln(ctx, format("} // {}\n", exit_label(ctx)));
    return tree;
}

NODE_PROCESSOR(FunctionBlock)
{
    auto block = std::dynamic_pointer_cast<FunctionBlock>(tree);
    writeln(ctx, format("{{ // {}", exit_label(ctx)));
    indent(ctx);
    if (block->declaration()->type()->type() != PrimitiveType::Void) {
        writeln(ctx, format("{} $function_return_value;", type_to_c_type(block->declaration()->type())));
    }
    writeln(ctx, "int $return_triggered = 0;");
    TRY_RETURN(transpile_block(block, ctx, result));
    write(ctx, "return");
    if (block->declaration()->type()->type() != PrimitiveType::Void) {
        write(ctx, " $function_return_value");
    }
    writeln(ctx, ";");
    dedent(ctx);
    writeln(ctx, format("} // {}\n", exit_label(ctx)));
    return tree;
}

NODE_PROCESSOR(BoundEnumDef)
{
    auto enum_def = std::dynamic_pointer_cast<BoundEnumDef>(tree);
    writeln(ctx, format("$enum_value $_{}_values[] = {", enum_def->type()->name()));
    indent(ctx);
    for (auto const& v : enum_def->type()->template_argument_values<NVP>("values")) {
        writeln(ctx, format("{{{}, \"{}\"},", v.second, v.first));
    }
    writeln(ctx, "{ 0, NULL }");
    dedent(ctx);
    writeln(ctx, "};\n");
    return tree;
}

NODE_PROCESSOR(BoundStructDefinition)
{
    auto struct_def = std::dynamic_pointer_cast<BoundStructDefinition>(tree);
    auto struct_ctx = ctx.make_subcontext();
    for (auto method_stmt : struct_def->methods()) {
        if (auto method = std::dynamic_pointer_cast<BoundFunctionDef>(method_stmt); method != nullptr) {
            TRY_RETURN(process(method, ctx, result));
        }
    }
    return tree;
}

NODE_PROCESSOR(BoundFunctionDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(tree);
    function_decl(ctx, func_decl, true);
    return tree;
}

ALIAS_NODE_PROCESSOR(BoundMethodDecl, BoundFunctionDecl)

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    TRY_RETURN(process(func_def->declaration(), ctx));
    auto s = func_def->statement();
    TRY_RETURN(process(func_def->statement(), ctx));
    return tree;
}

NODE_PROCESSOR(BoundFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
    TRY_RETURN(function_call(ctx, result, call, [&call, &ctx]() -> ErrorOr<void, SyntaxError> {
        write(ctx, c_function_name(call->declaration(), ctx));
        write(ctx, "(");
        for (auto ix = 0; ix < call->arguments().size(); ++ix) {
            if (ix > 0)
                write(ctx, ", ");
            write(ctx, format("$arg{}", ix));
        }
        write(ctx, ")");
        return {};
    }));
    return tree;
}

NODE_PROCESSOR(BoundNativeFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
    TRY_RETURN(function_call(ctx, result, call, [&call, &ctx]() -> ErrorOr<void, SyntaxError> {
        write(ctx, std::dynamic_pointer_cast<BoundNativeFunctionDecl>(call->declaration())->native_function_name());
        write(ctx, "(");
        for (auto ix = 0; ix < call->arguments().size(); ++ix) {
            if (ix > 0)
                write(ctx, ", ");
            write(ctx, format("$arg{}", ix));
        }
        write(ctx, ")");
        return {};
    }));
    return tree;
}

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
    TRY_RETURN(function_call(ctx, result, call, [&call, &ctx]() -> ErrorOr<void, SyntaxError> {
        CTranspilerFunctionType impl = get_c_transpiler_intrinsic(call->intrinsic());
        if (!impl)
            return SyntaxError { ErrorCode::InternalError, call->token(), format("No C Transpiler implementation for intrinsic {}", call->to_string()) };
        TRY_RETURN(impl(ctx, call->argument_types()));
        return {};
    }));
    return tree;
}

NODE_PROCESSOR(BoundMethodCall)
{
    auto call = std::dynamic_pointer_cast<BoundMethodCall>(tree);
    TRY_RETURN(function_call(ctx, result, call, [&call, &ctx]() -> ErrorOr<void, SyntaxError> {
        write(ctx, c_function_name(call->declaration(), ctx));
        write(ctx, "($this");
        for (auto ix = 0; ix < call->arguments().size(); ++ix) {
            write(ctx, ", ");
            write(ctx, format("$arg{}", ix));
        }
        write(ctx, ")");
        return {};
    }));
    return tree;
}

NODE_PROCESSOR(BoundCastExpression)
{
    auto cast = std::dynamic_pointer_cast<BoundCastExpression>(tree);
    write(ctx, format("({}) ", type_to_c_type(cast->type())));
    TRY_RETURN(process(cast->expression(), ctx));
    return tree;
}

NODE_PROCESSOR(BoundIntLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(tree);
    write(ctx, format("{}", literal->int_value()));
    return tree;
}

NODE_PROCESSOR(BoundEnumValue)
{
    auto enum_value = std::dynamic_pointer_cast<BoundEnumValue>(tree);
    write(ctx, format("{}", enum_value->label()));
    return tree;
}

NODE_PROCESSOR(BoundStringLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundStringLiteral>(tree);
    auto s = literal->value();
    replace_all(s, "\n", "\\n");
    write(ctx, format(R"(str_view_for("{}"))", s));
    return tree;
}

NODE_PROCESSOR(BoundBooleanLiteral)
{
    auto literal = std::dynamic_pointer_cast<BoundBooleanLiteral>(tree);
    write(ctx, format("{}", literal->value()));
    return tree;
}

NODE_PROCESSOR(BoundVariable)
{
    auto variable = std::dynamic_pointer_cast<BoundVariable>(tree);
    if (variable->type()->type() != PrimitiveType::String)
        write(ctx, variable->name());
    else
        write(ctx, format("str_copy({})", variable->name()));
    return tree;
}

NODE_PROCESSOR(BoundConditionalValue)
{
    auto conditional_value = std::dynamic_pointer_cast<BoundConditionalValue>(tree);
    write(ctx, format(R"(({}) {{ .success={}, .{}=)", conditional_value->type()->name(), conditional_value->success(),
        conditional_value->success() ? "value" : "error"));
    TRY_RETURN(process(conditional_value->expression(), ctx));
    write(ctx, " }");
    return tree;
}

#define CONDITIONAL_VALUE_ERROR "Can't access 'value' field when conditional status is error"
#define CONDITIONAL_ERROR_ERROR "Can't access 'error' field when conditional status is success"

NODE_PROCESSOR(BoundMemberAccess)
{
    auto access = std::dynamic_pointer_cast<BoundMemberAccess>(tree);
    switch (access->structure()->type()->type()) {
    case PrimitiveType::Module: {
        TRY_RETURN(process(access->member(), ctx));
        break;
    }
    case PrimitiveType::Struct: {
        TRY_RETURN(process(access->structure(), ctx));
        write(ctx, format(".{}", access->member()->name()));
        break;
    }
    case PrimitiveType::Conditional: {
        writeln(ctx, "({");
        indent(ctx);
        write(ctx, format(R"({} cond = )", access->structure()->type()->name()));
        TRY_RETURN(process(access->structure(), ctx));
        writeln(ctx, ";");
        char const *msg = (access->member()->name() == "value") ? CONDITIONAL_VALUE_ERROR : CONDITIONAL_ERROR_ERROR;
        char const *invert = (access->member()->name() == "value") ? "!" : "";
        auto const& loc = access->token().location;
        writeln(ctx, format(R"(if ({}cond.success) $fatal(($token) {{ .file_name="{}", .line_start={}, .column_start={}, .line_end={}, .column_end={}}, "{}");)",
            invert, loc.file_name, loc.start_line, loc.start_column, loc.end_line, loc.end_column, msg));
        writeln(ctx, format(R"(cond.{};)", access->member()->name()));
        dedent(ctx);
        write(ctx, "})");
        break;
    }
    default:
        fatal("Unreachable");
    }
    return tree;
}

NODE_PROCESSOR(BoundMemberAssignment)
{
    auto access = std::dynamic_pointer_cast<BoundMemberAssignment>(tree);
    switch (access->structure()->type()->type()) {
    case PrimitiveType::Struct: {
        TRY_RETURN(process(access->structure(), ctx));
        write(ctx, format(".{}", access->member()->name()));
        break;
    }
    case PrimitiveType::Conditional: {
        TRY_RETURN(process(access->structure(), ctx));
        writeln(ctx, format(R"(.success = {};)", access->member()->name() == "value"));
        TRY_RETURN(process(access->structure(), ctx));
        write(ctx, format(".{}", access->member()->name()));
        break;
    }
    default:
        fatal("Unreachable");
    }
    return tree;
}

NODE_PROCESSOR(BoundArrayAccess)
{
    auto access = std::dynamic_pointer_cast<BoundArrayAccess>(tree);
    TRY_RETURN(process(access->array(), ctx));
    write(ctx, "[");
    TRY_RETURN(process(access->subscript(), ctx));
    write(ctx, "]");
    return tree;
}

NODE_PROCESSOR(BoundAssignment)
{
    auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
    TRY_RETURN(process(assignment->assignee(), ctx));
    write(ctx, " = ");
    TRY_RETURN(process(assignment->expression(), ctx));
    return tree;
}

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);

    if (var_decl->is_static())
        write(ctx, "static ");
    type_to_c_type(ctx, var_decl->type());
    write(ctx, format(" {}", var_decl->name()));
    if (var_decl->type()->type() == PrimitiveType::Array) {
        assert(var_decl->type()->is_template_specialization());
        auto size = var_decl->type()->template_argument<long>("size");
        write(ctx, format("[{}]", size));
    }
    write(ctx, " = ");
    if (var_decl->expression() != nullptr) {
        TRY_RETURN(process(var_decl->expression(), ctx));
    } else {
        write(ctx, type_initialize(var_decl->type()));
    }
    writeln(ctx, ";");
    ctx.declare(var_decl->name(), var_decl);
    return tree;
}

ALIAS_NODE_PROCESSOR(BoundStaticVariableDeclaration, BoundVariableDeclaration)
ALIAS_NODE_PROCESSOR(BoundGlobalVariableDeclaration, BoundVariableDeclaration)
ALIAS_NODE_PROCESSOR(BoundLocalVariableDeclaration, BoundVariableDeclaration)

NODE_PROCESSOR(BoundExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
    TRY_RETURN(process(expr_stmt->expression(), ctx));
    writeln(ctx, ";");
    return tree;
}

NODE_PROCESSOR(BoundReturn)
{
    auto ret = std::dynamic_pointer_cast<BoundReturn>(tree);
    if (ret->expression()) {
        write(ctx, "$function_return_value = ");
        TRY_RETURN(process(ret->expression(), ctx));
        writeln(ctx, ";");
    }
    writeln(ctx, "$return_triggered = 1;");
    writeln(ctx, format("goto {};", exit_label(ctx)));
    return tree;
}

NODE_PROCESSOR(BoundWhileStatement)
{
    auto while_stmt = std::dynamic_pointer_cast<BoundWhileStatement>(tree);

    write(ctx, "while (");
    TRY_RETURN(process(while_stmt->condition(), ctx));
    TRY_RETURN(process(while_stmt->statement(), ctx));
    return tree;
}

NODE_PROCESSOR(BoundForStatement)
{
    auto for_stmt = std::dynamic_pointer_cast<BoundForStatement>(tree);
    auto range = std::dynamic_pointer_cast<BoundBinaryExpression>(for_stmt->range());
    assert(range != nullptr && range->op() == BinaryOperator::Range);
    write(ctx, format("for ({} {} = ", type_to_c_type(for_stmt->variable()->type()), for_stmt->variable()->name()));
    TRY_RETURN(process(range->lhs(), ctx));
    write(ctx, format("; {} < ", for_stmt->variable()->name()));
    TRY_RETURN(process(range->rhs(), ctx));
    writeln(ctx, format("; ++{})", for_stmt->variable()->name()));
    TRY_RETURN(process(for_stmt->statement(), ctx));
    return tree;
}

NODE_PROCESSOR(BoundIfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);

    auto first { true };
    for (auto& branch : if_stmt->branches()) {
        if (!first)
            write(ctx, "else ");
        if (branch->condition() != nullptr) {
            write(ctx, "if (");
            TRY_RETURN(process(branch->condition(), ctx));
            write(ctx, ") ");
        }
        TRY_RETURN(process(branch->statement(), ctx));
        if (branch->condition() == nullptr)
            break;
        first = false;
    }
    return tree;
}

NODE_PROCESSOR(BoundSwitchStatement)
{
    auto switch_stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
    auto default_case = switch_stmt->default_case();

    write(ctx, "switch (");
    TRY_RETURN(process(switch_stmt->expression(), ctx));
    writeln(ctx, ") {");
    for (auto const& switch_case : switch_stmt->cases()) {
        if (switch_case->condition() == nullptr) {
            default_case = switch_case;
            break;
        }
        write(ctx, "case ");
        TRY_RETURN(process(switch_case->condition(), ctx));
        write(ctx, ": ");
        TRY_RETURN(process(switch_case->statement(), ctx));
        writeln(ctx, "break;");
    }
    write(ctx, "default: ");
    if (default_case != nullptr)
        TRY_RETURN(process(default_case, ctx));
    writeln(ctx, "break;");
    writeln(ctx, "}");
    return tree;
}

ProcessResult transpile_to_c(std::shared_ptr<SyntaxNode> const& tree, Config const& config)
{
    CTranspilerContext root(config);
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
    std::vector<fs::path> output_files;
    auto compiler = config.cmdline_flag<std::string>("with-c-compiler", "cc");
    auto linker = config.cmdline_flag<std::string>("with-c-linker", compiler);

    for (auto& module_file : files(root)) {
        auto p = fs::path(".obelix") / module_file->name();
        output_files.push_back(p);

        if (config.cmdline_flag<bool>("show-c-file")) {
            std::cout << module_file->to_string();
        }
        if (module_file->name().ends_with(".h"))
            continue;
        auto o_file = p;
        o_file.replace_extension("o");
        unlink(o_file.c_str());
        std::vector<std::string> cc_args = { p.string(), "-c", "-o", o_file, format("-I{}/include", obl_dir), "-O3" };
        if (auto code = execute(compiler, cc_args); code.is_error())
            return SyntaxError { code.error(), Token {} };
        modules.push_back(o_file);
    }
    if (!config.cmdline_flag<bool>("keep-c-file")) {
        for (auto const& file : output_files)
            unlink(file.c_str());
    }

    if (!modules.empty()) {
        std::vector<std::string> ld_args = { "-o", config.main(), "-loblcrt", format("-L{}/lib", obl_dir) };
        for (auto& m : modules)
            ld_args.push_back(m);

        if (auto code = execute(linker, ld_args); code.is_error())
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
