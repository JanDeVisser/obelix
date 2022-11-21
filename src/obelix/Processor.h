/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>

#include <core/Error.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Config.h>
#include <obelix/Context.h>
#include <obelix/Syntax.h>
#include <obelix/arm64/MaterializedSyntaxNode.h>

namespace Obelix {

extern_logging_category(processor);

using ErrorOrNode = ErrorOr<std::shared_ptr<SyntaxNode>, SyntaxError>;

template<class NodeClass>
using ErrorOrTypedNode = ErrorOr<std::shared_ptr<NodeClass>, SyntaxError>;

using Errors = std::vector<SyntaxError>;

class ProcessResult {
public:
    ProcessResult() = default;
    ProcessResult(ProcessResult const& other) = default;

    ProcessResult(pSyntaxNode const& node)
        : m_result(node)
    {
    }

    ProcessResult(SyntaxError const& error)
        : m_result(error)
    {
        m_errors.push_back(m_result.error());
    }

    ProcessResult(SyntaxError&& error)
        : m_result(error)
    {
        m_errors.push_back(m_result.error());
    }

    [[nodiscard]] Errors const& errors() const { return m_errors; }
    void push_back(SyntaxError const& err) { m_errors.push_back(err); }
    [[nodiscard]] bool is_error() const { return m_result.is_error(); }
    [[nodiscard]] bool has_value() const { return m_result.has_value(); }
    [[nodiscard]] pSyntaxNode value() const { return m_result.value(); }
    void value(pSyntaxNode const& node) { m_result = node; }
    [[nodiscard]] SyntaxError error() const { return m_result.error(); }
    void error(SyntaxError const& error)
    {
        m_errors.push_back(error);
        m_result = error;
    }

    ProcessResult& operator=(pSyntaxNode const& result)
    {
        value(result);
        return *this;
    }

    ProcessResult& operator=(SyntaxError const& err)
    {
        error(err);
        return *this;
    }

