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

ErrorOr<std::optional<Obj>> get_object(std::shared_ptr<Expression> const &expr, Context<Obj>& ctx)
{
    if (auto obj_maybe = TRY(expr->to_object());obj_maybe.has_value()) {
        return obj_maybe.value();
    }
    auto processed = std::dynamic_pointer_cast<Expression>(TRY(process_tree(expr, ctx)));
    if (processed->node_type() == SyntaxNodeType::Literal)
        return std::dynamic_pointer_cast<Literal>(expr)->literal();
    if (auto obj_maybe = TRY(processed->to_object()); obj_maybe.has_value()) {
        return obj_maybe.value();
    }
    return std::optional<Obj> {};
}

#define OBJ(node, ctx)                                                                   \
    ({                                                                                   \
        auto __obj_maybe = TRY(get_object((node), ctx));                                 \
        if (!__obj_maybe.has_value())                                                    \
            return Error(ErrorCode::SyntaxError, "Expression does not result in value"); \
        auto __obj = __obj_maybe.value();                                                \
        if (__obj->is_exception())                                                       \
            return std::make_shared<Literal>(__obj);                                     \
        __obj;                                                                           \
    })

#define STMT_RESULT(expr)                                                                    \
    ({                                                                                       \
        auto __stmt_result = TRY((expr));                                                    \
        if (__stmt_result->node_type() != SyntaxNodeType::StatementExecutionResult)          \
            return Error { ErrorCode::SyntaxError, "Statement did not evaluate to result" }; \
        std::dynamic_pointer_cast<StatementExecutionResult>(__stmt_result);                  \
    })

