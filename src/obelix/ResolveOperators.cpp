/*
 * Copyright (c) 2022, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <obelix/Syntax.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

using ResolveOperatorContext = Context<bool>;

INIT_NODE_PROCESSOR(ResolveOperatorContext);

NODE_PROCESSOR(BoundUnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, expr->operand(), ctx);

    IntrinsicType intrinsic;
    std::shared_ptr<BoundIntrinsicDecl> decl;
    switch (expr->op()) {
    case UnaryOperator::Identity: {
        return operand;
    }
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
        auto impl = method_descr.implementation(Architecture::C_TRANSPILER);
        if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
            return SyntaxError { ErrorCode::InternalError, expr->token(), format("No intrinsic defined for {}", method_descr.name()) };
        intrinsic = impl.intrinsic;
        break;
    }
    }
    return TRY(process(make_node<BoundIntrinsicCall>(expr->token(), decl, BoundExpressions { operand }, intrinsic), ctx, result));
}

NODE_PROCESSOR(BoundBinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);

    if (expr->op() == BinaryOperator::Range)
        return tree;
    auto lhs = TRY_AND_CAST(BoundExpression, expr->lhs(), ctx);
    auto rhs = TRY_AND_CAST(BoundExpression, expr->rhs(), ctx);

    if (lhs->type()->type() == PrimitiveType::Pointer && (expr->op() == BinaryOperator::Add || expr->op() == BinaryOperator::Subtract)) {
        std::shared_ptr<BoundExpression> offset { nullptr };
        auto target_type = (lhs->type()->is_template_specialization()) ? lhs->type()->template_argument<std::shared_ptr<ObjectType>>("target") : get_type<uint8_t>();

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
            offset = TRY_AND_CAST(BoundExpression, make_node<BoundBinaryExpression>(expr->token(), size, BinaryOperator::Multiply, offset, ObjectType::get("u64")), ctx);
        }
        auto ident = make_node<BoundIdentifier>(Token {}, BinaryOperator_name(expr->op()), lhs->type());
        BoundIdentifiers params;
        params.push_back(make_node<BoundIdentifier>(Token {}, "ptr", lhs->type()));
        params.push_back(make_node<BoundIdentifier>(Token {}, "offset", ObjectType::get("s32")));
        auto decl = make_node<BoundIntrinsicDecl>(ident, params);
        return TRY(process(make_node<BoundIntrinsicCall>(expr->token(), decl, BoundExpressions { lhs, offset }, IntrinsicType::ptr_math), ctx, result));
    }

    if ((rhs->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs->type()->type() == lhs->type()->type()) && (rhs->type()->size() > lhs->type()->size()))
        rhs = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(rhs)->cast(lhs->type()));
    if ((lhs->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs->type()->type() == lhs->type()->type()) && (lhs->type()->size() > rhs->type()->size()))
        lhs = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(rhs)->cast(rhs->type()));
    auto method_descr_maybe = lhs->type()->get_method(to_operator(expr->op()), { rhs->type() });
    if (!method_descr_maybe.has_value())
        return SyntaxError { ErrorCode::InternalError, lhs->token(), format("No method defined for binary operator {}::{}({})", lhs->type()->to_string(), expr->op(), rhs->type()->to_string()) };
    auto method_descr = method_descr_maybe.value();
    auto impl = method_descr.implementation();
    if (!impl.is_intrinsic || impl.intrinsic == IntrinsicType::NotIntrinsic)
        return SyntaxError { ErrorCode::InternalError, lhs->token(), format("No intrinsic defined for {}", method_descr.name()) };
    auto intrinsic = impl.intrinsic;
    return TRY(process(make_node<BoundIntrinsicCall>(expr->token(), method_descr.declaration(), BoundExpressions { lhs, rhs }, intrinsic), ctx));
}

ProcessResult resolve_operators(std::shared_ptr<SyntaxNode> const& tree)
{
    Config config;
    ResolveOperatorContext ctx(config);
    return process<ResolveOperatorContext>(tree, ctx);
}

}