    ProcessResult& operator+=(ProcessResult const& other)
    {
        m_result = other.m_result;
        for (auto const& e : other.m_errors)
            m_errors.push_back(e);
        return *this;
    }

private:
    ErrorOrNode m_result { nullptr };
    Errors m_errors {};
};

#define TRY_AND_TRY_CAST(cls, expr, ctx)                          \
    ({                                                            \
        auto __casted = try_and_try_cast<cls>(expr, ctx, result); \
        if (__casted.is_error())                                  \
            return __casted.error();                              \
        std::shared_ptr<cls> __ret = __casted.value();            \
        __ret;                                                    \
    })

#define TRY_AND_CAST(cls, expr, ctx)                          \
    ({                                                        \
        auto __casted = try_and_cast<cls>(expr, ctx, result); \
        if (__casted.is_error())                              \
            return __casted.error();                          \
        std::shared_ptr<cls> __ret = __casted.value();        \
        __ret;                                                \
    })

#define TRY_AND_TRY_CAST_RETURN(cls, expr, ctx, return_value)     \
    ({                                                            \
        auto __casted = try_and_try_cast<cls>(expr, ctx, result); \
        if (__casted.is_error())                                  \
            return __casted.error();                              \
        if (__casted.value() == nullptr)                          \
            return return_value;                                  \
        std::shared_ptr<cls> __ret = __casted.value();            \
        __ret;                                                    \
    })

template<typename Context, typename Processor>
ErrorOrNode process_tree(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, ProcessResult& result, Processor processor)
{
    if (!tree)
        return tree;

    std::shared_ptr<SyntaxNode> ret = tree;

    switch (tree->node_type()) {

    case SyntaxNodeType::BoundType:
        return tree;

    case SyntaxNodeType::Compilation: {
        auto compilation = std::dynamic_pointer_cast<Compilation>(tree);
        Modules modules;
        for (auto& module : compilation->modules()) {
            modules.push_back(TRY_AND_CAST(Module, module, ctx));
        }
        ret = make_node<Compilation>(modules, compilation->main_module());
        break;
    }

    case SyntaxNodeType::BoundCompilation: {
        auto compilation = std::dynamic_pointer_cast<BoundCompilation>(tree);

        BoundModules modules;
        for (auto& module : compilation->modules()) {
            modules.push_back(TRY_AND_CAST(BoundModule, module, ctx));
        }
        BoundTypes types;
        for (auto& type : compilation->custom_types()) {
            types.push_back(TRY_AND_CAST(BoundType, type, ctx));
        }
        ret = std::make_shared<BoundCompilation>(modules, types, compilation->main_module());
        break;
    }

    case SyntaxNodeType::Module: {
        auto module = std::dynamic_pointer_cast<Module>(tree);
        ret = TRY(process_block<Module>(tree, ctx, result, processor, module->name()));
        break;
    }

    case SyntaxNodeType::Block: {
        ret = TRY(process_block<Block>(tree, ctx, result, processor));
        break;
    }

    case SyntaxNodeType::FunctionBlock: {
        ret = TRY(process_block<FunctionBlock>(tree, ctx, result, processor));
        break;
    }

    case SyntaxNodeType::ExpressionType: {
        auto expr_type = std::dynamic_pointer_cast<ExpressionType>(tree);
        TemplateArgumentNodes arguments;
        for (auto const& arg : expr_type->template_arguments()) {
            auto processed_arg = TRY_AND_CAST(ExpressionType, arg, ctx);
            arguments.push_back(arg);
        }
        return std::make_shared<ExpressionType>(expr_type->token(), expr_type->type_name(), arguments);
    }

    case SyntaxNodeType::StructDefinition: {
        auto struct_def = std::dynamic_pointer_cast<StructDefinition>(tree);
        Identifiers fields;
        for (auto const& field : struct_def->fields()) {
            auto processed_field = TRY_AND_CAST(Identifier, field, ctx);
            fields.push_back(processed_field);
        }
        return std::make_shared<StructDefinition>(struct_def->token(), struct_def->name(), fields);
    }

    case SyntaxNodeType::EnumDef: {
        auto enum_def = std::dynamic_pointer_cast<EnumDef>(tree);
        EnumValues values;
        for (auto const& value : enum_def->values()) {
            auto processed_value = TRY_AND_CAST(EnumValue, value, ctx);
            values.push_back(processed_value);
        }
        return std::make_shared<EnumDef>(enum_def->token(), enum_def->name(), values, enum_def->extend());
    }

    case SyntaxNodeType::BoundEnumDef: {
        auto enum_def = std::dynamic_pointer_cast<BoundEnumDef>(tree);
        BoundEnumValueDefs values;
        for (auto const& value : enum_def->values()) {
            auto processed_value = TRY_AND_CAST(BoundEnumValueDef, value, ctx);
            values.push_back(processed_value);
        }
        return std::make_shared<BoundEnumDef>(enum_def->token(), enum_def->name(), enum_def->type(), values, enum_def->extend());
    }

    case SyntaxNodeType::TypeDef: {
        auto type_def = std::dynamic_pointer_cast<TypeDef>(tree);
        auto type = TRY_AND_CAST(ExpressionType, type_def, ctx);
        return std::make_shared<TypeDef>(type_def->token(), type_def->name(), type);
    }

    case SyntaxNodeType::BoundTypeDef: {
        auto type_def = std::dynamic_pointer_cast<BoundTypeDef>(tree);
        auto type = TRY_AND_CAST(BoundType, type_def->type(), ctx);
        return std::make_shared<BoundTypeDef>(type_def->token(), type_def->name(), type);
    }

    case SyntaxNodeType::FunctionDef: {
        auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
        auto func_decl = TRY_AND_CAST(FunctionDecl, func_def->declaration(), ctx);
        auto statement = func_def->statement();
        if (statement)
            statement = TRY_AND_CAST(Statement, statement, ctx);
        ret = std::make_shared<FunctionDef>(func_def->token(), func_decl, statement);
        break;
    }

    case SyntaxNodeType::BoundFunctionDef: {
        auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
        auto func_decl = TRY_AND_CAST(BoundFunctionDecl, func_def->declaration(), ctx);
        auto statement = func_def->statement();
        if (statement)
            statement = TRY_AND_CAST(Statement, statement, ctx);
        ret = std::make_shared<BoundFunctionDef>(func_def->token(), func_decl, statement);
        break;
    }

    case SyntaxNodeType::FunctionDecl:
    case SyntaxNodeType::NativeFunctionDecl:
    case SyntaxNodeType::IntrinsicDecl: {
        auto func_decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
        auto identifier = TRY_AND_CAST(Identifier, func_decl->identifier(), ctx);
        Identifiers parameters;
        for (auto& param : func_decl->parameters()) {
            auto processed_param = TRY_AND_CAST(Identifier, param, ctx);
            parameters.push_back(processed_param);
        }
        switch (func_decl->node_type()) {
        case SyntaxNodeType::FunctionDecl:
            ret = std::make_shared<FunctionDecl>(func_decl->token(), identifier, parameters);
            break;
        case SyntaxNodeType::NativeFunctionDecl:
            ret = std::make_shared<NativeFunctionDecl>(func_decl->token(), identifier, parameters,
                std::dynamic_pointer_cast<NativeFunctionDecl>(func_decl)->native_function_name());
            break;
        case SyntaxNodeType::IntrinsicDecl:
            ret = std::make_shared<IntrinsicDecl>(func_decl->token(), identifier, parameters);
            break;
        default:
            fatal("Unreachable");
        }
        break;
    }

    case SyntaxNodeType::BoundFunctionDecl:
    case SyntaxNodeType::BoundNativeFunctionDecl:
    case SyntaxNodeType::BoundIntrinsicDecl: {
        auto func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(tree);
        auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(func_decl->identifier());
        BoundIdentifiers parameters;
        for (auto& param : func_decl->parameters()) {
            auto processed_param = std::dynamic_pointer_cast<BoundIdentifier>(param);
            parameters.push_back(processed_param);
        }
        switch (func_decl->node_type()) {
        case SyntaxNodeType::BoundFunctionDecl:
            ret = std::make_shared<BoundFunctionDecl>(func_decl, identifier, parameters);
            break;
        case SyntaxNodeType::BoundNativeFunctionDecl:
            ret = std::make_shared<BoundNativeFunctionDecl>(std::dynamic_pointer_cast<BoundNativeFunctionDecl>(func_decl), identifier, parameters);
            break;
        case SyntaxNodeType::BoundIntrinsicDecl:
            ret = std::make_shared<BoundIntrinsicDecl>(func_decl, identifier, parameters);
            break;
        default:
            fatal("Unreachable");
        }
        break;
    }

    case SyntaxNodeType::ExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, stmt->expression(), ctx);
        ret = std::make_shared<ExpressionStatement>(expr);
        break;
    }

