/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>

#include <config.h>
#include <core/Logging.h>
#include <core/ScopeGuard.h>
#include <obelix/ARM64Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/ARM64.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>

namespace Obelix {

ErrorOr<void> bool_unary_expression(ARM64Context& ctx, UnaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::ExclamationPoint:
        ctx.assembly().add_instruction("eorb", "w{},w{},#0x01", ctx.get_target_register()); // a is 0b00000001 (a was true) or 0b00000000 (a was false
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> bool_bool_binary_expression(ARM64Context& ctx, BinaryExpression const& expr)
{
    auto lhs = ctx.add_target_register();
    auto rhs = ctx.get_rhs_register();
    switch (expr.op().code()) {
    case TokenCode::LogicalAnd:
        ctx.assembly().add_instruction("and", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::LogicalOr:
        ctx.assembly().add_instruction("orr", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Hat:
        ctx.assembly().add_instruction("xor", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Equals: {
        ctx.assembly().add_instruction("eor", "x{},x{},x{}", lhs, lhs, rhs); // a is 0b00000000 (a == b) or 0b00000001 (a != b
        ctx.assembly().add_instruction("eor", "x{},x{},#0x01", lhs, lhs);    // a is 0b00000001 (a == b) or 0b00000000 (a != b
        break;
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> int_unary_expression(ARM64Context& ctx, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    auto operand = ctx.get_target_register();
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeUnsigned)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };
        ctx.assembly().add_instruction("neg", "x{},x{}", operand, operand);
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvn", "x{},x{}", operand, operand);
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> int_int_binary_expression(ARM64Context& ctx, BinaryExpression const& expr)
{
    auto lhs = ctx.add_target_register();
    auto rhs = ctx.get_rhs_register();
    switch (expr.op().code()) {
    case TokenCode::Plus:
        ctx.assembly().add_instruction("add", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("sub", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Asterisk:
        ctx.assembly().add_instruction("mul", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::Slash:
        ctx.assembly().add_instruction("sdiv", "x{},x{},x{}", lhs, lhs, rhs);
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmp", "x{},x{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("mov", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmp", "x{},x{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b.le", set_false);
        ctx.assembly().add_instruction("mov", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("mov", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmp", "x{},x{}", lhs, rhs);
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b.lt", set_true);
        ctx.assembly().add_instruction("mov", "w{},wzr", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("mov", "w{},#0x01", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> byte_unary_expression(ARM64Context& ctx, UnaryExpression const& expr)
{
    if (expr.op().code() == TokenCode::Plus)
        return {};
    auto operand = ctx.get_target_register();
    switch (expr.op().code()) {
    case TokenCode::Minus: {
        if (expr.operand()->type() == ObelixType::TypeUnsigned)
            return Error { ErrorCode::SyntaxError, "Cannot negate unsigned numbers" };

        ctx.assembly().add_instruction("neg", "w{},w{}", operand, operand);
        break;
    }
    case TokenCode::Tilde:
        ctx.assembly().add_instruction("mvnb", "w{},w{}", operand, operand);
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> byte_byte_binary_expression(ARM64Context& ctx, BinaryExpression const& expr)
{
    auto lhs = ctx.add_target_register();
    auto rhs = ctx.get_rhs_register();
    switch (expr.op().code()) {
    case TokenCode::Plus:
        ctx.assembly().add_instruction("andb", "w{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::Minus:
        ctx.assembly().add_instruction("subb", "w{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::Asterisk:
        ctx.assembly().add_instruction("smull", "x{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::Slash:
        ctx.assembly().add_instruction("sdiv", "w{},w{},w{}", lhs, lhs, rhs);
        break;
    case TokenCode::EqualsTo: {
        ctx.assembly().add_instruction("cmp", "w{},w{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bne", set_false);
        ctx.assembly().add_instruction("movb", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::GreaterThan: {
        ctx.assembly().add_instruction("cmp", "w{},w{}", lhs, rhs);
        auto set_false = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_false);
        ctx.assembly().add_instruction("movb", "w{},#0x01", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_false);
        ctx.assembly().add_instruction("movb", "w{},wzr", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    case TokenCode::LessThan: {
        ctx.assembly().add_instruction("cmp", "w{},w{}", lhs, rhs);
        auto set_true = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("bmi", set_true);
        ctx.assembly().add_instruction("movb", "w{},wzr", lhs);
        auto done = format("lbl_{}", Obelix::Label::reserve_id());
        ctx.assembly().add_instruction("b", done);
        ctx.assembly().add_label(set_true);
        ctx.assembly().add_instruction("movb", "w{},#0x01", lhs);
        ctx.assembly().add_label(done);
        return {};
    }
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOr<void> string_binary_expression(ARM64Context& ctx, BinaryExpression const& expr)
{
    switch (expr.op().code()) {
    case TokenCode::Plus:
        break;
    case TokenCode::Asterisk:
        break;
    default:
        return Error(ErrorCode::NotYetImplemented, format("Cannot emit operation of type {} yet", expr.op().value()));
    }
    return {};
}

ErrorOrNode output_arm64_processor(std::shared_ptr<SyntaxNode> const& tree, ARM64Context& ctx)
{
    switch (tree->node_type()) {
    case SyntaxNodeType::MaterializedFunctionDef: {
        auto func_def = std::dynamic_pointer_cast<MaterializedFunctionDef>(tree);
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
        if (call->type() == ObelixType::TypeString)
            ctx.reserve_register(1);
        // Call function:
        ctx.assembly().add_instruction("bl", call->name());
        // Add x0 to the register context
        ctx.add_target_register();
        if (call->type() == ObelixType::TypeString)
            ctx.add_target_register();
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

        ctx.clear_context();
        ctx.reserve_register(0);
        if (native_func_call->type() == ObelixType::TypeString)
            ctx.reserve_register(1);
        // Call the native function
        ctx.assembly().add_instruction("bl", func_decl->native_function_name());
        // Add x0 to the register context
        ctx.add_target_register();
        if (native_func_call->type() == ObelixType::TypeString)
            ctx.add_target_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::CompilerIntrinsic: {
        auto call = std::dynamic_pointer_cast<CompilerIntrinsic>(tree);

        ctx.new_enclosing_context();

        // This is a mmap syscall
        if (call->name() == "allocate") {
            ctx.reserve_register(0);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x1,x0");
            ctx.assembly().add_instruction("mov", "x0,xzr");
            ctx.assembly().add_instruction("mov", "w2,#3");
            ctx.assembly().add_instruction("mov", "w3,#0x1002");
            ctx.assembly().add_instruction("mov", "w4,#-1");
            ctx.assembly().add_instruction("mov", "x5,xzr");
            ctx.assembly().syscall(0xC5);
        }

        if (call->name() == "close") {
            ctx.reserve_register(0);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x06);
        }

        if (call->name() == "fputs") {
            ctx.reserve_register(0, 1, 2);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(1), ctx));
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "itoa") {
            ctx.reserve_register(0);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x2,x0");
            ctx.assembly().add_instruction("sub", "sp,sp,32");
            ctx.assembly().add_instruction("mov", "x0,[sp,-32]!");
            ctx.assembly().add_instruction("mov", "x1,#32");
            ctx.assembly().add_instruction("mov", "w3,#10");
            ctx.assembly().add_instruction("bl", "to_string");
            ctx.assembly().add_instruction("add", "sp,sp,32");
        }

        if (call->name() == "exit") {
            ctx.reserve_register(0);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x01);
        }

        if (call->name() == "eputs") {
            ctx.assembly().add_instruction("mov", "x0,#0x02");
            ctx.reserve_register(1, 2);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "fsize") {
            ctx.reserve_register(0);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            auto sz = sizeof(struct stat);
            if (sz % 16)
                sz += 16 - (sz % 16);
            ctx.assembly().add_instruction("sub", "sp,sp,#{}", sz);
            ctx.assembly().add_instruction("mov", "x1,sp");
            ctx.assembly().syscall(189);
            ctx.assembly().add_instruction("cmp", "x0,#0x00");
            auto lbl = format("lbl_{}", Label::reserve_id());
            ctx.assembly().add_instruction("bne", lbl);
            ctx.assembly().add_instruction("ldr", "x0,[sp,#{}]", offsetof(struct stat, st_size));
            ctx.assembly().add_label(lbl);
            ctx.assembly().add_instruction("add", "sp,sp,#{}", sz);
        }

        if (call->name() == "memset") {
            ctx.reserve_register(0, 1, 2);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(1), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(2), ctx));

            int count_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("mov", "x{},xzr", count_reg);

            auto loop = format("lbl_{}", Label::reserve_id());
            auto skip = format("lbl_{}", Label::reserve_id());
            ctx.assembly().add_label(loop);
            ctx.assembly().add_instruction("cmp", "x{},x2", count_reg);
            ctx.assembly().add_instruction("b.ge", skip);

            int ptr_reg = ctx.temporary_register();
            ctx.assembly().add_instruction("add", "x{},x0,x{}", ptr_reg, count_reg);
            ctx.assembly().add_instruction("strb", "w1,[x{}]", ptr_reg);
            ctx.assembly().add_instruction("add", "x{},x{},#1", count_reg, count_reg);
            ctx.assembly().add_instruction("b", loop);
            ctx.assembly().add_label(skip);
            ctx.assembly().add_instruction("mov", "x0,x{}", count_reg);
        }

        if (call->name() == "open") {
            ctx.reserve_register(0, 1, 2);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(1), ctx));
            ctx.assembly().add_instruction("mov", "x1,x2");
            ctx.assembly().syscall(0x05);
        }

        if (call->name() == "putchar") {
            ctx.reserve_register(0);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("strb", "w0,[sp,-16]!");
            ctx.assembly().add_instruction("mov", "x0,#1"); // x0: stdin
            ctx.assembly().add_instruction("mov", "x1,sp"); // x1: SP
            ctx.assembly().add_instruction("mov", "x2,#1"); // x2: Number of characters
            ctx.assembly().syscall(0x04);
            ctx.assembly().add_instruction("add", "sp,sp,16");
        }

        if (call->name() == "puts") {
            ctx.reserve_register(1, 2);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            ctx.assembly().add_instruction("mov", "x0,#1");
            ctx.assembly().syscall(0x04);
        }

        if (call->name() == "read") {
            ctx.reserve_register(0, 1, 2);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(1), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(2), ctx));
            ctx.assembly().syscall(0x03);
        }

        if (call->name() == "write") {
            ctx.reserve_register(0, 1, 2);
            TRY_RETURN(output_arm64_processor(call->arguments().at(0), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(1), ctx));
            TRY_RETURN(output_arm64_processor(call->arguments().at(2), ctx));
            ctx.assembly().syscall(0x04);
        }

        ctx.clear_context();
        ctx.reserve_register(0);
        if (call->type() == ObelixType::TypeString)
            ctx.reserve_register(1);
        // Add x0 to the register context
        ctx.add_target_register();
        if (call->type() == ObelixType::TypeString)
            ctx.add_target_register();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        if (expr->lhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->lhs()->to_string(0) };
        }
        if (expr->rhs()->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, expr->rhs()->to_string(0) };
        }

        ctx.new_inherited_context();
        ctx.new_targeted_context();
        auto rhs = TRY_AND_CAST(Expression, output_arm64_processor(expr->rhs(), ctx));
        ctx.release_register_context();

        auto lhs = TRY_AND_CAST(Expression, output_arm64_processor(expr->lhs(), ctx));
        if ((expr->lhs()->type() == ObelixType::TypeInt && expr->lhs()->type() == ObelixType::TypeInt) || (expr->lhs()->type() == ObelixType::TypeUnsigned && expr->lhs()->type() == ObelixType::TypeUnsigned)) {
            TRY_RETURN(int_int_binary_expression(ctx, *expr));
        }
        if ((expr->lhs()->type() == ObelixType::TypeByte && expr->lhs()->type() == ObelixType::TypeByte) || (expr->lhs()->type() == ObelixType::TypeChar && expr->lhs()->type() == ObelixType::TypeChar)) {
            TRY_RETURN(byte_byte_binary_expression(ctx, *expr));
        }
        if (expr->lhs()->type() == ObelixType::TypeBoolean && expr->lhs()->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_bool_binary_expression(ctx, *expr));
        }
        if (expr->lhs()->type() == ObelixType::TypeString) {
            TRY_RETURN(string_binary_expression(ctx, *expr));
        }
        ctx.clear_rhs();
        ctx.release_register_context();
        return tree;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, output_arm64_processor(expr->operand(), ctx));

        if (operand->type() == ObelixType::TypeUnknown) {
            return Error { ErrorCode::UntypedExpression, operand->to_string(0) };
        }

        if (operand->type() == ObelixType::TypeInt || operand->type() == ObelixType::TypeUnsigned) {
            TRY_RETURN(int_unary_expression(ctx, *expr));
        }
        if (operand->type() == ObelixType::TypeByte || operand->type() == ObelixType::TypeChar) {
            TRY_RETURN(byte_unary_expression(ctx, *expr));
        }
        if (operand->type() == ObelixType::TypeBoolean) {
            TRY_RETURN(bool_unary_expression(ctx, *expr));
        }
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
            ctx.assembly().add_instruction("mov", "x{},#{}", ctx.add_target_register(), val->to_long().value());
            break;
        }
        case ObelixType::TypeChar:
        case ObelixType::TypeByte:
        case ObelixType::TypeBoolean: {
            ctx.assembly().add_instruction("mov", "w{},#{}", ctx.add_target_register(), static_cast<uint8_t>(val->to_long().value()));
            break;
        }
        case ObelixType::TypeString: {
            auto str_id = ctx.assembly().add_string(val->to_string());
            ctx.assembly().add_instruction("adr", "x{},str_{}", ctx.add_target_register(), str_id);
            ctx.assembly().add_instruction("mov", "w{},#{}", ctx.add_target_register(), val->to_string().length());
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

        switch (identifier->type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("ldr", "x0,[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("ldrbs", "w{},[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("ldrb", "w{},[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("ldr", "x{},[fp,#{}]", ctx.add_target_register(), idx);
            auto reg = ctx.add_target_register();
            ctx.assembly().add_instruction("ldrw", "w{},[fp,#{}]", reg, idx + 8);
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

        switch (assignment->type()) {
        case ObelixType::TypePointer:
        case ObelixType::TypeInt:
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeUnsigned:
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeByte:
            ctx.assembly().add_instruction("strbs", "x{},[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeChar:
        case ObelixType::TypeBoolean:
            ctx.assembly().add_instruction("strb", "x{},[fp,#{}]", ctx.add_target_register(), idx);
            break;
        case ObelixType::TypeString: {
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.add_target_register(), idx);
            auto reg = ctx.add_target_register();
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
            auto reg = ctx.add_target_register();
            switch (var_decl->expression()->type()) {
            case ObelixType::TypeString: {
                ctx.assembly().add_instruction("mov", "w{},wzr", ctx.add_target_register());
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
        ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_target_register(0), var_decl->offset());
        if (ctx.get_target_count() > 1) {
            ctx.assembly().add_instruction("str", "x{},[fp,#{}]", ctx.get_target_register(1), var_decl->offset() + 8);
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
                ctx.assembly().add_instruction("cmp", "w{},0x00", ctx.get_target_register());
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
            switch (parameter.type()) {
            case ObelixType::TypeString:
                offset += 16;
                break;
            default:
                offset += 8;
            }
        }

        auto materialized_function_decl = std::make_shared<MaterializedFunctionDecl>(func_decl->identifier(), function_parameters);
        if (func_decl->node_type() == SyntaxNodeType::NativeFunctionDecl) {
            auto native_decl = std::dynamic_pointer_cast<NativeFunctionDecl>(func_decl);
            materialized_function_decl = std::make_shared<MaterializedNativeFunctionDecl>(materialized_function_decl, native_decl->native_function_name());
        }

        func_ctx.declare("#offset", offset);
        std::shared_ptr<Block> block;
        if (func_def->statement()) {
            assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
            block = TRY_AND_CAST(FunctionBlock, prepare_arm64_processor(func_def->statement(), func_ctx));
        }
        return std::make_shared<MaterializedFunctionDef>(materialized_function_decl, block, func_ctx.get("#offset").value());
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto offset = ctx.get("#offset").value();
        auto expression = TRY_AND_CAST(Expression, prepare_arm64_processor(var_decl->expression(), ctx));
        auto ret = std::make_shared<MaterializedVariableDecl>(var_decl->variable(), offset, expression, var_decl->is_const());
        switch (var_decl->type()) {
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
        if (is_intrinsic(call->name()))
            return std::make_shared<CompilerIntrinsic>(call);
        return tree;
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

    Assembly assembly;
    ARM64Context root(assembly);

    assembly.code = ".align 2\n\n";

    auto ret = output_arm64_processor(processed, root);

    if (ret.is_error()) {
        return ret;
    }

    printf("%s\n%s\n", assembly.code.c_str(), assembly.text.c_str());

    std::string bare_file_name = file_name;
    if (auto dot = file_name.find_last_of('.'); dot != std::string::npos) {
        bare_file_name = file_name.substr(0, dot);
    }
    auto assembly_file = format("{}.s", bare_file_name);
    FILE* f = fopen(assembly_file.c_str(), "w");
    if (f) {
        fprintf(f, "%s\n%s\n", assembly.code.c_str(), assembly.text.c_str());
        fclose(f);

        std::string obl_dir = (getenv("OBL_DIR")) ? getenv("OBL_DIR") : OBELIX_DIR;
        auto as_cmd = format("as -o {}.o {}.s", bare_file_name, bare_file_name);
        auto ld_cmd = format("ld -o {} {}.o -loblrt -lSystem -syslibroot `xcrun -sdk macosx --show-sdk-path` -e _start -arch arm64 -L{}/lib",
            bare_file_name, bare_file_name, obl_dir);

        std::cout << "[CMD] " << as_cmd << "\n";
        if (!system(as_cmd.c_str())) {
            std::cout << "[CMD] " << ld_cmd << "\n";
            system(ld_cmd.c_str());
        }
    }
    return ret;
}

}
