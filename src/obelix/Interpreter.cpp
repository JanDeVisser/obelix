/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "obelix/Operator.h"
#include <core/Arguments.h>
#include <memory>
#include <obelix/BoundFunction.h>
#include <obelix/Interpreter.h>
#include <obelix/Intrinsics.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

#define STMT_RESULT(expr)                                                                                                                                  \
    ({                                                                                                                                                     \
        auto __stmt_result = TRY((expr));                                                                                                                  \
        if (__stmt_result->node_type() != SyntaxNodeType::StatementExecutionResult)                                                                        \
            return Error { ErrorCode::SyntaxError, format("Statement '" #expr "' evaluated to a '{}'", SyntaxNodeType_name(__stmt_result->node_type())) }; \
        std::dynamic_pointer_cast<StatementExecutionResult>(__stmt_result);                                                                                \
    })

using ProcessorMap = std::unordered_map<SyntaxNodeType,std::function<ErrorOrNode(std::shared_ptr<SyntaxNode>,InterpreterContext&)>>;
ProcessorMap stmt_execute_map;

ErrorOrNode interpreter_processor(std::shared_ptr<SyntaxNode> const&, InterpreterContext&);

using FunctionType = std::function<ErrorOr<void>(InterpreterContext&)>;
static std::array<FunctionType, IntrinsicType::count> s_intrinsics = {};

bool register_interpreter_intrinsic(IntrinsicType type, FunctionType intrinsic)
{
    s_intrinsics[type] = intrinsic;
    return true;
}

#define INTRINSIC(intrinsic)                                                                                              \
    ErrorOr<void> interpreter_intrinsic_##intrinsic(InterpreterContext& ctx);                                                 \
    auto s_interpreter_##intrinsic##_decl = register_interpreter_intrinsic(intrinsic, interpreter_intrinsic_##intrinsic); \
    ErrorOr<void> interpreter_intrinsic_##intrinsic(InterpreterContext& ctx)

INTRINSIC(add_int_int)
{
    ctx.set_return_value(
        std::make_shared<BoundLiteral>(ctx.arguments()[0]->token(),
            ctx.arguments()[0]->int_value() + ctx.arguments()[1]->int_value()));
    return {};
}

InterpreterContext::InterpreterContext(InterpreterContext& parent)
    : Context<int>(parent)
{
}

InterpreterContext::InterpreterContext(InterpreterContext* parent)
    : Context<int>(parent)
{
}

InterpreterContext::InterpreterContext()
    : Context()
{
}

#if 0

ErrorOrNode process_SyntaxNode(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    debug(parser, "Executing '{}' ({})", tree->to_string(), SyntaxNodeType_name(tree->node_type()));
    return tree;
};

ErrorOrNode process_Statement(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    debug(parser, "Returning '{}' ({})", tree->to_string(0), SyntaxNodeType_name(tree->node_type()));
    return tree;
};

ErrorOrNode process_Pass(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    return execution_ok();
};

ErrorOrNode process_Import(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    fatal(format("SyntaxNodeType::{} should have been elided in earlier stages", SyntaxNodeType_name(tree->node_type())).c_str());
};

ErrorOrNode process_FunctionDef(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto function_def = std::dynamic_pointer_cast<FunctionDef>(tree);
    auto bound_function = make_obj<BoundFunction>(ctx, *function_def);
    ctx.declare(function_def->name(), bound_function);
    return execution_evaluates_to(bound_function);
};

ErrorOrNode process_NativeFunctionDef(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto function_def = std::dynamic_pointer_cast<NativeFunctionDef>(tree);
    auto native_function = make_obj<NativeFunction>(function_def->native_function_name());
    ctx.declare(function_def->name(), native_function);
    return execution_evaluates_to(native_function);
};

ErrorOrNode process_VariableDeclaration(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
    if (ctx.contains(var_decl->variable().identifier()))
        return Error { ErrorCode::VariableAlreadyDeclared, var_decl->variable().identifier() };
    Obj value = Obj::null();
    if (var_decl->expression())
        value = OBJ(var_decl->expression(), ctx);
    ctx.declare(var_decl->variable().identifier(), value);
    return execution_evaluates_to(value);
};

ErrorOrNode process_Return(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
    Obj value = Obj::null();
    if (return_stmt->expression())
        value = OBJ(return_stmt->expression(), ctx);
    return return_result(value);
};

ErrorOrNode process_Continue(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    return continue_loop();
};

ErrorOrNode process_Break(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    return continue_loop();
};

ErrorOrNode process_Branch(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto branch = std::dynamic_pointer_cast<Branch>(tree);
    Obj condition_result = Obj::True();
    if (branch->condition()) {
        condition_result = OBJ(branch->condition(), ctx);
    }
    if (condition_result) {
        auto result_maybe = TRY(process_node(branch->statement(), ctx));
        if (result_maybe->node_type() != SyntaxNodeType::StatementExecutionResult)
            return Error { ErrorCode::SyntaxError, "Branch statement did not evaluate to result" };
        return result_maybe;
    }
    return skip_block();
};

ErrorOrNode process_IfStatement(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
    auto result = STMT_RESULT(process_Branch(tree, ctx));
    if (result->flow_control() != FlowControl::Skipped)
        return result;
    for (auto& elif : if_stmt->elifs()) {
        result = STMT_RESULT(process_Branch(elif, ctx));
        if (result->flow_control() != FlowControl::Skipped)
            return result;
    }
    if (if_stmt->else_stmt()) {
        return STMT_RESULT(process_node(if_stmt->else_stmt(), ctx));
    }
    return execution_ok();
};

ErrorOrNode process_Label(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto label_stmt = std::dynamic_pointer_cast<Label>(tree);
    return mark_label(label_stmt->label_id());
};

ErrorOrNode process_Goto(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto goto_stmt = std::dynamic_pointer_cast<Goto>(tree);
    return goto_label(goto_stmt->label_id());
};

ErrorOrNode process_SwitchStatement(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
    for (auto& case_stmt : switch_stmt->cases()) {
        auto result = STMT_RESULT(process_node(case_stmt, ctx));
        if (result->flow_control() != FlowControl::Skipped)
            return result;
    }
    if (switch_stmt->default_case())
        return STMT_RESULT(process_node(switch_stmt->default_case(), ctx));
    return execution_ok();
};

ErrorOrNode process_ExpressionStatement(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
    auto obj = OBJ(stmt->expression(), ctx);
    return execution_evaluates_to(obj);
};

ErrorOrNode process_TypedExpression(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto typed_expression = std::dynamic_pointer_cast<TypedExpression>(tree);
    auto ret = OBJ(typed_expression->expression(), ctx);
    return std::make_shared<Literal>(ret);
};

ErrorOrNode process_FunctionCall(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto function_call = std::dynamic_pointer_cast<FunctionCall>(tree);
    auto callable = OBJ(function_call->function(), ctx);
    auto args = make_typed<Arguments>();
    for (auto& arg : function_call->arguments()->arguments()) {
        auto evaluated_arg = OBJ(arg, ctx);
        args->add(evaluated_arg);
    }
    return std::make_shared<Literal>(callable->call(args));
};

ErrorOrNode process_Block(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    debug(parser, "Executing block '{}'", tree->to_string(0));
    auto block = std::dynamic_pointer_cast<Block>(tree);
    Context child_ctx(&ctx, stmt_execute_map);
    std::shared_ptr<StatementExecutionResult> result;
    auto labels = std::unordered_map<int, int>();
    int ix = 0;
    while (ix < block->statements().size()) {
        auto stmt = block->statements().at(ix);
        result = STMT_RESULT(process_node(stmt, child_ctx));
        switch (result->flow_control()) {
        case FlowControl::None:
            ix++;
            break;
        case FlowControl::Label:
            labels.insert_or_assign((int)result->result()->to_long().value(), ix);
            ix++;
            break;
        case FlowControl::Goto: {
            int label_id = (int)result->result()->to_long().value();
            if (labels.contains(label_id)) {
                ix = labels.at(label_id);
                break;
            }
            while (ix < block->statements().size()) {
                auto s = block->statements().at(ix);
                if (s->node_type() == SyntaxNodeType::Label) {
                    auto label = std::dynamic_pointer_cast<Label>(s);
                    if (label->label_id() == label_id) {
                        labels.insert_or_assign(label_id, ix);
                        break;
                    }
                }
                ix++;
            }
            if (ix >= block->statements().size())
                return result;
            break;
        }
        default:
            return result;
        }
    }
    return result;
};

ErrorOrNode process_Identifier(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto ident = std::dynamic_pointer_cast<Identifier>(tree);
    if (auto value_maybe = ctx.get(ident->name()); value_maybe.has_value())
        return std::make_shared<Literal>(value_maybe.value());
    return tree;
};

#endif

ErrorOrNode process_BoundBinaryExpression(std::shared_ptr<SyntaxNode> const& tree, InterpreterContext& ctx)
{
    auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);
    auto lhs = TRY_AND_CAST(BoundExpression, interpreter_processor(expr->lhs(), ctx));
    auto rhs = TRY_AND_CAST(BoundExpression, interpreter_processor(expr->rhs(), ctx));

    if (lhs->type()->type() == PrimitiveType::Pointer && (expr->op() == BinaryOperator::Add || expr->op() == BinaryOperator::Subtract))
        return tree;
    if (BinaryOperator_is_assignment(expr->op()))
        return tree;

    auto method_def_maybe = lhs->type()->get_method(to_operator(expr->op()), { rhs->type() });
    if (!method_def_maybe.has_value())
        return tree;
    auto impl = method_def_maybe.value().implementation(Architecture::INTERPRETER);
    if (!impl.is_intrinsic || impl.intrinsic != IntrinsicType::NotIntrinsic || s_intrinsics[impl.intrinsic] == nullptr)
        return tree;

    ctx.reset();
    ctx.add_argument(std::dynamic_pointer_cast<BoundLiteral>(lhs));
    ctx.add_argument(std::dynamic_pointer_cast<BoundLiteral>(rhs));
    
    auto func = s_intrinsics[impl.intrinsic];
    if (auto err = func(ctx); err.is_error())
        return err.error();
    return ctx.return_value();
};

ErrorOrNode process_BoundUnaryExpression(std::shared_ptr<SyntaxNode> const& tree, InterpreterContext& ctx)
{
    auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, interpreter_processor(expr->operand(), ctx));

    if (operand->type()->type() == PrimitiveType::Pointer)
        return tree;

    auto method_def_maybe = operand->type()->get_method(to_operator(expr->op()), {});
    if (!method_def_maybe.has_value())
        return tree;
    auto impl = method_def_maybe.value().implementation(Architecture::INTERPRETER);
    if (!impl.is_intrinsic || impl.intrinsic != IntrinsicType::NotIntrinsic || s_intrinsics[impl.intrinsic] == nullptr)
        return tree;

    ctx.reset();
    ctx.add_argument(std::dynamic_pointer_cast<BoundLiteral>(operand));
    
    auto func = s_intrinsics[impl.intrinsic];
    if (auto err = func(ctx); err.is_error())
        return err.error();
    return ctx.return_value();
};