    case SyntaxNodeType::ExpressionList: {
        auto list = std::dynamic_pointer_cast<ExpressionList>(tree);
        Expressions expressions;
        for (auto const& expr : list->expressions()) {
            auto processed = TRY_AND_CAST(Expression, expr, ctx);
            expressions.push_back(processed);
        }
        return std::make_shared<ExpressionList>(list->token(), expressions);
    }

    case SyntaxNodeType::BoundExpressionStatement: {
        auto stmt = std::dynamic_pointer_cast<BoundExpressionStatement>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, stmt->expression(), ctx);
        ret = std::make_shared<BoundExpressionStatement>(stmt->token(), expr);
        break;
    }

    case SyntaxNodeType::BinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);

        auto lhs = TRY_AND_CAST(Expression, expr->lhs(), ctx);
        auto rhs = TRY_AND_CAST(Expression, expr->rhs(), ctx);
        ret = std::make_shared<BinaryExpression>(lhs, expr->op(), rhs, expr->type());
        break;
    }

    case SyntaxNodeType::BoundBinaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundBinaryExpression>(tree);

        auto lhs = TRY_AND_CAST(BoundExpression, expr->lhs(), ctx);
        auto rhs = TRY_AND_CAST(BoundExpression, expr->rhs(), ctx);
        ret = std::make_shared<BoundBinaryExpression>(expr->token(), lhs, expr->op(), rhs, expr->type());
        break;
    }

    case SyntaxNodeType::UnaryExpression: {
        auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
        auto operand = TRY_AND_CAST(Expression, expr->operand(), ctx);
        ret = std::make_shared<UnaryExpression>(expr->op(), operand, expr->type());
        break;
    }

    case SyntaxNodeType::BoundUnaryExpression: {
        auto expr = std::dynamic_pointer_cast<BoundUnaryExpression>(tree);
        auto operand = TRY_AND_CAST(BoundExpression, expr->operand(), ctx);
        ret = std::make_shared<BoundUnaryExpression>(expr->token(), operand, expr->op(), expr->type());
        break;
    }

    case SyntaxNodeType::BoundConditionalValue: {
        auto conditional_value = std::dynamic_pointer_cast<BoundConditionalValue>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, conditional_value->expression(), ctx);
        ret = std::make_shared<BoundConditionalValue>(conditional_value->token(), expr, conditional_value->success(), conditional_value->type());
        break;
    }

    case SyntaxNodeType::CastExpression: {
        auto cast_expr = std::dynamic_pointer_cast<CastExpression>(tree);
        auto expr = TRY_AND_CAST(Expression, cast_expr->expression(), ctx);
        ret = std::make_shared<CastExpression>(cast_expr->token(), expr, cast_expr->type());
        break;
    }

    case SyntaxNodeType::BoundCastExpression: {
        auto cast_expr = std::dynamic_pointer_cast<BoundCastExpression>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, cast_expr->expression(), ctx);
        ret = std::make_shared<BoundCastExpression>(cast_expr->token(), expr, cast_expr->type());
        break;
    }

    case SyntaxNodeType::BoundAssignment: {
        auto assignment = std::dynamic_pointer_cast<BoundAssignment>(tree);
        auto assignee = TRY_AND_CAST(BoundVariableAccess, assignment->assignee(), ctx);
        auto expression = TRY_AND_CAST(BoundExpression, assignment->expression(), ctx);
        ret = std::make_shared<BoundAssignment>(assignment->token(), assignee, expression);
        break;
    }

    case SyntaxNodeType::BoundModule: {
        auto module = std::dynamic_pointer_cast<BoundModule>(tree);
        auto block = TRY_AND_CAST(Block, module->block(), ctx);
        ret = std::make_shared<BoundModule>(module->token(), module->name(), block, module->exports(), module->imports());
        break;
    }

    case SyntaxNodeType::BoundFunctionCall: {
        auto func_call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
        auto arguments = TRY(xform_bound_expressions(func_call->arguments(), ctx, result, processor));
        ret = std::make_shared<BoundFunctionCall>(func_call, arguments);
        break;
    }

    case SyntaxNodeType::BoundNativeFunctionCall: {
        auto func_call = std::dynamic_pointer_cast<BoundNativeFunctionCall>(tree);
        auto arguments = TRY(xform_bound_expressions(func_call->arguments(), ctx, result, processor));
        ret = std::make_shared<BoundNativeFunctionCall>(func_call, arguments);
        break;
    }

    case SyntaxNodeType::BoundIntrinsicCall: {
        auto call = std::dynamic_pointer_cast<BoundIntrinsicCall>(tree);
        auto arguments = TRY(xform_bound_expressions(call->arguments(), ctx, result, processor));
        ret = std::make_shared<BoundIntrinsicCall>(call, arguments);
        break;
    }

    case SyntaxNodeType::VariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, var_decl->expression(), ctx);
        ret = std::make_shared<VariableDeclaration>(var_decl->token(), var_decl->identifier(), expr);
        break;
    }

    case SyntaxNodeType::StaticVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<StaticVariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, var_decl->expression(), ctx);
        ret = std::make_shared<StaticVariableDeclaration>(var_decl->token(), var_decl->identifier(), expr);
        break;
    }

    case SyntaxNodeType::LocalVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<LocalVariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, var_decl->expression(), ctx);
        ret = std::make_shared<LocalVariableDeclaration>(var_decl->token(), var_decl->identifier(), expr);
        break;
    }

    case SyntaxNodeType::GlobalVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<GlobalVariableDeclaration>(tree);
        auto expr = TRY_AND_CAST(Expression, var_decl->expression(), ctx);
        ret = std::make_shared<GlobalVariableDeclaration>(var_decl->token(), var_decl->identifier(), expr);
        break;
    }

    case SyntaxNodeType::BoundVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
        auto identifier = TRY_AND_CAST(BoundIdentifier, var_decl->variable(), ctx);
        auto expr = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
        ret = std::make_shared<BoundVariableDeclaration>(var_decl->token(), identifier, var_decl->is_const(), expr);
        break;
    }

    case SyntaxNodeType::BoundStaticVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<BoundStaticVariableDeclaration>(tree);
        auto identifier = TRY_AND_CAST(BoundIdentifier, var_decl->variable(), ctx);
        auto expr = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
        ret = std::make_shared<BoundStaticVariableDeclaration>(var_decl->token(), identifier, var_decl->is_const(), expr);
        break;
    }

    case SyntaxNodeType::BoundLocalVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<BoundLocalVariableDeclaration>(tree);
        auto identifier = TRY_AND_CAST(BoundIdentifier, var_decl->variable(), ctx);
        auto expr = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
        ret = std::make_shared<BoundLocalVariableDeclaration>(var_decl->token(), identifier, var_decl->is_const(), expr);
        break;
    }

    case SyntaxNodeType::BoundGlobalVariableDeclaration: {
        auto var_decl = std::dynamic_pointer_cast<BoundGlobalVariableDeclaration>(tree);
        auto identifier = TRY_AND_CAST(BoundIdentifier, var_decl->variable(), ctx);
        auto expr = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
        ret = std::make_shared<BoundGlobalVariableDeclaration>(var_decl->token(), identifier, var_decl->is_const(), expr);
        break;
    }

    case SyntaxNodeType::Return: {
        auto return_stmt = std::dynamic_pointer_cast<Return>(tree);
        auto expr = TRY_AND_CAST(Expression, return_stmt->expression(), ctx);
        ret = std::make_shared<Return>(return_stmt->token(), expr, return_stmt->return_error());
        break;
    }

    case SyntaxNodeType::BoundReturn: {
        auto return_stmt = std::dynamic_pointer_cast<BoundReturn>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, return_stmt->expression(), ctx);
        ret = std::make_shared<BoundReturn>(return_stmt, expr, return_stmt->return_error());
        break;
    }

    case SyntaxNodeType::Branch: {
        auto branch = std::dynamic_pointer_cast<Branch>(tree);
        std::shared_ptr<Expression> condition { nullptr };
        if (branch->condition())
            condition = TRY_AND_CAST(Expression, branch->condition(), ctx);
        auto statement = TRY_AND_CAST(Statement, branch->statement(), ctx);
        ret = std::make_shared<Branch>(branch, condition, statement);
        break;
    }

    case SyntaxNodeType::BoundBranch: {
        auto branch = std::dynamic_pointer_cast<BoundBranch>(tree);
        std::shared_ptr<BoundExpression> condition { nullptr };
        if (branch->condition())
            condition = TRY_AND_CAST(BoundExpression, branch->condition(), ctx);
        auto statement = TRY_AND_CAST(Statement, branch->statement(), ctx);
        ret = std::make_shared<BoundBranch>(branch, condition, statement);
        break;
    }

    case SyntaxNodeType::IfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);
        Branches branches;
        for (auto& branch : if_stmt->branches()) {
            auto branch_maybe = processor(branch, ctx, result);
            if (branch_maybe.has_value())
                branches.push_back(std::dynamic_pointer_cast<Branch>(branch_maybe.value()));
        }
        ret = std::make_shared<IfStatement>(if_stmt->token(), branches);
        break;
    }

    case SyntaxNodeType::BoundIfStatement: {
        auto if_stmt = std::dynamic_pointer_cast<BoundIfStatement>(tree);
        BoundBranches branches;
        for (auto& branch : if_stmt->branches()) {
            auto branch_processed = TRY_AND_CAST(BoundBranch, branch, ctx);
            branches.push_back(branch_processed);
        }
        ret = std::make_shared<BoundIfStatement>(if_stmt->token(), branches);
        break;
    }

    case SyntaxNodeType::WhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
        auto condition = TRY_AND_CAST(Expression, while_stmt->condition(), ctx);
        auto stmt = TRY_AND_CAST(Statement, while_stmt->statement(), ctx);
        if ((condition != while_stmt->condition()) || (stmt != while_stmt->statement()))
            ret = std::make_shared<WhileStatement>(while_stmt->token(), condition, stmt);
        break;
    }

    case SyntaxNodeType::BoundWhileStatement: {
        auto while_stmt = std::dynamic_pointer_cast<BoundWhileStatement>(tree);
        auto condition = TRY_AND_CAST(BoundExpression, while_stmt->condition(), ctx);
        auto stmt = TRY_AND_CAST(Statement, while_stmt->statement(), ctx);
        if ((condition != while_stmt->condition()) || (stmt != while_stmt->statement()))
            ret = std::make_shared<BoundWhileStatement>(while_stmt, condition, stmt);
        break;
    }

    case SyntaxNodeType::ForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<ForStatement>(tree);
        auto variable = TRY_AND_CAST(Variable, for_stmt->variable(), ctx);
        auto range = TRY_AND_CAST(Expression, for_stmt->range(), ctx);
        auto stmt = TRY_AND_CAST(Statement, for_stmt->statement(), ctx);
        ret = std::make_shared<ForStatement>(for_stmt->token(), variable, range, stmt);
        break;
    }

    case SyntaxNodeType::BoundForStatement: {
        auto for_stmt = std::dynamic_pointer_cast<BoundForStatement>(tree);
        auto variable = TRY_AND_CAST(BoundVariable, for_stmt->variable(), ctx);
        auto range = TRY_AND_CAST(BoundExpression, for_stmt->range(), ctx);
        auto stmt = TRY_AND_CAST(Statement, for_stmt->statement(), ctx);
        ret = std::make_shared<BoundForStatement>(for_stmt, variable, range, stmt);
        break;
    }

    case SyntaxNodeType::CaseStatement: {
        auto stmt = std::dynamic_pointer_cast<CaseStatement>(tree);
        std::shared_ptr<Expression> condition { nullptr };
        if (stmt->condition())
            condition = TRY_AND_CAST(Expression, stmt->condition(), ctx);
        auto statement = TRY_AND_CAST(Statement, stmt->statement(), ctx);
        ret = std::make_shared<CaseStatement>(stmt, condition, statement);
        break;
    }

    case SyntaxNodeType::DefaultCase: {
        auto stmt = std::dynamic_pointer_cast<DefaultCase>(tree);
        std::shared_ptr<Expression> condition { nullptr };
        if (stmt->condition())
            condition = TRY_AND_CAST(Expression, stmt->condition(), ctx);
        auto statement = TRY_AND_CAST(Statement, stmt->statement(), ctx);
        ret = std::make_shared<DefaultCase>(stmt, condition, statement);
        break;
    }

    case SyntaxNodeType::SwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
        auto expr = TRY_AND_CAST(Expression, switch_stmt->expression(), ctx);
        CaseStatements cases;
        for (auto& case_stmt : switch_stmt->cases()) {
            cases.push_back(TRY_AND_CAST(CaseStatement, case_stmt, ctx));
        }
        auto default_case = TRY_AND_CAST(DefaultCase, switch_stmt->default_case(), ctx);
        ret = std::make_shared<SwitchStatement>(switch_stmt->token(), expr, cases, default_case);
        break;
    }

    case SyntaxNodeType::BoundSwitchStatement: {
        auto switch_stmt = std::dynamic_pointer_cast<BoundSwitchStatement>(tree);
        auto expr = TRY_AND_CAST(BoundExpression, switch_stmt->expression(), ctx);
        BoundBranches cases;
        for (auto& case_stmt : switch_stmt->cases()) {
            cases.push_back(TRY_AND_CAST(BoundBranch, case_stmt, ctx));
        }
        auto default_case = TRY_AND_CAST(BoundBranch, switch_stmt->default_case(), ctx);
        ret = std::make_shared<BoundSwitchStatement>(switch_stmt, expr, cases, default_case);
        break;
    }

    default:
        break;
    }

    return ret;
}

