/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "obelix/Type.h"
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
    void add_unresolved_function(std::shared_ptr<FunctionCall> func_call)
    {
        if (parent() != nullptr) {
            (static_cast<MaterializeContext*>(parent()))->add_unresolved_function(func_call);
            return;
        }
        m_unresolved_functions.push_back(func_call);
    }

    [[nodiscard]] std::vector<std::shared_ptr<FunctionCall>> const& unresolved_functions() const
    {
        if (parent() != nullptr) {
            return (static_cast<MaterializeContext*>(parent()))->unresolved_functions();
        }
        return m_unresolved_functions;
    }

    void clear_unresolved_functions()
    {
        if (parent() != nullptr) {
            (static_cast<MaterializeContext*>(parent()))->clear_unresolved_functions();
            return;
        }
        m_unresolved_functions.clear();
    }

    void add_declared_function(std::string const& name, std::shared_ptr<MaterializedFunctionDecl> func)
    {
        if (parent() != nullptr) {
            (static_cast<MaterializeContext*>(parent()))->add_declared_function(name, func);
            return;
        }
        m_declared_functions[name] = func;
    }

    [[nodiscard]] std::unordered_map<std::string, std::shared_ptr<MaterializedFunctionDecl>> const& declared_functions() const
    {
        if (parent() != nullptr) {
            return (static_cast<MaterializeContext*>(parent()))->declared_functions();
        }
        return m_declared_functions;
    }

    [[nodiscard]] std::optional<std::shared_ptr<MaterializedFunctionDecl>> declared_function(std::string const& name) const
    {
        if (parent() != nullptr) {
            return (static_cast<MaterializeContext*>(parent()))->declared_function(name);
        }
        if (m_declared_functions.contains(name))
            return m_declared_functions.at(name);
        return {};
    }

    void clear_declared_functions()
    {
        if (parent() != nullptr) {
            (static_cast<MaterializeContext*>(parent()))->clear_declared_functions();
            return;
        }
        m_declared_functions.clear();
    }

private:
    std::vector<std::shared_ptr<FunctionCall>> m_unresolved_functions;
    std::unordered_map<std::string, std::shared_ptr<MaterializedFunctionDecl>> m_declared_functions;

};

INIT_NODE_PROCESSOR(MaterializeContext);

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    auto func_decl = func_def->declaration();
    MaterializeContext func_ctx(ctx);
    int offset = 16;
    MaterializedFunctionParameters function_parameters;

    auto ngrn = 0;
    auto nsaa = 0;
    for (auto& parameter : func_decl->parameters()) {
        MaterializedFunctionParameter::ParameterPassingMethod method;
        int where;
        switch (parameter->type()->type()) {
            case PrimitiveType::IntegerNumber:
            case PrimitiveType::SignedIntegerNumber:
            case PrimitiveType::Pointer:
                if (ngrn < 8) {
                    method = MaterializedFunctionParameter::ParameterPassingMethod::Register;
                    where = ngrn++;
                    break;
                }
                method = MaterializedFunctionParameter::ParameterPassingMethod::Stack;
                where = nsaa;
                nsaa += 8;
                break;
            case PrimitiveType::Struct: {
                auto size_in_double_words = parameter->type()->size() / 8;
                if (parameter->type()->size() % 8 != 0)
                    size_in_double_words++;
                if (ngrn + size_in_double_words <= 8) {
                    method = MaterializedFunctionParameter::ParameterPassingMethod::Register;
                    where = ngrn;
                    ngrn += size_in_double_words;
                }
                method = MaterializedFunctionParameter::ParameterPassingMethod::Stack;
                where = nsaa;
                nsaa += parameter->type()->size();
                break;
            }
            default:
                fatal("Not yet implemented in materialize BoundFunctionDef");
        }

        auto materialized_parameter = make_node<MaterializedFunctionParameter>(parameter, std::make_shared<StackVariableAddress>(offset), method, where);
        func_ctx.declare(materialized_parameter->name(), materialized_parameter);
        function_parameters.push_back(materialized_parameter);
        offset += parameter->type()->size();
        if (offset % 8)
            offset += 8 - (offset % 8);
    }

    std::shared_ptr<MaterializedFunctionDecl> materialized_function_decl;
    std::shared_ptr<MaterializedFunctionDef> ret;
    switch (func_decl->node_type()) {
    case SyntaxNodeType::BoundNativeFunctionDecl: {
        auto native_decl = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(func_decl);
        materialized_function_decl = make_node<MaterializedNativeFunctionDecl>(native_decl, function_parameters, nsaa);
        ret = make_node<MaterializedFunctionDef>(func_def, materialized_function_decl, nullptr);
        break;
    }
    case SyntaxNodeType::BoundIntrinsicDecl: {
        auto intrinsic_decl = std::dynamic_pointer_cast<BoundIntrinsicDecl>(func_decl);
        materialized_function_decl = make_node<MaterializedIntrinsicDecl>(intrinsic_decl, function_parameters, nsaa);
        ret = make_node<MaterializedFunctionDef>(func_def, materialized_function_decl, nullptr);
        break;
    }
    case SyntaxNodeType::BoundFunctionDecl: {
        func_ctx.offset = offset;
        std::shared_ptr<Block> block;
        assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
        block = TRY_AND_CAST(FunctionBlock, process(func_def->statement(), func_ctx));
        materialized_function_decl = make_node<MaterializedFunctionDecl>(func_decl, function_parameters, nsaa, offset);
        ret = make_node<MaterializedFunctionDef>(func_def, materialized_function_decl, block);
        break;
    }
    default:
        fatal("Unreachable");
    }
    ctx.declare(materialized_function_decl->name(), materialized_function_decl);
    ctx.add_declared_function(materialized_function_decl->name(), materialized_function_decl);
    return ret;
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
    auto ret = make_node<MaterializedVariableDecl>(var_decl, std::make_shared<StackVariableAddress>(offset), expression);
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
    auto ret = make_node<MaterializedVariableDecl>(var_decl, std::make_shared<StaticVariableAddress>(format("_{}", var_decl->name())), expression);
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
    auto decl_maybe = ctx.declared_function(call->name());
    assert(decl_maybe.has_value());
    auto decl = decl_maybe.value();
    return make_node<MaterializedFunctionCall>(call, arguments, decl);
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
    auto decl_maybe = ctx.declared_function(call->name());
    assert(decl_maybe.has_value());
    auto decl = std::dynamic_pointer_cast<MaterializedNativeFunctionDecl>(decl_maybe.value());
    assert(decl != nullptr);
    return make_node<MaterializedNativeFunctionCall>(call, arguments, decl);
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
    auto decl_maybe = ctx.declared_function(call->name());
    std::shared_ptr<MaterializedIntrinsicDecl> materialized;
    if (!decl_maybe.has_value()) {
        materialized = TRY_AND_CAST(MaterializedIntrinsicDecl, process(call->declaration(), ctx));
        ctx.add_declared_function(call->name(), materialized);
        ctx.declare(call->name(), materialized);
    } else {
        materialized = std::dynamic_pointer_cast<MaterializedIntrinsicDecl>(decl_maybe.value());
        assert(materialized != nullptr);
    }
    return std::make_shared<MaterializedIntrinsicCall>(call, arguments, materialized, call->intrinsic());
}

