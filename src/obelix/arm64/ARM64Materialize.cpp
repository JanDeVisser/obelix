/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory>

#include <obelix/arm64/ARM64.h>
#include <obelix/BoundSyntaxNode.h>
#include <obelix/Context.h>
#include <obelix/arm64/MaterializedSyntaxNode.h>
#include <obelix/Processor.h>
#include <obelix/Syntax.h>
#include <utility>

namespace Obelix {

class MaterializeContextPayload
{
public:
    enum class ContextLevel {
        Global,
        Module,
        Function,
        Block,
    };

    MaterializeContextPayload() = default;

    MaterializeContextPayload(ContextLevel level)
        : m_level(level)
    {
    }

    MaterializeContextPayload(int offset)
        : m_offset(offset)
        , m_level(ContextLevel::Function)
    {
    }

    [[nodiscard]] int offset() const
    {
        return m_offset;
    };

    void increase_offset(int increment)
    {
        if (increment % 16)
            increment += 16 - (increment % 16);
        m_offset += increment;
    };

    [[nodiscard]] ContextLevel level() const { return m_level; }

    void add_unresolved_function(std::shared_ptr<BoundFunctionCall> func_call)
    {
        m_unresolved_functions.push_back(func_call);
    }

    [[nodiscard]] std::vector<std::shared_ptr<BoundFunctionCall>> const& unresolved_functions() const
    {
        return m_unresolved_functions;
    }

    void clear_unresolved_functions()
    {
        m_unresolved_functions.clear();
    }

    void add_materialized_function(std::shared_ptr<MaterializedFunctionDecl> func)
    {
        m_materialized_functions.insert({ func->name(), func });
    }

    [[nodiscard]] std::multimap<std::string, std::shared_ptr<MaterializedFunctionDecl>> const& materialized_functions() const
    {
        return m_materialized_functions;
    }

    [[nodiscard]] std::shared_ptr<MaterializedFunctionDecl> match(std::string const& name, ObjectTypes const& arg_types) const
    {
        debug(parser, "matching function {}({})", name, arg_types);
        //        debug(parser, "Current materialized functions:");
        //        for (auto const& f : m_materialized_functions) {
        //            debug(parser, "{} {}", f.second->node_type(), f.second->to_string());
        //        }
        std::shared_ptr<MaterializedFunctionDecl> func_decl = nullptr;
        auto range = m_materialized_functions.equal_range(name);
        for (auto iter = range.first; iter != range.second; ++iter) {
            debug(parser, "checking {}({})", iter->first, iter->second->parameters());
            auto candidate = iter->second;
            if (arg_types.size() != candidate->parameters().size())
                continue;

            bool all_matched = true;
            for (auto ix = 0u; ix < arg_types.size(); ix++) {
                auto& arg_type = arg_types.at(ix);
                auto& param = candidate->parameters().at(ix);
                if (!arg_type->is_assignable_to(param->type())) {
                    all_matched = false;
                    break;
                }
            }
            if (all_matched) {
                func_decl = candidate;
                break;
            }
        }
        if (func_decl != nullptr)
            debug(parser, "match() returns {}", *func_decl);
        else
            debug(parser, "No matching function found");
        return func_decl;
    }

