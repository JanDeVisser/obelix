/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>

#include <obelix/ARM64.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/MaterializedSyntaxNode.h>
#include <obelix/Processor.h>
#include <obelix/Syntax.h>

namespace Obelix {

class MaterializeContext : public Context<std::shared_ptr<SyntaxNode>>
{
public:
    int offset { 0 };
};

INIT_NODE_PROCESSOR(MaterializeContext);

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    auto func_decl = func_def->declaration();
    MaterializeContext func_ctx(ctx);
    int offset = 16;
    MaterializedFunctionParameters function_parameters;
    for (auto& parameter : func_decl->parameters()) {
        auto materialized_parameter = make_node<MaterializedFunctionParameter>(parameter, offset);
        func_ctx.declare(materialized_parameter->name(), materialized_parameter);
        function_parameters.push_back(materialized_parameter);
        offset += parameter->type()->size();
        if (offset % 8)
            offset += 8 - (offset % 8);
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
        func_ctx.offset = offset;
        std::shared_ptr<Block> block;
        assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
        block = TRY_AND_CAST(FunctionBlock, process(func_def->statement(), func_ctx));
        return make_node<MaterializedFunctionDef>(func_def, materialized_function_decl, block, func_ctx.offset);
    }
    default:
        fatal("Unreachable");
    }
}

NODE_PROCESSOR(FunctionBlock)
{
    auto block = std::dynamic_pointer_cast<FunctionBlock>(tree);
    Statements statements;
    for (auto& stmt : block->statements()) {
        auto new_statement = TRY_AND_CAST(Statement, process(stmt, ctx));
        statements.push_back(new_statement);
    }
    return std::make_shared<FunctionBlock>(tree->token(), statements);
}

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
    auto offset = ctx.offset;
    auto expression = TRY_AND_CAST(BoundExpression, process(var_decl->expression(), ctx));
    auto ret = make_node<MaterializedVariableDecl>(var_decl, offset, expression);
    ctx.declare(var_decl->name(), ret);
    offset += var_decl->type()->size();
    if (offset % 8)
        offset += 8 - (offset % 8);
    ctx.offset = offset;
    return ret;
}

NODE_PROCESSOR(BoundStaticVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundStaticVariableDeclaration>(tree);
    auto expression = TRY_AND_CAST(BoundExpression, process(var_decl->expression(), ctx));
    auto ret = make_node<MaterializedVariableDecl>(var_decl, format("_{}", var_decl->name()), 0, expression);
    ctx.declare(var_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
    BoundExpressions arguments;
    for (auto& expr : call->arguments()) {
        auto new_expr = process(expr, ctx);
        if (new_expr.is_error())
            return new_expr.error();
        arguments.push_back(std::dynamic_pointer_cast<BoundExpression>(new_expr.value()));
    }
    return make_node<BoundFunctionCall>(call, arguments);
}

NODE_PROCESSOR(BoundNativeFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
    BoundExpressions arguments;
    for (auto& expr : call->arguments()) {
        auto new_expr = process(expr, ctx);
        if (new_expr.is_error())
            return new_expr.error();
        arguments.push_back(std::dynamic_pointer_cast<BoundExpression>(new_expr.value()));
    }
    return make_node<BoundNativeFunctionCall>(call, arguments);
}

NODE_PROCESSOR(BoundIntrinsicCall)
{
    auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
    BoundExpressions arguments;
    for (auto& expr : call->arguments()) {
        auto new_expr = process(expr, ctx);
        if (new_expr.is_error())
            return new_expr.error();
        arguments.push_back(std::dynamic_pointer_cast<BoundExpression>(new_expr.value()));
    }
    return make_node<BoundIntrinsicCall>(call, arguments);
}

NODE_PROCESSOR(BoundUnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, process(expr->operand(), ctx));

    IntrinsicType intrinsic;
    switch (expr->op()) {
    case UnaryOperator::Dereference:
        intrinsic = IntrinsicType::dereference;
        break;
    default: {
        auto method_descr_maybe = operand->type()->get_method(to_operator(expr->op()), {});
        if (!method_descr_maybe.has_value())
            return SyntaxError { ErrorCode::InternalError, expr->token(), format("No method defined for unary operator {}::{}", operand->type()->to_string(), expr->op()) };
        auto method_descr = method_descr_maybe.value();
        auto impl = method_descr.implementation(Architecture::MACOS_ARM64);
        if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
            return SyntaxError { ErrorCode::InternalError, expr->token(), format("No intrinsic defined for {}", method_descr.name()) };
        intrinsic = impl.intrinsic;
        break;
    }
    }
    return make_node<BoundIntrinsicCall>(expr->token(), UnaryOperator_name(expr->op()), intrinsic, BoundExpressions { operand }, expr->type());
}