NODE_PROCESSOR(BoundUnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, process(expr->operand(), ctx));

    IntrinsicType intrinsic;
    std::shared_ptr<BoundIntrinsicDecl> decl;
    switch (expr->op()) {
    case UnaryOperator::Dereference: {
        auto method_descr_maybe = operand->type()->get_method(Operator::Dereference);
        if (!method_descr_maybe.has_value())
            return SyntaxError { ErrorCode::InternalError, expr->token(), format("No method defined for unary operator {}::{}", operand->type()->to_string(), expr->op()) };
        auto method_descr = method_descr_maybe.value();
        decl = method_descr.declaration();
        intrinsic = IntrinsicType::dereference;
        break;
    }
    default: {
        auto method_descr_maybe = operand->type()->get_method(to_operator(expr->op()), {});
        if (!method_descr_maybe.has_value())
            return SyntaxError { ErrorCode::InternalError, expr->token(), format("No method defined for unary operator {}::{}", operand->type()->to_string(), expr->op()) };
        auto method_descr = method_descr_maybe.value();
        decl = method_descr.declaration();
        auto impl = method_descr.implementation(Architecture::MACOS_ARM64);
        if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
            return SyntaxError { ErrorCode::InternalError, expr->token(), format("No intrinsic defined for {}", method_descr.name()) };
        intrinsic = impl.intrinsic;
        break;
    }
    }
    return process(make_node<BoundIntrinsicCall>(expr->token(), decl, BoundExpressions { operand }, intrinsic), ctx);
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
        auto ident = make_node<BoundIdentifier>(Token {}, BinaryOperator_name(expr->op()), lhs->type());
        BoundIdentifiers params;
        params.push_back(make_node<BoundIdentifier>(Token {}, "ptr", lhs->type()));
        params.push_back(make_node<BoundIdentifier>(Token {}, "offset", ObjectType::get("s32")));
        auto decl = make_node<BoundIntrinsicDecl>(ident, params);
        return process(make_node<BoundIntrinsicCall>(expr->token(), decl, BoundExpressions { lhs, offset }, IntrinsicType::ptr_math), ctx);
    }

    auto method_descr_maybe = lhs->type()->get_method(to_operator(expr->op()), { rhs->type() });
    if (!method_descr_maybe.has_value())
        return SyntaxError { ErrorCode::InternalError, lhs->token(), format("No method defined for binary operator {}::{}({})", lhs->type()->to_string(), expr->op(), rhs->type()->to_string()) };
    auto method_descr = method_descr_maybe.value();
    auto impl = method_descr.implementation(Architecture::MACOS_ARM64);
    if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
        return SyntaxError { ErrorCode::InternalError, lhs->token(), format("No intrinsic defined for {}", method_descr.name()) };
    auto intrinsic = impl.intrinsic;
    return process(make_node<BoundIntrinsicCall>(expr->token(), method_descr.declaration(), BoundExpressions { lhs, rhs }, intrinsic), ctx);
}

std::shared_ptr<MaterializedIdentifier> make_materialized_identifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> const& address)
{
    switch (identifier->type()->type()) {
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::SignedIntegerNumber:
    case PrimitiveType::Pointer:
        return make_node<MaterializedIntIdentifier>(identifier, address);
    case PrimitiveType::Struct:
        return make_node<MaterializedStructIdentifier>(identifier, address);
    case PrimitiveType::Array:
        return make_node<MaterializedArrayIdentifier>(identifier, address);
    default:
        fatal("Cannot materialize identifiers of type '{}' yet", identifier->type());
    }
}

std::shared_ptr<MaterializedIdentifier> make_materialized_identifier(std::shared_ptr<MaterializedDeclaration> const& decl, std::shared_ptr<BoundIdentifier> const& identifier)
{
    return make_materialized_identifier(identifier, decl->address());
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
    auto materialized_member = make_materialized_identifier(member, std::make_shared<StructMemberAddress>(strukt->address(), offset));
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