    void clear_materialized_functions()
    {
        m_materialized_functions.clear();
    }

private:
    int m_offset { 0 };
    ContextLevel m_level { ContextLevel::Global };
    std::vector<std::shared_ptr<BoundFunctionCall>> m_unresolved_functions;
    std::multimap<std::string, std::shared_ptr<MaterializedFunctionDecl>> m_materialized_functions;
};

using MaterializeContext = Context<std::shared_ptr<SyntaxNode>, MaterializeContextPayload>;

template <typename ...Args>
MaterializeContext& make_subcontext(MaterializeContext& ctx, Args&&... args)
{
    MaterializeContextPayload payload(std::forward<Args>(args)...);
    return ctx.make_subcontext(payload);
}

[[nodiscard]] int offset(MaterializeContext const& ctx)
{
    return ctx.call_on_ancestors<int>(
        [](MaterializeContext const& ctx) -> std::optional<int> {
            if (ctx().level() == MaterializeContextPayload::ContextLevel::Block)
                return {};
            return ctx().offset();
        }
    );
}

void increase_offset(MaterializeContext& ctx, int increment)
{
    ctx.ancestor_data(
        [](MaterializeContext const& ctx) -> bool {
               return (ctx().level() != MaterializeContextPayload::ContextLevel::Block);
        }).increase_offset(increment);
}

void add_unresolved_function(MaterializeContext& ctx, std::shared_ptr<BoundFunctionCall> func_call)
{
    ctx.call_on_root(
        [&func_call](MaterializeContext& ctx) -> void {
            ctx().add_unresolved_function(func_call);
        }
    );
}

[[nodiscard]] BoundFunctionCalls const& unresolved_functions(MaterializeContext const& ctx)
{
    return ctx.root_data().unresolved_functions();
}

void clear_unresolved_functions(MaterializeContext& ctx)
{
    ctx.root_data().clear_unresolved_functions();
}

void add_materialized_function(MaterializeContext& ctx, pMaterializedFunctionDecl func)
{
    ctx.root_data().add_materialized_function(std::move(func));
}

[[nodiscard]] std::multimap<std::string, pMaterializedFunctionDecl> const& materialized_functions(MaterializeContext const& ctx)
{
    return ctx.root_data().materialized_functions();
}

[[nodiscard]] pMaterializedFunctionDecl match(MaterializeContext const& ctx, std::string const& name, ObjectTypes const& arg_types)
{
    return ctx.root_data().match(name, arg_types);
}

void clear_materialized_functions(MaterializeContext& ctx)
{
    ctx.root_data().clear_materialized_functions();
}

INIT_NODE_PROCESSOR(MaterializeContext)

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
    add_materialized_function(ctx, ret);
    return ret;
}

NODE_PROCESSOR(BoundNativeFunctionDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundNativeFunctionDecl>(tree);
    auto materialized_parameters = make_materialized_parameters(func_decl);
    auto ret = make_node<MaterializedNativeFunctionDecl>(func_decl,
        materialized_parameters.function_parameters, materialized_parameters.nsaa);
    ctx.declare(func_decl->name(), ret);
    add_materialized_function(ctx, ret);
    return ret;
}

NODE_PROCESSOR(BoundIntrinsicDecl)
{
    auto func_decl = std::dynamic_pointer_cast<BoundIntrinsicDecl>(tree);
    auto materialized_parameters = make_materialized_parameters(func_decl);
    auto ret = make_node<MaterializedIntrinsicDecl>(func_decl,
        materialized_parameters.function_parameters, materialized_parameters.nsaa);
    ctx.declare(func_decl->name(), ret);
    add_materialized_function(ctx, ret);
    return ret;
}

NODE_PROCESSOR(BoundFunctionDef)
{
    auto func_def = std::dynamic_pointer_cast<BoundFunctionDef>(tree);
    auto func_decl = TRY_AND_CAST(MaterializedFunctionDecl, func_def->declaration(), ctx);

    auto& func_ctx = make_subcontext(ctx, func_decl->stack_depth());
    for (auto const& param : func_decl->parameters()) {
        func_ctx.declare(param->name(), param);
    }
    std::shared_ptr<Block> block;
    assert(func_def->statement()->node_type() == SyntaxNodeType::FunctionBlock);
    block = TRY_AND_CAST(FunctionBlock, func_def->statement(), func_ctx);
    return make_node<MaterializedFunctionDef>(func_def, func_decl, block, offset(func_ctx));
}

NODE_PROCESSOR(FunctionBlock)
{
    auto block = std::dynamic_pointer_cast<FunctionBlock>(tree);
    Statements statements;
    for (auto& stmt : block->statements()) {
        auto new_statement = TRY_AND_CAST(Statement, stmt, ctx);
        statements.push_back(new_statement);
    }
    return std::make_shared<FunctionBlock>(tree->token(), statements, block->declaration());
}

