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
#include <utility>

namespace Obelix {

class MaterializeContext : public Context<std::shared_ptr<SyntaxNode>>
{
public:
    enum class ContextLevel {
        Global,
        Function,
        Block,
    };

    MaterializeContext()
        : Context()
        , m_level(ContextLevel::Global)
    {
    }

    MaterializeContext(ContextLevel level)
        : Context()
        , m_level(level)
    {
    }

    MaterializeContext(MaterializeContext& parent)
        : Context(dynamic_cast<Context&>(parent))
    {
    }

    MaterializeContext(MaterializeContext& parent, int offset)
        : Context(dynamic_cast<Context&>(parent))
        , m_offset(offset)
        , m_level(ContextLevel::Function)
    {
    }

    [[nodiscard]] int offset() const
    {
        if (level() == ContextLevel::Block) {
            return dynamic_cast<MaterializeContext*>(parent())->offset();
        }
        return m_offset;
    };

    void increase_offset(int increment)
    {
        if (level() == ContextLevel::Block) {
            dynamic_cast<MaterializeContext*>(parent())->increase_offset(increment);
            return;
        }
        if (increment % 16)
            increment += 16 - (increment % 16);
        m_offset += increment;;
    };

    [[nodiscard]] ContextLevel level() const { return m_level; }

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
    int m_offset { 0 };
    ContextLevel m_level { ContextLevel::Block };
    std::vector<std::shared_ptr<FunctionCall>> m_unresolved_functions;
    std::unordered_map<std::string, std::shared_ptr<MaterializedFunctionDecl>> m_declared_functions;
};

INIT_NODE_PROCESSOR(MaterializeContext);

struct ParameterMaterializations {
    MaterializedFunctionParameters function_parameters;
    int offset { 0 };
    int ngrn { 0 };
    int nsaa { 0 };
};

ParameterMaterializations make_materialized_parameters(std::shared_ptr<BoundFunctionDecl> func_decl)
{
    ParameterMaterializations ret;
    for (auto const& parameter : func_decl->parameters()) {
        MaterializedFunctionParameter::ParameterPassingMethod method;
        int where;
        auto primitive_type = parameter->type()->type();
        if (primitive_type == PrimitiveType::Compatible)
            primitive_type = func_decl->parameter_types()[0]->type();
        switch (primitive_type) {
            case PrimitiveType::Boolean:
            case PrimitiveType::IntegerNumber:
            case PrimitiveType::SignedIntegerNumber:
            case PrimitiveType::Pointer:
                if (ret.ngrn < 8) {
                    method = MaterializedFunctionParameter::ParameterPassingMethod::Register;
                    where = ret.ngrn++;
                    break;
                }
                method = MaterializedFunctionParameter::ParameterPassingMethod::Stack;
                ret.nsaa += 8;
                where = ret.nsaa;
                break;
            case PrimitiveType::Struct: {
                auto size_in_double_words = parameter->type()->size() / 8;
                if (parameter->type()->size() % 8)
                    size_in_double_words++;
                if (ret.ngrn + size_in_double_words <= 8) {
                    method = MaterializedFunctionParameter::ParameterPassingMethod::Register;
                    where = ret.ngrn;
                    ret.ngrn += size_in_double_words;
                    break;
                }
                method = MaterializedFunctionParameter::ParameterPassingMethod::Stack;
                ret.nsaa += parameter->type()->size();
                where = ret.nsaa;
                break;
            }
            default:
                fatal("Type '{}' Not yet implemented in make_materialized_parameters", parameter->type());
        }
    
        ret.offset += parameter->type()->size();
        if (ret.offset % 16)
            ret.offset += 16 - (ret.offset % 16);
        auto materialized_parameter = make_node<MaterializedFunctionParameter>(parameter, std::make_shared<StackVariableAddress>(ret.offset), method, where);
        ret.function_parameters.push_back(materialized_parameter);
    }
    return ret;
}

NODE_PROCESSOR(BoundFunctionDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundFunctionDecl>(tree);
    auto materialized_parameters = make_materialized_parameters(func_decl);
    auto ret = make_node<MaterializedFunctionDecl>(func_decl, 
        materialized_parameters.function_parameters, materialized_parameters.nsaa, materialized_parameters.offset);
    ctx.declare(func_decl->name(), ret);
    ctx.add_declared_function(func_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundNativeFunctionDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(tree);
    auto materialized_parameters = make_materialized_parameters(func_decl);
    auto ret = make_node<MaterializedNativeFunctionDecl>(func_decl, 
        materialized_parameters.function_parameters, materialized_parameters.nsaa);
    ctx.declare(func_decl->name(), ret);
    ctx.add_declared_function(func_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundIntrinsicDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundIntrinsicDecl>(tree);
    auto materialized_parameters = make_materialized_parameters(func_decl);
    auto ret = make_node<MaterializedIntrinsicDecl>(func_decl, 
        materialized_parameters.function_parameters, materialized_parameters.nsaa);
    ctx.declare(func_decl->name(), ret);
    ctx.add_declared_function(func_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    auto func_decl = TRY_AND_CAST(MaterializedFunctionDecl, process(func_def->declaration(), ctx));

    MaterializeContext func_ctx(ctx, func_decl->stack_depth());
    for (auto const& param : func_decl->parameters()) {
        func_ctx.declare(param->name(), param);
    }
    std::shared_ptr<Block> block;
    assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
    block = TRY_AND_CAST(FunctionBlock, process(func_def->statement(), func_ctx));
    return make_node<MaterializedFunctionDef>(func_def, func_decl, block, func_ctx.offset());
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
    auto expression = TRY_AND_CAST(BoundExpression, process(var_decl->expression(), ctx));
    ctx.increase_offset(var_decl->type()->size());
    auto ret = make_node<MaterializedVariableDecl>(var_decl, std::make_shared<StackVariableAddress>(ctx.offset()), expression);
    ctx.declare(var_decl->name(), ret);
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

std::shared_ptr<MaterializedIdentifier> make_materialized_identifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> const& address)
{
    switch (identifier->type()->type()) {
    case PrimitiveType::Boolean:
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

NODE_PROCESSOR(BoundVariable)
{
    auto identifier = std::dynamic_pointer_cast<BoundVariable>(tree);
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
