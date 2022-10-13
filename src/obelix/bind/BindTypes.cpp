/*
 * Copyright (c) 2021, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <memory>
#include <map>

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/Intrinsics.h>
#include <obelix/Parser.h>
#include <obelix/Processor.h>
#include <obelix/bind/BindContext.h>

namespace Obelix {

extern_logging_category(parser);

ErrorOr<std::shared_ptr<BoundFunctionCall>, SyntaxError> make_function_call(BindContext& ctx, std::shared_ptr<BoundFunction> function, std::shared_ptr<BoundExpressionList> arguments)
{
    BoundExpressions args;
    ObjectTypes arg_types;
    for (auto& arg : arguments->expressions()) {
        args.push_back(arg);
        arg_types.push_back(arg->type());
    }
    std::shared_ptr<BoundFunctionDecl> func_decl;
    switch (function->node_type()) {
    case SyntaxNodeType::BoundLocalFunction: {
        func_decl = ctx.match(function->name(), arg_types);
        break;
    }
    case SyntaxNodeType::BoundImportedFunction: {
        auto imported_func = std::dynamic_pointer_cast<BoundImportedFunction>(function);
        func_decl = imported_func->module()->resolve(function->name(), arg_types);
        break;
    }
    default:
        fatal("Unreachable: {}", function->node_type());
    }
    if (func_decl == nullptr) {
        ctx.add_unresolved_function(make_functioncall(function, arg_types));
        return nullptr;
    }

    switch (func_decl->node_type()) {
    case SyntaxNodeType::BoundIntrinsicDecl: {
        auto intrinsic = IntrinsicType_by_name(func_decl->name());
        if (intrinsic == IntrinsicType::NotIntrinsic)
            return SyntaxError { ErrorCode::SyntaxError, function->token(), format("Intrinsic {} not defined", func_decl->name()) };
        return std::make_shared<BoundIntrinsicCall>(function->token(), std::dynamic_pointer_cast<BoundIntrinsicDecl>(func_decl), args, intrinsic);
    }
    case SyntaxNodeType::BoundNativeFunctionDecl:
        return std::make_shared<BoundNativeFunctionCall>(function->token(), std::dynamic_pointer_cast<BoundNativeFunctionDecl>(func_decl), args);
    default:
        return std::make_shared<BoundFunctionCall>(function->token(), func_decl, args);
    }
}

INIT_NODE_PROCESSOR(BindContext);

NODE_PROCESSOR(StructDefinition)
{
    auto struct_def = std::dynamic_pointer_cast<StructDefinition>(tree);
    if (struct_def->fields().empty())
        return SyntaxError { ErrorCode::SyntaxError, struct_def->token(), format("Struct {} has no fields", struct_def->name()) };
    BoundIdentifiers bound_fields;
    FieldDefs field_defs {};
    for (auto const& field : struct_def->fields()) {
        auto field_type_maybe = field->type()->resolve_type();
        if (field_type_maybe.is_error()) {
            auto err = field_type_maybe.error();
            return SyntaxError { err.code(), struct_def->token(), err.message() };
        }
        auto field_type = field_type_maybe.value();
        bound_fields.push_back(std::make_shared<BoundIdentifier>(field, field_type));
        field_defs.emplace_back(field->name(), field_type);
    }
    auto type_maybe = ObjectType::make_struct_type(struct_def->name(), field_defs);
    if (type_maybe.is_error()) {
        auto err = type_maybe.error();
        return SyntaxError { err.code(), struct_def->token(), err.message() };
    }
    auto type = type_maybe.value();
    return std::make_shared<BoundStructDefinition>(struct_def, type);
}

NODE_PROCESSOR(EnumDef)
{
    auto enum_def = std::dynamic_pointer_cast<EnumDef>(tree);
    NVPs enum_values;
    BoundEnumValueDefs bound_values;
    long v = 0;
    for (auto const& value : enum_def->values()) {
        if (value->value().has_value())
            // FIXME Check sanity of value
            v = value->value().value();
        enum_values.emplace_back(value->label(), v);
        bound_values.push_back(std::make_shared<BoundEnumValueDef>(value->token(), value->label(), v));
        v++;
    }
    std::shared_ptr<ObjectType> type;
    if (!enum_def->extend()) {
        type = ObjectType::make_enum_type(enum_def->name(), enum_values);
    } else {
        type = ObjectType::get(enum_def->name());
        if (type->type() != PrimitiveType::Enum)
            return SyntaxError { ErrorCode::NoSuchType, enum_def->token(), "Cannot extend non-existing enum '{}'", enum_def->name() };
        auto error_maybe = type->extend_enum_type(enum_values);
        if (error_maybe.is_error())
            return SyntaxError { error_maybe.error().code(), enum_def->token(), error_maybe.error().message() };
    }
    return std::make_shared<BoundEnumDef>(enum_def, type, bound_values);
}

NODE_PROCESSOR(Compilation)
{
    auto compilation = std::dynamic_pointer_cast<Compilation>(tree);
    assert(ctx.type() == BindContextType::RootContext);
    BoundModules modules;
    for (auto& imported : compilation->modules()) {
        modules.push_back(TRY_AND_CAST(BoundModule, process(imported, ctx)));
    }
    return std::make_shared<BoundCompilation>(modules);

}

NODE_PROCESSOR(Module)
{
    auto module = std::dynamic_pointer_cast<Module>(tree);
    assert(ctx.type() == BindContextType::RootContext);
    BindContext module_ctx(ctx, BindContextType::ModuleContext);
    Statements statements;
    for (auto& stmt : module->statements()) {
        auto new_statement = TRY_AND_CAST(Statement, process(stmt, module_ctx));
        statements.push_back(new_statement);
    }
    auto block = std::make_shared<Block>(tree->token(), statements);
    BoundFunctionDecls exports;
    for (const auto& name : module_ctx.names()) {
        auto const& declaration = std::dynamic_pointer_cast<BoundFunctionDecl>(name.second);
        if (declaration) {
            exports.push_back(declaration);
        }
    }
    auto ret = std::make_shared<BoundModule>(module->token(), module->name(), block, exports, module_ctx.imported_functions());
    ctx.declare(ret->name(), ret);
    ctx.add_module(ret);
    return ret;
}

NODE_PROCESSOR(BoundModule)
{
    auto module = std::dynamic_pointer_cast<BoundModule>(tree);
    ctx.declare(module->name(), module);
    ctx.add_module(module);
    return tree;
}

NODE_PROCESSOR(VariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<VariableDeclaration>(tree);
    auto t = var_decl->type();
    std::shared_ptr<ObjectType> var_type { nullptr };
    if (t) {
        auto var_type_maybe = t->resolve_type();
        if (var_type_maybe.is_error()) {
            auto err = var_type_maybe.error();
            return SyntaxError { err.code(), var_decl->token(), err.message() };
        }
        var_type = var_type_maybe.value();
    }
    std::shared_ptr<BoundExpression> expr { nullptr };
    if (var_decl->expression()) {
        auto processed_expr = TRY(process(var_decl->expression(), ctx));
        assert(processed_expr != nullptr);
        expr = std::dynamic_pointer_cast<BoundExpression>(processed_expr);
        if (expr == nullptr) {
            return tree;
        }

        if (var_type && var_type->type() != PrimitiveType::Any && !expr->type()->is_assignable_to(var_type)) {
            auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(expr);
            if (int_literal != nullptr) {
                expr = TRY(int_literal->cast(var_type));
            } else {
                ObjectType::dump();
                return SyntaxError { ErrorCode::TypeMismatch, var_decl->token(), var_decl->name(), var_decl->type(), expr->type() };
            }
        }
        if (!var_type)
            var_type = expr->type();
    } else if (!var_type) {
        return SyntaxError { ErrorCode::UntypedVariable, var_decl->token(), var_decl->name() };
    }

    auto identifier = std::make_shared<BoundIdentifier>(var_decl->identifier(), var_type);
    std::shared_ptr<BoundVariableDeclaration> ret;
    if (tree->node_type() == SyntaxNodeType::StaticVariableDeclaration)
        ret = std::make_shared<BoundStaticVariableDeclaration>(var_decl, identifier, expr);
    else if (tree->node_type() == SyntaxNodeType::LocalVariableDeclaration)
        ret = std::make_shared<BoundLocalVariableDeclaration>(var_decl, identifier, expr);
    else if (tree->node_type() == SyntaxNodeType::GlobalVariableDeclaration)
        ret = std::make_shared<BoundGlobalVariableDeclaration>(var_decl, identifier, expr);
    else
        ret = std::make_shared<BoundVariableDeclaration>(var_decl, identifier, expr);
    ctx.declare(var_decl->name(), ret);
    return ret;
}

ALIAS_NODE_PROCESSOR(StaticVariableDeclaration, VariableDeclaration);
ALIAS_NODE_PROCESSOR(LocalVariableDeclaration, VariableDeclaration);
ALIAS_NODE_PROCESSOR(GlobalVariableDeclaration, VariableDeclaration);

NODE_PROCESSOR(FunctionDecl)
{
    auto decl = std::dynamic_pointer_cast<FunctionDecl>(tree);
    if (decl->type() == nullptr)
        return SyntaxError { ErrorCode::UntypedFunction, decl->token(), decl->name() };

    // auto ret_type_or_error = decl->type()->resolve_type();
    // if (ret_type_or_error.is_error()) {
    //     return SyntaxError { ret_type_or_error.error(), decl->token() };
    // }
    auto ret_type = TRY_ADAPT(decl->type()->resolve_type(), decl->token());

    auto identifier = std::make_shared<BoundIdentifier>(decl->identifier(), ret_type);
    BoundIdentifiers bound_parameters;
    for (auto& parameter : decl->parameters()) {
        if (parameter->type() == nullptr)
            return SyntaxError { ErrorCode::UntypedParameter, parameter->token(), parameter->name() };
        auto parameter_type = TRY_ADAPT(parameter->type()->resolve_type(), identifier->token());
        bound_parameters.push_back(std::make_shared<BoundIdentifier>(parameter, parameter_type));
    }
    std::shared_ptr<BoundFunctionDecl> bound_decl;
    switch (decl->node_type()) {
    case SyntaxNodeType::IntrinsicDecl:
        bound_decl = std::make_shared<BoundIntrinsicDecl>(std::dynamic_pointer_cast<IntrinsicDecl>(decl), identifier, bound_parameters);
        break;
    case SyntaxNodeType::NativeFunctionDecl:
        bound_decl = std::make_shared<BoundNativeFunctionDecl>(std::dynamic_pointer_cast<NativeFunctionDecl>(decl), identifier, bound_parameters);
        break;
    default:
        bound_decl = std::make_shared<BoundFunctionDecl>(decl, identifier, bound_parameters);
        break;
    }
    ctx.declare(bound_decl->name(), bound_decl);
    ctx.add_declared_function(bound_decl->name(), bound_decl);
    return bound_decl;
}

ALIAS_NODE_PROCESSOR(IntrinsicDecl, FunctionDecl);
ALIAS_NODE_PROCESSOR(NativeFunctionDecl, FunctionDecl);

NODE_PROCESSOR(FunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<FunctionDef>(tree);
    auto decl = TRY_AND_CAST(BoundFunctionDecl, process(func_def->declaration(), ctx));

    std::shared_ptr<Statement> func_block { nullptr };
    if (func_def->statement()) {
        BindContext func_ctx(ctx, BindContextType::SubContext);
        for (auto& param : decl->parameters()) {
            auto dummy_decl = std::make_shared<VariableDeclaration>(param->token(), std::make_shared<Identifier>(param->token(), param->name()));
            func_ctx.declare(param->name(), std::make_shared<BoundVariableDeclaration>(dummy_decl, param, nullptr));
        }
        func_ctx.return_type = decl->type();
        func_block = TRY_AND_CAST(Statement, process(func_def->statement(), func_ctx));
    }
    return std::make_shared<BoundFunctionDef>(func_def, decl, func_block);
}

NODE_PROCESSOR(BinaryExpression)
{
    auto expr = std::dynamic_pointer_cast<BinaryExpression>(tree);
    auto lhs = TRY_AND_TRY_CAST(BoundExpression, process(expr->lhs(), ctx));

    struct BinaryOperatorMap {
        TokenCode code;
        BinaryOperator op;
    };

    constexpr static BinaryOperatorMap operator_map[] = {
        { TokenCode::Plus, BinaryOperator::Add },
        { TokenCode::Minus, BinaryOperator::Subtract },
        { TokenCode::Asterisk, BinaryOperator::Multiply },
        { TokenCode::Slash, BinaryOperator::Divide },
        { TokenCode::Percent, BinaryOperator::Modulo },
        { TokenCode::Equals, BinaryOperator::Assign },
        { TokenCode::EqualsTo, BinaryOperator::Equals },
        { TokenCode::NotEqualTo, BinaryOperator::NotEquals },
        { TokenCode::GreaterEqualThan, BinaryOperator::GreaterEquals },
        { TokenCode::LessEqualThan, BinaryOperator::LessEquals },
        { TokenCode::GreaterThan, BinaryOperator::Greater },
        { TokenCode::LessThan, BinaryOperator::Less },
        { TokenCode::LogicalAnd, BinaryOperator::LogicalAnd },
        { TokenCode::LogicalOr, BinaryOperator::LogicalOr },
        { TokenCode::Ampersand, BinaryOperator::BitwiseAnd },
        { TokenCode::Pipe, BinaryOperator::BitwiseOr },
        { TokenCode::Hat, BinaryOperator::BitwiseXor },
        { TokenCode::Period, BinaryOperator::MemberAccess },
        { TokenCode::OpenBracket, BinaryOperator::Subscript },
        { TokenCode::OpenParen, BinaryOperator::Call },
        { Parser::KeywordIncEquals, BinaryOperator::BinaryIncrement },
        { Parser::KeywordDecEquals, BinaryOperator::BinaryDecrement },
        { Parser::KeywordRange, BinaryOperator::Range },
    };

    BinaryOperator op = BinaryOperator::Invalid;
    for (auto ix : operator_map) {
        if (ix.code == expr->op().code()) {
            op = ix.op;
            break;
        }
    }
    if (op == BinaryOperator::Invalid)
        return SyntaxError { ErrorCode::OperatorUnresolved, expr->token(), expr->op().value(), lhs->to_string() };

    auto rhs = expr->rhs();
    if (op == BinaryOperator::MemberAccess) {
        if (lhs == nullptr)
            return SyntaxError { ErrorCode::SyntaxError, expr->token(), format("Cannot access member {} of {}", expr->rhs(), expr->lhs()) };
        switch (lhs->type()->type()) {
        case PrimitiveType::Struct: {
            auto field_var = std::dynamic_pointer_cast<Variable>(rhs);
            if (field_var == nullptr)
                return SyntaxError { ErrorCode::NotMember, rhs->token(), rhs, lhs };
            auto field = lhs->type()->field(field_var->name());
            if (field.type->type() == PrimitiveType::Unknown)
                return SyntaxError { ErrorCode::NotMember, rhs->token(), rhs, lhs };
            auto member_identifier = std::make_shared<BoundIdentifier>(rhs->token(), field_var->name(), field.type);
            return std::make_shared<BoundMemberAccess>(lhs, member_identifier);
        }
        case PrimitiveType::Type: {
            auto type_literal = std::dynamic_pointer_cast<BoundTypeLiteral>(lhs);
            auto value_var = std::dynamic_pointer_cast<Variable>(rhs);
            if (value_var == nullptr)
                return SyntaxError { ErrorCode::NotMember, rhs->token(), rhs, lhs };
            assert(type_literal->value()->type() == PrimitiveType::Enum);
            auto values = type_literal->value()->template_argument_values<NVP>("values");
            for (auto const& v : values) {
                if (v.first == value_var->name())
                    return std::make_shared<BoundEnumValue>(lhs->token(), type_literal->value(), v.first, v.second);
            }
            return SyntaxError { ErrorCode::NotMember, rhs->token(), rhs, lhs };
        }
        case PrimitiveType::Module: {
            auto module = std::dynamic_pointer_cast<BoundModule>(lhs);
            auto module_member = std::dynamic_pointer_cast<Variable>(rhs);
            if (module_member == nullptr)
                return SyntaxError { ErrorCode::CannotAccessMember, rhs->token(), rhs->to_string() };
            auto func = module->exported(module_member->name());
            if (func != nullptr) {
                ctx.add_imported_function(func);
                return std::make_shared<BoundImportedFunction>(rhs->token(), module, module_member->name());
            }
            return SyntaxError { ErrorCode::CannotAccessMember, rhs->token(), rhs->to_string() };
        }
        default:
            return SyntaxError { ErrorCode::CannotAccessMember, lhs->token(), lhs->to_string() };
        }
    }

    auto rhs_processed = TRY(process(expr->rhs(), ctx));
    auto rhs_bound = std::dynamic_pointer_cast<BoundExpression>(rhs_processed);

    if (op == BinaryOperator::Call) {
        if (lhs != nullptr && lhs->type()->type() != PrimitiveType::Function)
            return SyntaxError { ErrorCode::ObjectNotCallable, expr->lhs() };
        if (lhs == nullptr) {
            if (auto variable = std::dynamic_pointer_cast<Variable>(expr->lhs()); variable != nullptr) {
                auto declaration_maybe = ctx.get(variable->name());
                if (declaration_maybe.has_value()) {
                    if (auto declaration = std::dynamic_pointer_cast<BoundFunctionDecl>(declaration_maybe.value()); declaration != nullptr)
                        lhs = std::make_shared<BoundLocalFunction>(variable->token(), variable->name());
                }
                if (lhs == nullptr) {
                    auto root_module = ctx.module("");
                    assert(root_module != nullptr);
                    if (auto declaration = root_module->exported(variable->name()); declaration != nullptr) {
                        ctx.add_imported_function(declaration);
                        lhs = std::make_shared<BoundImportedFunction>(variable->token(), root_module, variable->name());
                    }
                }
            }
        }
        if (lhs == nullptr)
            return SyntaxError { ErrorCode::ObjectNotCallable, expr->lhs() };
        if (rhs_bound == nullptr || rhs_bound->type()->type() != PrimitiveType::List)
            return SyntaxError { ErrorCode::SyntaxError, format("Cannot call {} with {}", lhs, expr->rhs()) };
        auto ret = TRY(make_function_call(ctx, std::dynamic_pointer_cast<BoundFunction>(lhs), std::dynamic_pointer_cast<BoundExpressionList>(rhs_bound)));
        return ret;
    }

    if (lhs == nullptr)
        return SyntaxError { ErrorCode::UntypedExpression, expr->lhs()->token(), expr->lhs()->to_string() };

    if (op == BinaryOperator::Subscript) {
        if (lhs->type()->type() != PrimitiveType::Array)
            return SyntaxError { ErrorCode::CannotAccessMember, lhs->token(), lhs->to_string() };
        if (rhs_bound->type()->type() != PrimitiveType::SignedIntegerNumber)
            return SyntaxError { ErrorCode::TypeMismatch, rhs->token(), rhs, ObjectType::get(PrimitiveType::Int), rhs_bound->type() };
        if (rhs_bound->node_type() == SyntaxNodeType::BoundIntLiteral) {
            auto literal = std::dynamic_pointer_cast<BoundIntLiteral>(rhs_bound);
            auto value = literal->value<int>();
            if ((value < 0) || (lhs->type()->template_argument<long>("size") <= value))
                return SyntaxError { ErrorCode::IndexOutOfBounds, rhs->token(), value, lhs->type()->template_argument<long>("size") };
        }
        return std::make_shared<BoundArrayAccess>(lhs, rhs_bound, lhs->type()->template_argument<std::shared_ptr<ObjectType>>("base_type"));
    }

    if (op == BinaryOperator::Assign || BinaryOperator_is_assignment(op)) {
        auto assignee = std::dynamic_pointer_cast<BoundVariableAccess>(lhs);
        if (assignee == nullptr)
            return SyntaxError { ErrorCode::CannotAssignToRValue, lhs->token(), lhs->to_string() };
        auto lhs_as_ident = std::dynamic_pointer_cast<BoundIdentifier>(assignee);
        if (lhs_as_ident) {
            auto identifier = std::dynamic_pointer_cast<BoundIdentifier>(lhs);
            auto var_decl_maybe = ctx.get(identifier->name());
            if (!var_decl_maybe.has_value())
                return SyntaxError { ErrorCode::UndeclaredVariable, lhs->token(), identifier->name() };
            auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(var_decl_maybe.value());
            if (var_decl == nullptr)
                return SyntaxError { ErrorCode::CannotAssignToFunction, lhs->token(), identifier->name() };
            if (var_decl->is_const())
                return SyntaxError { ErrorCode::CannotAssignToConstant, lhs->token(), var_decl->name() };
        }
        if (!rhs_bound->type()->is_assignable_to(lhs->type())) {
            auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(rhs_bound);
            if (int_literal != nullptr) {
                rhs_bound = TRY(int_literal->cast(lhs->type()));
            } else {
                return SyntaxError { ErrorCode::TypeMismatch, expr->token(), rhs->to_string(), lhs->type(), rhs_bound->type() };
            }
        }

        if (op == BinaryOperator::Assign)
            return std::make_shared<BoundAssignment>(expr->token(), assignee, rhs_bound);

        // +=, -= and friends: rewrite to a straight-up assignment to a binary
        auto new_rhs = std::make_shared<BoundBinaryExpression>(expr->token(),
            lhs, BinaryOperator_for_assignment_operator(op), rhs_bound, rhs_bound->type());
        return std::make_shared<BoundAssignment>(expr->token(), assignee, new_rhs);
    }

    if ((rhs_bound->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs_bound->type()->type() == lhs->type()->type()) && (rhs_bound->type()->size() > lhs->type()->size())) {
        rhs_bound = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(rhs_bound)->cast(lhs->type()));
    } else if ((lhs->node_type() == SyntaxNodeType::BoundIntLiteral) && (rhs_bound->type()->type() == lhs->type()->type()) && (lhs->type()->size() > rhs_bound->type()->size())) {
        lhs = TRY(std::dynamic_pointer_cast<BoundIntLiteral>(lhs)->cast(rhs_bound->type()));
    }
    auto return_type_maybe = lhs->type()->return_type_of(to_operator(op), rhs_bound->type());
    if (return_type_maybe.has_value())
        return std::make_shared<BoundBinaryExpression>(expr, lhs, op, rhs_bound, return_type_maybe.value());
    return SyntaxError { ErrorCode::ReturnTypeUnresolved, expr->token(), format("{} {} {}", lhs, op, rhs) };
}

NODE_PROCESSOR(UnaryExpression)
{
    auto expr = std::dynamic_pointer_cast<UnaryExpression>(tree);
    auto operand = TRY_AND_CAST(BoundExpression, process(expr->operand(), ctx));
    if (operand == nullptr)
        return tree;

    struct UnaryOperatorMap {
        TokenCode code;
        UnaryOperator op;
    };

    constexpr static UnaryOperatorMap operator_map[8] = {
        { TokenCode::Asterisk, UnaryOperator::Dereference },
        { TokenCode::AtSign, UnaryOperator::AddressOf },
        { TokenCode::Plus, UnaryOperator::Identity },
        { TokenCode::Minus, UnaryOperator::Negate },
        { TokenCode::ExclamationPoint, UnaryOperator::LogicalInvert },
        { TokenCode::UnaryIncrement, UnaryOperator::UnaryIncrement },
        { TokenCode::UnaryDecrement, UnaryOperator::UnaryDecrement },
        { TokenCode::Tilde, UnaryOperator::BitwiseInvert },
    };

    UnaryOperator op { UnaryOperator::InvalidUnary };
    for (auto ix : operator_map) {
        if (ix.code == expr->op().code()) {
            op = ix.op;
            break;
        }
    }
    if (op == UnaryOperator::InvalidUnary)
        return SyntaxError { ErrorCode::OperatorUnresolved, expr->token(), expr->op().value(), operand->to_string() };

    auto return_type = operand->type()->return_type_of(to_operator(op));
    if (!return_type.has_value())
        return SyntaxError { ErrorCode::ReturnTypeUnresolved, expr->token(), format("{} {}", op, operand) };
    return std::make_shared<BoundUnaryExpression>(expr, operand, op, return_type.value());
}

NODE_PROCESSOR(CastExpression)
{
    auto cast = std::dynamic_pointer_cast<CastExpression>(tree);
    auto expr = TRY_AND_CAST(BoundExpression, process(cast->expression(), ctx));
    auto type_maybe = cast->type()->resolve_type();
    if (type_maybe.is_error()) {
        auto err = type_maybe.error();
        return SyntaxError { err.code(), cast->token(), err.message() };
    }
    auto type = type_maybe.value();
    if (expr->type()->can_cast_to(type) == ObjectType::CanCast::Never)
        return SyntaxError { ErrorCode::TypeMismatch, cast->token(), format("Cannot cast {} to {}", expr->type(), type) };
    return std::make_shared<BoundCastExpression>(cast->token(), expr, type_maybe.value());
}

NODE_PROCESSOR(ExpressionList)
{
    auto list = std::dynamic_pointer_cast<ExpressionList>(tree);
    BoundExpressions bound_expressions;
    for (auto const& expr : list->expressions()) {
        auto bound_expr = TRY_AND_CAST(BoundExpression, process(expr, ctx));
        bound_expressions.push_back(bound_expr);
    }
    return std::make_shared<BoundExpressionList>(tree->token(), bound_expressions);
}

NODE_PROCESSOR(Import)
{
    return tree;
}

NODE_PROCESSOR(Variable)
{
    auto variable = std::dynamic_pointer_cast<Variable>(tree);
    auto declaration_maybe = ctx.get(variable->name());
    if (!declaration_maybe.has_value()) {
        auto type = ObjectType::get(variable->name());
        if (type->type() != PrimitiveType::Unknown)
            return std::make_shared<BoundTypeLiteral>(variable->token(), type);
    } else {
        auto declaration = std::dynamic_pointer_cast<BoundVariableDeclaration>(declaration_maybe.value());
        if (declaration != nullptr)
            return std::make_shared<BoundVariable>(variable, declaration->type());
        auto func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(declaration_maybe.value());
        if (func_decl != nullptr)
            return std::make_shared<BoundLocalFunction>(variable->token(), variable->name());
        auto module = std::dynamic_pointer_cast<BoundModule>(declaration_maybe.value());
        if (module != nullptr)
            return module;
        return SyntaxError { ErrorCode::SyntaxError, variable->token(), format("Identifier '{}' cannot be referenced as a variable, it's a {}", variable->name(), declaration_maybe.value()) };
    }
    return tree;
}

NODE_PROCESSOR(IntLiteral)
{
    auto literal = std::dynamic_pointer_cast<IntLiteral>(tree);
    std::shared_ptr<ObjectType> type = ObjectType::get("s64");
    if (literal->is_typed()) {
         auto type_or_error = literal->type()->resolve_type();
         if (type_or_error.is_error())
            return SyntaxError { ErrorCode::NoSuchType, literal->token(), format("Unknown type '{}'", literal->type()) };
        type = type_or_error.value();
    }
    return std::make_shared<BoundIntLiteral>(std::dynamic_pointer_cast<IntLiteral>(tree), type);
}

NODE_PROCESSOR(StringLiteral)
{
    return std::make_shared<BoundStringLiteral>(std::dynamic_pointer_cast<StringLiteral>(tree));
}

NODE_PROCESSOR(BooleanLiteral)
{
    return std::make_shared<BoundBooleanLiteral>(std::dynamic_pointer_cast<BooleanLiteral>(tree));
}

NODE_PROCESSOR(ExpressionStatement)
{
    auto expr_stmt = std::dynamic_pointer_cast<ExpressionStatement>(tree);
    auto expr = TRY(process(expr_stmt->expression(), ctx));
    auto bound_expr = std::dynamic_pointer_cast<BoundExpression>(expr);
    if (bound_expr == nullptr)
        return tree;
    return std::make_shared<BoundExpressionStatement>(expr_stmt, bound_expr);
}

NODE_PROCESSOR(Return)
{
    auto ret_stmt = std::dynamic_pointer_cast<Return>(tree);
    auto bound_expr = TRY_AND_CAST(BoundExpression, process(ret_stmt->expression(), ctx));
    if (ctx.return_type) {
        if (!bound_expr) {
            return tree;
        }
        if (ctx.return_type->type() != bound_expr->type()->type())
            return SyntaxError { ErrorCode::TypeMismatch, ret_stmt->token(), bound_expr->to_string(), ctx.return_type->name(), bound_expr->type_name() };
    } else {
        if (bound_expr)
            return SyntaxError { ErrorCode::SyntaxError, ret_stmt->token(), format("Expected void return, got a return value of type '{}'", bound_expr->type_name()) };
    }
    return std::make_shared<BoundReturn>(ret_stmt, bound_expr);
}

NODE_PROCESSOR(Branch)
{
    auto branch = std::dynamic_pointer_cast<Branch>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, process(branch->condition(), ctx));
    if (bound_condition == nullptr)
        return tree;
    auto bound_statement = TRY_AND_CAST(Statement, process(branch->statement(), ctx));
    return std::make_shared<BoundBranch>(branch, bound_condition, bound_statement);
}

NODE_PROCESSOR(IfStatement)
{
    auto if_stmt = std::dynamic_pointer_cast<IfStatement>(tree);

    BoundBranches bound_branches;
    for (auto& branch : if_stmt->branches()) {
        auto bound_branch = TRY_AND_CAST(BoundBranch, process(branch, ctx));
        bound_branches.push_back(bound_branch);
    }
    auto bound_else_stmt = TRY_AND_CAST(Statement, process(if_stmt->else_stmt(), ctx));
    return std::make_shared<BoundIfStatement>(if_stmt, bound_branches, bound_else_stmt);
}

NODE_PROCESSOR(WhileStatement)
{
    auto stmt = std::dynamic_pointer_cast<WhileStatement>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, process(stmt->condition(), ctx));
    if (bound_condition == nullptr)
        return tree;
    auto bound_statement = TRY_AND_CAST(Statement, process(stmt->statement(), ctx));
    return std::make_shared<BoundWhileStatement>(stmt, bound_condition, bound_statement);
}

NODE_PROCESSOR(ForStatement)
{
    auto stmt = std::dynamic_pointer_cast<ForStatement>(tree);

    bool must_declare_variable = true;
    auto t = stmt->variable()->type();
    std::shared_ptr<ObjectType> var_type { nullptr };
    if (t) {
        auto var_type_maybe = t->resolve_type();
        if (var_type_maybe.is_error()) {
            auto err = var_type_maybe.error();
            return SyntaxError { err.code(), stmt->variable()->token(), err.message() };
        }
        var_type = var_type_maybe.value();
    } else {
        auto var_decl_maybe = ctx.get(stmt->variable()->name());
        if (var_decl_maybe.has_value()) {
            auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(var_decl_maybe.value());
            if (var_decl != nullptr) {
                var_type = var_decl->type();
                must_declare_variable = false;
            }
        }
    }

    auto bound_range = TRY_AND_CAST(BoundExpression, process(stmt->range(), ctx));
    if (bound_range == nullptr)
        return tree;
    auto range_binary_expr = std::dynamic_pointer_cast<BoundBinaryExpression>(bound_range);
    if (range_binary_expr == nullptr)
        return SyntaxError { ErrorCode::SyntaxError, stmt->token(), "Invalid for-loop range" };
    if (range_binary_expr->op() != BinaryOperator::Range)
        return SyntaxError { ErrorCode::SyntaxError, stmt->token(), "Invalid for-loop range" };
    auto range_type = range_binary_expr->lhs()->type();

    if (var_type && var_type->type() != PrimitiveType::Any && !range_type->is_assignable_to(var_type)) {
        auto int_literal = std::dynamic_pointer_cast<BoundIntLiteral>(range_binary_expr->lhs());
        if (int_literal != nullptr) {
            auto casted = TRY(int_literal->cast(var_type));
            range_binary_expr = std::make_shared<BoundBinaryExpression>(range_binary_expr->token(), casted,
                range_binary_expr->op(), range_binary_expr->rhs(), range_binary_expr->type());
        } else {
            return SyntaxError { ErrorCode::TypeMismatch, stmt->token(), stmt->variable()->name(), var_type, range_type };
        }
    }
    if (var_type == nullptr) {
        var_type = range_type;
    }

    BindContext for_ctx(ctx);
    auto bound_var_decl = std::make_shared<BoundVariableDeclaration>(stmt->token(),
        std::make_shared<BoundIdentifier>(stmt->token(), stmt->variable()->name(), var_type),
        false, nullptr);
    auto bound_var = std::make_shared<BoundVariable>(stmt->variable(), var_type);
    if (must_declare_variable)
        for_ctx.declare(stmt->variable()->name(), bound_var_decl);
    auto bound_statement = TRY_AND_CAST(Statement, process(stmt->statement(), for_ctx));
    return std::make_shared<BoundForStatement>(stmt, bound_var, range_binary_expr, bound_statement, must_declare_variable);
}

NODE_PROCESSOR(CaseStatement)
{
    auto branch = std::dynamic_pointer_cast<CaseStatement>(tree);
    auto bound_condition = TRY_AND_CAST(BoundExpression, process(branch->condition(), ctx));
    if (bound_condition == nullptr)
        return tree;
    auto bound_statement = TRY_AND_CAST(Statement, process(branch->statement(), ctx));
    return std::make_shared<BoundBranch>(branch, bound_condition, bound_statement);
}

NODE_PROCESSOR(DefaultCase)
{
    auto branch = std::dynamic_pointer_cast<DefaultCase>(tree);
    auto bound_statement = TRY_AND_CAST(Statement, process(branch->statement(), ctx));
    return std::make_shared<BoundBranch>(branch, nullptr, bound_statement);
}

NODE_PROCESSOR(SwitchStatement)
{
    auto stmt = std::dynamic_pointer_cast<SwitchStatement>(tree);
    auto bound_expression = TRY_AND_CAST(BoundExpression, process(stmt->expression(), ctx));
    if (bound_expression == nullptr)
        return tree;
    BoundBranches bound_case_statements;
    for (auto& case_stmt : stmt->cases()) {
        auto bound_case = TRY_AND_CAST(BoundBranch, process(case_stmt, ctx));
        bound_case_statements.push_back(bound_case);
    }
    auto bound_default_case = TRY_AND_CAST(BoundBranch, process(stmt->default_case(), ctx));
    return std::make_shared<BoundSwitchStatement>(stmt, bound_expression, bound_case_statements, bound_default_case);
}

ErrorOrNode bind_types(std::shared_ptr<SyntaxNode> const& tree, Config const& config)
{
    BindContext root;
    for (root.stage = 0; root.stage < 2; ++root.stage) {
        root.clear_unresolved_functions();
        auto ret = process(tree, root);

        if (ret.is_error() || root.unresolved_functions().empty()) {
            if (!ret.is_error() && config.cmdline_flag("show-tree"))
                std::cout << "\n\nTypes bound:\n" << ret.value()->to_xml() << "\n";
            return ret;
        }
        if (config.cmdline_flag("show-tree") && !root.unresolved_functions().empty()) {
            std::cout << "Unresolved after stage " << root.stage << ":\n";
            for (auto const& unresolved : root.unresolved_functions()) {
                std::cout << unresolved.first->to_string() << "(...)\n";
            }
            std::cout << "Current declared functions:\n";
            for (auto const& f : root.declared_functions()) {
                std::cout << f.second->to_string() << "\n";
            }
        }
    }
    auto func_call = root.unresolved_functions().back();
    // FIXME Deal with multiple unresolved functions
    return SyntaxError { ErrorCode::UntypedFunction, func_call.first->token(), func_call.first->name() };
}

}