template<class StmtClass, typename Context, typename Processor, typename... Args>
ErrorOrNode process_block(std::shared_ptr<SyntaxNode> const& tree, Context& ctx, ProcessResult& result, Processor& processor, Args&&... args)
{
    auto block = std::dynamic_pointer_cast<Block>(tree);
    Context& child_ctx = make_subcontext<Context>(ctx);
    Statements statements;
    for (auto& stmt : block->statements()) {
        auto processed_or_error = processor(stmt, child_ctx, result);
        if (processed_or_error.is_error()) {
            result = processed_or_error.error();
            continue;
        }
        auto processed_node = processed_or_error.value();
        if (auto new_statement = std::dynamic_pointer_cast<Statement>(processed_node); new_statement != nullptr) {
            statements.push_back(new_statement);
        } else if (auto node_list = std::dynamic_pointer_cast<NodeList<Statement>>(processed_node); node_list != nullptr) {
            for (auto const& node : *node_list) {
                statements.push_back(node);
            }
        }
    }
    if (statements != block->statements())
        return std::make_shared<StmtClass>(tree->token(), statements, std::forward<Args>(args)...);
    else
        return tree;
}

template<typename Context, typename Processor>
ErrorOr<Expressions, SyntaxError> xform_expressions(Expressions const& expressions, Context& ctx, ProcessResult& result, Processor processor)
{
    Expressions ret;
    for (auto& expr : expressions) {
        auto new_expr = TRY(processor(expr, ctx, result));
        ret.push_back(std::dynamic_pointer_cast<Expression>(new_expr));
    }
    return ret;
}