[[maybe_unused]] auto dummy = []() {
    stmt_execute_map[SyntaxNodeType::BoundBinaryExpression] = process_BoundBinaryExpression;
    stmt_execute_map[SyntaxNodeType::BoundUnaryExpression] = process_BoundUnaryExpression;

#if 0
    stmt_execute_map[SyntaxNodeType::SyntaxNode] = process_SyntaxNode;
    stmt_execute_map[SyntaxNodeType::Statement] = process_Statement;
    stmt_execute_map[SyntaxNodeType::Pass] = process_Pass;
    stmt_execute_map[SyntaxNodeType::Import] = process_Import;
    stmt_execute_map[SyntaxNodeType::FunctionDef] = process_FunctionDef;
    stmt_execute_map[SyntaxNodeType::NativeFunctionDef] = process_NativeFunctionDef;
    stmt_execute_map[SyntaxNodeType::VariableDeclaration] = process_VariableDeclaration;
    stmt_execute_map[SyntaxNodeType::Return] = process_Return;
    stmt_execute_map[SyntaxNodeType::Continue] = process_Continue;
    stmt_execute_map[SyntaxNodeType::Break] = process_Break;
    stmt_execute_map[SyntaxNodeType::Branch] = process_Branch;
    stmt_execute_map[SyntaxNodeType::ElseStatement] = process_Branch;
    stmt_execute_map[SyntaxNodeType::ElifStatement] = process_Branch;
    stmt_execute_map[SyntaxNodeType::CaseStatement] = process_Branch;
    stmt_execute_map[SyntaxNodeType::DefaultCase] = process_Branch;
    stmt_execute_map[SyntaxNodeType::IfStatement] = process_IfStatement;
    stmt_execute_map[SyntaxNodeType::Label] = process_Label;
    stmt_execute_map[SyntaxNodeType::Goto] = process_Goto;
    stmt_execute_map[SyntaxNodeType::SwitchStatement] = process_SwitchStatement;
    stmt_execute_map[SyntaxNodeType::ExpressionStatement] = process_ExpressionStatement;
    stmt_execute_map[SyntaxNodeType::TypedExpression] = process_TypedExpression;
    stmt_execute_map[SyntaxNodeType::Identifier] = process_Identifier;
    stmt_execute_map[SyntaxNodeType::FunctionCall] = process_FunctionCall;

    execute_map[SyntaxNodeType::Block] = process_Block;
    stmt_execute_map[SyntaxNodeType::Block] = process_Block;
    execute_map[SyntaxNodeType::Module] = process_Block;
#endif
    return true;
};

ErrorOrNode interpreter_processor(std::shared_ptr<SyntaxNode> const& tree, InterpreterContext& ctx)
{
    if (stmt_execute_map.contains(tree->node_type()))
        return stmt_execute_map[tree->node_type()](tree, ctx);
    return process_tree(tree, ctx, interpreter_processor);    
}

ErrorOrNode interpret(std::shared_ptr<SyntaxNode> const& tree)
{
    InterpreterContext root;
    return interpreter_processor(tree, root);
}

}
