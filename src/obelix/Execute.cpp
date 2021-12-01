/*
 * Execute.cpp - Copyright (c) 2021 Jan de Visser <jan@finiandarcy.com>
 *
 * This file is part of obelix.
 *
 * obelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * obelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/Arguments.h>
#include <obelix/BoundFunction.h>
#include <obelix/Processor.h>

namespace Obelix {

extern_logging_category(parser);

ErrorOr<std::optional<Obj>> get_object(std::shared_ptr<Expression> const& expr, Context<Obj>& ctx)
{
    if (auto obj_maybe = TRY(expr->to_object()); obj_maybe.has_value()) {
        return obj_maybe.value();
    }
    auto processed = std::dynamic_pointer_cast<Expression>(TRY(process_tree(expr, ctx)));
    if (processed->node_type() == SyntaxNodeType::Literal)
        return std::dynamic_pointer_cast<Literal>(processed)->literal();
    if (auto obj_maybe = TRY(processed->to_object()); obj_maybe.has_value()) {
        return obj_maybe.value();
    }
    return std::optional<Obj> {};
}

#define OBJ(node, ctx)                                                                                     \
    ({                                                                                                     \
        std::shared_ptr<Literal> __literal;                                                                \
        if ((node)->node_type() != SyntaxNodeType::Literal) {                                              \
            auto __node_processed = TRY(process_node((node), ctx));                                        \
            if (__node_processed->node_type() != SyntaxNodeType::Literal)                                  \
                return Error(ErrorCode::SyntaxError, "Expression '" #node "' does not result in literal"); \
            __literal = std::dynamic_pointer_cast<Literal>(__node_processed);                              \
        } else {                                                                                           \
            __literal = std::dynamic_pointer_cast<Literal>((node));                                        \
        }                                                                                                  \
        auto __obj = __literal->literal();                                                                 \
        if (__obj->is_exception())                                                                         \
            return std::make_shared<Literal>(__obj);                                                       \
        __obj;                                                                                             \
    })

#define STMT_RESULT(expr)                                                                                                                                  \
    ({                                                                                                                                                     \
        auto __stmt_result = TRY((expr));                                                                                                                  \
        if (__stmt_result->node_type() != SyntaxNodeType::StatementExecutionResult)                                                                        \
            return Error { ErrorCode::SyntaxError, format("Statement '" #expr "' evaluated to a '{}'", SyntaxNodeType_name(__stmt_result->node_type())) }; \
        std::dynamic_pointer_cast<StatementExecutionResult>(__stmt_result);                                                                                \
    })

using ExecuteContext = Context<Obj>;
ExecuteContext::ProcessorMap execute_map;
ExecuteContext::ProcessorMap stmt_execute_map;

ErrorOrNode process_SyntaxNode(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    debug(parser, "Executing '{}' ({})", tree->to_string(0), SyntaxNodeType_name(tree->node_type()));
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

ErrorOrNode process_BinaryExpression(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
    if (expr->op().code() == TokenCode::Equals) {
        if (expr->lhs()->node_type() != SyntaxNodeType::Identifier)
            return Error(ErrorCode::SyntaxError, "Left hand side of assignment is not an lvalue ({} {})", SyntaxNodeType_name(expr->lhs()->node_type()), expr->lhs()->to_string(0));
        auto identifier = std::dynamic_pointer_cast<Identifier>(expr->lhs());
        auto rhs_obj = OBJ(expr->rhs(), ctx);
        ctx.set(identifier->name(), rhs_obj);
        return std::make_shared<Literal>(rhs_obj);
    }

    auto right = OBJ(expr->rhs(), ctx);
    auto left = OBJ(expr->lhs(), ctx);
    auto ret_maybe = left.evaluate(expr->op().value(), right);
    if (!ret_maybe.has_value())
        return Error { ErrorCode::OperatorUnresolved, expr->op().value(), left };
    return std::make_shared<Literal>(ret_maybe.value());
};

ErrorOrNode process_Identifier(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto ident = std::dynamic_pointer_cast<Identifier>(tree);
    if (auto value_maybe = ctx.get(ident->name()); value_maybe.has_value())
        return std::make_shared<Literal>(value_maybe.value());
    return tree;
};

ErrorOrNode process_UnaryExpression(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
    auto operand = OBJ(expr->operand(), ctx);
    auto ret_maybe = operand->evaluate(expr->op().value());
    if (!ret_maybe.has_value())
        return Error { ErrorCode::OperatorUnresolved, expr->op().value(), operand };
    return std::make_shared<Literal>(ret_maybe.value());
};

ErrorOrNode process_FunctionCall(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx)
{
    auto function_call = std::dynamic_pointer_cast<FunctionCall>(tree);
    auto callable = OBJ(function_call->function(), ctx);
    auto args = make_typed<Arguments>();
    for (auto& arg : function_call->arguments()) {
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

ErrorOrNode execute(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& root)
{
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
    stmt_execute_map[SyntaxNodeType::BinaryExpression] = process_BinaryExpression;
    stmt_execute_map[SyntaxNodeType::Identifier] = process_Identifier;
    stmt_execute_map[SyntaxNodeType::UnaryExpression] = process_UnaryExpression;
    stmt_execute_map[SyntaxNodeType::FunctionCall] = process_FunctionCall;

    execute_map[SyntaxNodeType::Block] = process_Block;
    stmt_execute_map[SyntaxNodeType::Block] = process_Block;
    execute_map[SyntaxNodeType::Module] = process_Block;

    Context<Obj> ctx(&root, execute_map);
    debug(parser, "Executing '{}'", tree->to_string(0));
    return process_node(tree, ctx);
}

ErrorOrNode execute(std::shared_ptr<SyntaxNode> const& tree)
{
    Context<Obj> root;
    return execute(tree, root);
}

}