template<typename Context, typename Processor>
ErrorOr<BoundExpressions, SyntaxError> xform_bound_expressions(BoundExpressions const& expressions, Context& ctx, ProcessResult& result, Processor processor)
{
    BoundExpressions ret;
    for (auto& expr : expressions) {
        auto new_expr = processor(expr, ctx, result);
        if (new_expr.is_error())
            return new_expr.error();
        ret.push_back(std::dynamic_pointer_cast<BoundExpression>(new_expr.value()));
    }
    return ret;
}

template<typename Ctx>
ProcessResult& process(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx, ProcessResult& result)
{
    if (!tree) {
        result.value(nullptr);
        return result;
    }
    std::string log_message;
    debug(processor, "Process <{} {}>", tree->node_type(), tree);
    switch (tree->node_type()) {
#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type)                                                           \
    case SyntaxNodeType::type: {                                                            \
        log_message = format("<{} {}> => ", #type, tree);                                   \
        ErrorOrNode processed = process_node<Ctx, SyntaxNodeType::type>(tree, ctx, result); \
        result = nullptr;                                                                   \
        if (processed.is_error()) {                                                         \
            log_message += format("Error {}", processed.error());                           \
            result = processed.error();                                                     \
        } else {                                                                            \
            result = processed.value();                                                     \
            log_message += format("<{} {}>", result.value()->node_type(), result.value());  \
        }                                                                                   \
        debug(processor, "{}", log_message);                                                   \
        return result;                                                                      \
    }
        ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE
    default:
        fatal("Unknown SyntaxNodeType '{}'", tree->node_type());
    }
}

template<typename Ctx>
ProcessResult process(std::shared_ptr<SyntaxNode> const& tree)
{
    ProcessResult result;
    Ctx ctx;
    return process<Ctx>(tree, ctx, result);
}

template<typename Ctx>
ProcessResult process(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx)
{
    ProcessResult result;
    return process<Ctx>(tree, ctx, result);
}

template<typename Ctx, SyntaxNodeType node_type>
ErrorOrNode process_node(std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx, ProcessResult& result)
{
    debug(processor, "Falling back to default processor for type {}", tree->node_type());
    return process_tree(tree, ctx, result, [](std::shared_ptr<SyntaxNode> const& tree, Ctx& ctx, ProcessResult& result) {
        return process(tree, ctx, result);
    });
}

template<class Cls, class Ctx>
ErrorOrTypedNode<Cls> try_and_try_cast(pSyntaxNode const& expr, Ctx& ctx, ProcessResult& result)
{
    auto processed_maybe = process(expr, ctx, result);
    if (processed_maybe.is_error()) {
        debug(processor, "Processing node results in error '{}' instead of node of type '{}'",
            processed_maybe.error(), typeid(Cls).name());
        return processed_maybe.error();
    }
    auto processed = processed_maybe.value();
    if (processed == nullptr)
        return nullptr;
    return std::dynamic_pointer_cast<Cls>(processed);
}

template<class Cls, class Ctx>
ErrorOrTypedNode<Cls> try_and_cast(pSyntaxNode const& expr, Ctx& ctx, ProcessResult& result)
{
    auto processed_maybe = process(expr, ctx, result);
    if (processed_maybe.is_error()) {
        debug(processor, "Processing node results in error '{}' instead of node of type '{}'",
            processed_maybe.error(), typeid(Cls).name());
        return processed_maybe.error();
    }
    auto processed = processed_maybe.value();
    if (processed == nullptr)
        return nullptr;
    auto casted = std::dynamic_pointer_cast<Cls>(processed);
    if (casted == nullptr) {
        return SyntaxError {
            ErrorCode::InternalError,
            format("Processing node results in unexpected type '{}' instead of '{}'",
                processed->node_type(), typeid(Cls).name())
        };
    }
    return casted;
}

#define INIT_NODE_PROCESSOR(context_type)                                                                                    \
    using ContextType = context_type;                                                                                        \
    ProcessResult context_type##_processor(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx, ProcessResult& result) \
    {                                                                                                                        \
        return process<context_type>(tree, ctx, result);                                                                     \
    }

#define NODE_PROCESSOR(node_type) \
    template<>                    \
    ErrorOrNode process_node<ContextType, SyntaxNodeType::node_type>(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx, ProcessResult& result)

#define ALIAS_NODE_PROCESSOR(node_type, alias_node_type)                                                                                               \
    template<>                                                                                                                                         \
    ErrorOrNode process_node<ContextType, SyntaxNodeType::node_type>(std::shared_ptr<SyntaxNode> const& tree, ContextType& ctx, ProcessResult& result) \
    {                                                                                                                                                  \
        return process_node<ContextType, SyntaxNodeType::alias_node_type>(tree, ctx, result);                                                          \
    }

ProcessResult fold_constants(std::shared_ptr<SyntaxNode> const&);
ProcessResult bind_types(std::shared_ptr<SyntaxNode> const&, Config const&);
ProcessResult lower(std::shared_ptr<SyntaxNode> const&, Config const&);
ProcessResult resolve_operators(std::shared_ptr<SyntaxNode> const&);
}