ErrorOrNode execute(std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& root)
{
    using ExecuteContext = Context<Obj>;
    ExecuteContext::ProcessorMap execute_map;
    ExecuteContext::ProcessorMap stmt_execute_map;

    stmt_execute_map[SyntaxNodeType::Pass] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return execution_ok();
    };

    stmt_execute_map[SyntaxNodeType::Import] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        fatal("SyntaxNodeType::Import should have been elided in earlier stages");
    };

    stmt_execute_map[SyntaxNodeType::FunctionDef] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto function_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto bound_function = make_obj<BoundFunction>(ctx, *function_def);
        ctx.declare(function_def->name(), bound_function);
        return execution_evaluates_to(bound_function);
    };

    stmt_execute_map[SyntaxNodeType::NativeFunctionDef] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto function_def = std::dynamic_pointer_cast<NativeFunctionDef>(tree);
        auto native_function = make_obj<NativeFunction>(function_def->native_function_name());
        ctx.declare(function_def->name(), native_function);
        return execution_evaluates_to(native_function);
    };

    stmt_execute_map[SyntaxNodeType::VariableDeclaration] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        if (ctx.contains(var_decl->variable().identifier()))
            return Error { ErrorCode::VariableAlreadyDeclared, var_decl->variable().identifier() };
        Obj value = Obj::null();
        if (var_decl->expression())
            value = OBJ(var_decl->expression(), ctx);
        ctx.declare(var_decl->variable().identifier(), value);
        return execution_evaluates_to(value);
    };

    stmt_execute_map[SyntaxNodeType::Return] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
        Obj value = Obj::null();
        if (return_stmt->expression())
            value = OBJ(return_stmt->expression(), ctx);
        return return_result(value);
    };

    stmt_execute_map[SyntaxNodeType::Continue] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return continue_loop();
    };

    stmt_execute_map[SyntaxNodeType::Break] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        return continue_loop();
    };

    auto branch_handler = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto branch = std::dynamic_pointer_cast<Branch>(tree);
        Obj condition_result = Obj::True();
        if (branch->condition()) {
            condition_result = OBJ(branch->condition(), ctx);
        }
        if (condition_result) {
            auto result_maybe = TRY(process_tree(branch->statement(), ctx));
            if (result_maybe->node_type() != SyntaxNodeType::StatementExecutionResult)
                return Error { ErrorCode::SyntaxError, "Statement did not evaluate to result" };
            return result_maybe;
        }
        return skip_block();
    };

    stmt_execute_map[SyntaxNodeType::Branch] = branch_handler;
    stmt_execute_map[SyntaxNodeType::ElseStatement] = stmt_execute_map[SyntaxNodeType::Branch];
    stmt_execute_map[SyntaxNodeType::ElifStatement] = stmt_execute_map[SyntaxNodeType::Branch];
    stmt_execute_map[SyntaxNodeType::CaseStatement] = stmt_execute_map[SyntaxNodeType::Branch];
    stmt_execute_map[SyntaxNodeType::DefaultCase] = stmt_execute_map[SyntaxNodeType::Branch];

    stmt_execute_map[SyntaxNodeType::IfStatement] = [branch_handler](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        auto result = STMT_RESULT(branch_handler(tree, ctx));
        if (result->flow_control() != FlowControl::Skipped)
            return result;
        for (auto& elif : if_stmt->elifs()) {
            result = STMT_RESULT(branch_handler(elif, ctx));
            if (result->flow_control() != FlowControl::Skipped)
                return result;
        }
        if (if_stmt->else_stmt()) {
            return STMT_RESULT(process_tree(if_stmt->else_stmt(), ctx));
        }
        return execution_ok();
    };

    stmt_execute_map[SyntaxNodeType::WhileStatement] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        do {
            Obj condition_result = Obj::True();
            if (while_stmt->condition()) {
                condition_result = OBJ(while_stmt->condition(), ctx);
            }
            if (!condition_result)
                return break_loop();
            if (condition_result) {
                auto result = STMT_RESULT(process_tree(while_stmt->statement(), ctx));
                if ((result->flow_control() == FlowControl::Break) || (result->flow_control() == FlowControl::Return))
                    return result;
            }
        } while (true);
    };

    stmt_execute_map[SyntaxNodeType::SwitchStatement] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
        for (auto& case_stmt : switch_stmt->cases()) {
            auto result = STMT_RESULT(process_tree(case_stmt, ctx));
            if (result->flow_control() != FlowControl::Skipped)
                return result;
        }
        if (switch_stmt->default_case())
            return STMT_RESULT(process_tree(switch_stmt->default_case(), ctx));
        return execution_ok();
    };

    stmt_execute_map[SyntaxNodeType::ExpressionStatement] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        auto obj = OBJ(stmt->expression(), ctx);
        return execution_evaluates_to(obj);
    };

    execute_map[SyntaxNodeType::Block] = [stmt_execute_map](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto block = std::dynamic_pointer_cast<Block>(tree);
        Context child_ctx(&ctx, stmt_execute_map);
        std::shared_ptr<StatementExecutionResult> result;
        for (auto& stmt : block->statements()) {
            result = STMT_RESULT(process_tree(stmt, child_ctx));
            if (result->flow_control() != FlowControl::None)
                return result;
        }
        return result;
    };

    stmt_execute_map[SyntaxNodeType::Block] = execute_map[SyntaxNodeType::Block];
    execute_map[SyntaxNodeType::Module] = execute_map[SyntaxNodeType::Block];

    execute_map[SyntaxNodeType::TypedExpression] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto typed_expression = std::dynamic_pointer_cast<TypedExpression>(tree);
        auto ret = OBJ(typed_expression->expression(), ctx);
        return std::make_shared<Literal>(ret);
    };

    execute_map[SyntaxNodeType::BinaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
        if (expr->op().code() == TokenCode::Equals) {
            auto lhs = TRY(process_tree(expr->lhs(), ctx));
            if (lhs->node_type() != SyntaxNodeType::Identifier)
                return Error(ErrorCode::SyntaxError, "Left hand side of assignment is not an lvalue");
            auto identifier = std::dynamic_pointer_cast<Identifier>(lhs);
            auto rhs_obj = OBJ(expr->rhs(), ctx);
            ctx.set(identifier->name(), rhs_obj);
            return std::make_shared<Literal>(rhs_obj);
        }
        auto ret = OBJ(expr, ctx);
        return std::make_shared<Literal>(ret);
    };

    execute_map[SyntaxNodeType::UnaryExpression] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto ret = OBJ(expr, ctx);
        return std::make_shared<Literal>(ret);
    };

    execute_map[SyntaxNodeType::FunctionCall] = [](std::shared_ptr<SyntaxNode> const& tree, Context<Obj>& ctx) -> ErrorOrNode {
        auto function_call = std::dynamic_pointer_cast<FunctionCall>(tree);
        auto callable = OBJ(function_call->function(), ctx);
        auto args = make_typed<Arguments>();
        for (auto& arg : function_call->arguments()) {
            auto evaluated_arg = OBJ(arg, ctx);
            args->add(evaluated_arg);
        }
        return std::make_shared<Literal>(callable->call(args));
    };

    Context<Obj> ctx(&root, execute_map);
    return process_tree(tree, ctx);
}

ErrorOrNode execute(std::shared_ptr<SyntaxNode> const& tree)
{
    Context<Obj> root;
    return execute(tree, root);
}

}