NODE_PROCESSOR(BoundBinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, process(expr->lhs(), ctx));
    auto rhs = TRY_AND_CAST(BoundExpression, process(expr->rhs(), ctx));

    if (lhs->type()->type() == PrimitiveType::Pointer && (expr->op() == BinaryOperator::Add || expr->op() == BinaryOperator::Subtract)) {
        std::shared_ptr<BoundExpression> offset { nullptr };
        auto target_type = (lhs->type()->is_template_specialization()) ? lhs->type()->template_arguments()[0].as_type() : get_type<uint8_t>();

        if (rhs->node_type() == SyntaxNodeType::BoundIntLiteral) {
            auto offset_literal = std::dynamic_pointer_cast<BoundIntLiteral>(rhs);
            auto offset_val = offset_literal->value();
            if (expr->op() == BinaryOperator::Subtract)
                offset_val *= -1;
            offset = make_node<BoundIntLiteral>(rhs->token(), target_type->size() * offset_val);
        } else {
            offset = rhs;
            if (expr->op() == BinaryOperator::Subtract)
                offset = make_node<BoundUnaryExpression>(expr->token(), offset, UnaryOperator::Negate, expr->type());
            auto size = make_node<BoundIntLiteral>(rhs->token(), target_type->size());
            offset = make_node<BoundBinaryExpression>(expr->token(), size, BinaryOperator::Multiply, offset, get_type<int>());
            offset = TRY_AND_CAST(BoundExpression, process(offset, ctx));
        }
        return make_node<BoundIntrinsicCall>(expr->token(), BinaryOperator_name(expr->op()), IntrinsicType::ptr_math, BoundExpressions { lhs, offset }, expr->type());
    }

    auto method_descr_maybe = lhs->type()->get_method(to_operator(expr->op()), { rhs->type() });
    if (!method_descr_maybe.has_value())
        return SyntaxError { ErrorCode::InternalError, lhs->token(), format("No method defined for binary operator {}::{}({})", 
            lhs->type()->to_string(), expr->op(), rhs->type()->to_string()) };
    auto method_descr = method_descr_maybe.value();
    auto impl = method_descr.implementation(Architecture::MACOS_ARM64);
    if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
        return SyntaxError { ErrorCode::InternalError, lhs->token(), format("No intrinsic defined for {}", method_descr.name()) };
    auto intrinsic = impl.intrinsic;

    return make_node<BoundIntrinsicCall>(expr->token(), BinaryOperator_name(expr->op()), intrinsic, BoundExpressions { lhs, rhs }, expr->type() );
}

std::shared_ptr<MaterializedIdentifier> make_materialized_identifier(std::shared_ptr<BoundIdentifier> identifier, std::string label, int offset)
{
    switch (identifier->type()->type()) {
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::SignedIntegerNumber:
        return make_node<MaterializedIntIdentifier>(identifier, move(label), offset);
    case PrimitiveType::Struct:
        return make_node<MaterializedStructIdentifier>(identifier, move(label), offset);
    default:
        fatal("Cannot materialize identifiers of type '{}' yet", identifier->type());
    }
}

std::shared_ptr<MaterializedIdentifier> make_materialized_identifier(std::shared_ptr<MaterializedDeclaration> decl, std::shared_ptr<BoundIdentifier> identifier)
{
    return make_materialized_identifier(identifier, decl->label(), decl->offset());
}

NODE_PROCESSOR(BoundIdentifier)
{
    auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(tree);
    auto decl_maybe = ctx.get(identifier->name());
    if (!decl_maybe.has_value())
        return SyntaxError { ErrorCode::InternalError, identifier->token(), format("Undeclared variable '{}' during code generation", identifier->name()) };
    auto decl = decl_maybe.value();
    auto materialized_decl = std::dynamic_pointer_cast<MaterializedDeclaration>(decl);
    if (materialized_decl != nullptr) {
        return make_materialized_identifier(materialized_decl, identifier);
    }
    return SyntaxError { ErrorCode::InternalError, format("Identifier declaration has unexpected node type '{}'", decl->node_type()) };
}

NODE_PROCESSOR(BoundMemberAccess)
{
    auto member_access = std::dynamic_pointer_cast<BoundMemberAccess>(tree);
    auto strukt = TRY_AND_CAST(MaterializedVariableAccess, process(member_access->structure(), ctx));
    auto member = member_access->member();
    auto type = strukt->type();
    auto offset = type->offset_of(member->name());
    if (offset < 0)
        return SyntaxError { ErrorCode::InternalError, member_access->token(), format("Invalid member name '{}' for struct of type '{}'", member->name(), type->name()) };
    auto materialized_member = make_materialized_identifier(member, "", strukt->offset() + offset);
    return make_node<MaterializedMemberAccess>(member_access, strukt, materialized_member);
}

NODE_PROCESSOR(BoundArrayAccess)
{
    auto array_access = std::dynamic_pointer_cast<BoundArrayAccess>(tree);
    auto array = TRY_AND_CAST(MaterializedVariableAccess, process(array_access->array(), ctx));
    auto subscript = TRY_AND_CAST(BoundExpression, process(array_access->subscript(), ctx));
    auto type = array->type()->template_arguments()[0].as_type();
    auto element_size = type->size();
   return make_node<MaterializedArrayAccess>(array_access, array, subscript, element_size);
}

ErrorOrNode materialize_arm64(std::shared_ptr<SyntaxNode> const& tree)
{
    return process<MaterializeContext>(tree);
}

}