NODE_PROCESSOR(BoundVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundVariableDeclaration>(tree);
    auto expression = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
    increase_offset(ctx, var_decl->type()->size());
    auto ret = make_node<MaterializedVariableDecl>(var_decl, offset(ctx), expression);
    ctx.declare(var_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundStaticVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundStaticVariableDeclaration>(tree);
    auto expression = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
    auto ret = make_node<MaterializedStaticVariableDecl>(var_decl, expression);
    ctx.declare(var_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundLocalVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundLocalVariableDeclaration>(tree);
    auto expression = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
    auto ret = make_node<MaterializedLocalVariableDecl>(var_decl, expression);
    ctx.declare(var_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundGlobalVariableDeclaration)
{
    auto var_decl = std::dynamic_pointer_cast<BoundGlobalVariableDeclaration>(tree);
    auto expression = TRY_AND_CAST(BoundExpression, var_decl->expression(), ctx);
    auto ret = make_node<MaterializedGlobalVariableDecl>(var_decl, expression);
    ctx.declare(var_decl->name(), ret);
    return ret;
}

NODE_PROCESSOR(BoundFunctionCall)
{
    auto call = std::dynamic_pointer_cast<BoundFunctionCall>(tree);
    BoundExpressions arguments;
    ObjectTypes arg_types;
    for (auto& expr : call->arguments()) {
        auto new_expr = process(expr, ctx);
        if (new_expr.is_error())
            return new_expr.error();
        auto arg = std::dynamic_pointer_cast<BoundExpression>(new_expr.value());
        arguments.push_back(arg);
        arg_types.push_back(arg->type());
    }
    std::shared_ptr<MaterializedFunctionDecl> materialized_decl = match(ctx, call->name(), arg_types);
    switch (call->node_type()) {
    case SyntaxNodeType::BoundNativeFunctionCall:
        assert(materialized_decl != nullptr);
        return make_node<MaterializedNativeFunctionCall>(call, arguments, std::dynamic_pointer_cast<MaterializedNativeFunctionDecl>(materialized_decl));
    case SyntaxNodeType::BoundIntrinsicCall: {
        std::shared_ptr<MaterializedIntrinsicDecl> materialized;
        if (materialized_decl == nullptr) {
            materialized = TRY_AND_CAST(MaterializedIntrinsicDecl, call->declaration(), ctx);
            add_materialized_function(ctx, materialized);
            ctx.declare(call->name(), materialized);
        } else {
            materialized = std::dynamic_pointer_cast<MaterializedIntrinsicDecl>(materialized_decl);
            assert(materialized != nullptr);
        }
        auto intrinsic_call = std::dynamic_pointer_cast<BoundIntrinsicCall>(call);
        return std::make_shared<MaterializedIntrinsicCall>(call, arguments, materialized, intrinsic_call->intrinsic());
    }
    default:
        assert(materialized_decl != nullptr);
        return make_node<MaterializedFunctionCall>(call, arguments, materialized_decl);
    }
}

ALIAS_NODE_PROCESSOR(BoundNativeFunctionCall, BoundFunctionCall)
ALIAS_NODE_PROCESSOR(BoundIntrinsicCall, BoundFunctionCall)

std::shared_ptr<MaterializedIdentifier> make_materialized_identifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> const& address)
{
    switch (identifier->type()->type()) {
    case PrimitiveType::Boolean:
    case PrimitiveType::IntegerNumber:
    case PrimitiveType::SignedIntegerNumber:
    case PrimitiveType::Pointer:
    case PrimitiveType::Enum:
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
    auto strukt = TRY_AND_CAST(MaterializedVariableAccess, member_access->structure(), ctx);
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
    auto array = TRY_AND_CAST(MaterializedVariableAccess, array_access->array(), ctx);
    auto subscript = TRY_AND_CAST(BoundExpression, array_access->subscript(), ctx);
    auto type = array->type()->template_argument<std::shared_ptr<ObjectType>>("base_type");
    auto element_size = type->size();
    return make_node<MaterializedArrayAccess>(array_access, array, subscript, element_size);
}

ProcessResult materialize_arm64(std::shared_ptr<SyntaxNode> const& tree)
{
    Config config;
    MaterializeContext ctx(config);
    return process<MaterializeContext>(tree, ctx);
}

}
