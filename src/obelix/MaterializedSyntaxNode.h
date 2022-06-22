/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <memory>
#include <string>

#include <obelix/BoundSyntaxNode.h>
#include <obelix/Syntax.h>
#include <obelix/SyntaxNodeType.h>

namespace Obelix {

#define ENUMERATE_VARIABLEADDRESSTYPES(S)  \
    S(StackVariableAddress)                \
    S(StaticVariableAddress)               \
    S(StructMemberAddress)                   

enum class VariableAddressType {
#undef __VARIABLEADDRESSTYPE
#define __VARIABLEADDRESSTYPE(type) type,
    ENUMERATE_VARIABLEADDRESSTYPES(__VARIABLEADDRESSTYPE)
#undef __VARIABLEADDRESSTYPE
};

constexpr char const* VariableAddressType_name(VariableAddressType type)
{
    switch (type) {
#undef __VARIABLEADDRESSTYPE
#define __VARIABLEADDRESSTYPE(type) \
    case VariableAddressType::type: \
        return #type;
        ENUMERATE_VARIABLEADDRESSTYPES(__VARIABLEADDRESSTYPE)
#undef __VARIABLEADDRESSTYPE
    }
}

template<>
struct Converter<VariableAddressType> {
    static std::string to_string(VariableAddressType val)
    {
        return VariableAddressType_name(val);
    }

    static double to_double(VariableAddressType val)
    {
        return static_cast<double>(val);
    }

    static long to_long(VariableAddressType val)
    {
        return static_cast<long>(val);
    }
};

class ARM64Context;

class VariableAddress {
public:
    virtual ~VariableAddress() = default;
    virtual std::string to_string() const = 0;
    virtual VariableAddressType address_type() const = 0;
    virtual ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const = 0;
    virtual ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const = 0;
    virtual ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const = 0;
};

class StackVariableAddress : public VariableAddress {
public:
    StackVariableAddress(int offset)
        : m_offset(offset)
    {
    }
    [[nodiscard]] int offset() const { return m_offset; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("StackVariableAddress: [{}]", m_offset);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StackVariableAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:    
    int m_offset;
};

struct StaticVariableAddress : public VariableAddress {
public:
    StaticVariableAddress(std::string label)
        : m_label(move(label))
    {
    }
    [[nodiscard]] std::string const& label() const { return m_label; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("StaticVariableAddress: [.{}]", m_label);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StaticVariableAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:
    std::string m_label;
};

struct StructMemberAddress : public VariableAddress {
public:
    StructMemberAddress(std::shared_ptr<VariableAddress> strukt, int offset)
        : m_struct(move(strukt))        
        , m_offset(offset)
    {
    }
    [[nodiscard]] std::shared_ptr<VariableAddress> const& structure() const { return m_struct; }
    [[nodiscard]] int offset() const { return m_offset; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("StructMemberAddress: [{}]", m_offset);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StructMemberAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:
    std::shared_ptr<VariableAddress> m_struct;
    int m_offset;
};

class ArrayElementAddress : public VariableAddress {
public:
    ArrayElementAddress(std::shared_ptr<VariableAddress> array, int element_size)
        : m_array(move(array))        
        , m_element_size(element_size)
    {
    }
    [[nodiscard]] std::shared_ptr<VariableAddress> const& array() const { return m_array; }
    [[nodiscard]] int element_size() const { return m_element_size; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("ArrayElementAddress: [{}]", m_element_size);
    }
    [[nodiscard]] VariableAddressType address_type() const override { return VariableAddressType::StructMemberAddress; }
    ErrorOr<void, SyntaxError> store_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> load_variable(std::shared_ptr<ObjectType>, ARM64Context&, int) const override;
    ErrorOr<void, SyntaxError> prepare_pointer(ARM64Context&) const override;

private:
    std::shared_ptr<VariableAddress> m_array;
    int m_element_size;
};

class MaterializedDeclaration {
public:
    MaterializedDeclaration(std::shared_ptr<VariableAddress> address)
        : m_address(move(address))
    {
    }

    [[nodiscard]] std::shared_ptr<VariableAddress> const& address() const { return m_address; }
    [[nodiscard]] virtual std::shared_ptr<ObjectType> const& declared_type() const = 0;
private:
    std::shared_ptr<VariableAddress> m_address;
};

class MaterializedFunctionParameter : public BoundIdentifier, public MaterializedDeclaration {
public:
    enum class ParameterPassingMethod {
        Register,
        Stack
    };

    MaterializedFunctionParameter(std::shared_ptr<BoundIdentifier> const& param, std::shared_ptr<VariableAddress> address, ParameterPassingMethod method, int where)
        : BoundIdentifier(param->token(), param->name(), param->type())
        , MaterializedDeclaration(move(address))
        , m_method(method)
        , m_where(where)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionParameter; }
    [[nodiscard]] std::string attributes() const override
    {
        return format(R"({} address="{}" where="{}")", BoundIdentifier::attributes(), address()->to_string(), where());
    }
    [[nodiscard]] std::string to_string() const override
    {
        return format("{} {} => {}", BoundIdentifier::to_string(), where(), address()->to_string());
    }

    [[nodiscard]] std::shared_ptr<ObjectType> const& declared_type() const override
    {
        return type();
    }

    [[nodiscard]] ParameterPassingMethod method() const { return m_method; }
    [[nodiscard]] int where() const { return m_where; }

private:
    ParameterPassingMethod m_method;
    int m_where;
};

using MaterializedFunctionParameters = std::vector<std::shared_ptr<MaterializedFunctionParameter>>;

class MaterializedFunctionDecl : public Statement {
public:
    explicit MaterializedFunctionDecl(std::shared_ptr<BoundFunctionDecl> const& decl, MaterializedFunctionParameters parameters, int nsaa)
        : Statement(decl->token())
        , m_identifier(decl->identifier())
        , m_parameters(move(parameters))
        , m_nsaa(nsaa)
    {
        if (m_nsaa % 16)
            m_nsaa += 16 - (m_nsaa % 16);
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionDecl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_identifier; }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> type() const { return identifier()->type(); }
    [[nodiscard]] MaterializedFunctionParameters const& parameters() const { return m_parameters; }
    [[nodiscard]] int nsaa() const { return m_nsaa; }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" return_type="{}" nsaa="{}")", name(), type(), nsaa());
    }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& param : m_parameters) {
            ret.push_back(param);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        return format("func {}({}): {} [{}/{}]", name(), parameters_to_string(), type(), nsaa());
    }

protected:
    [[nodiscard]] std::string parameters_to_string() const
    {
        std::string ret;
        bool first = true;
        for (auto& param : m_parameters) {
            if (!first)
                ret += ", ";
            first = false;
            ret += param->to_string();
        }
        return ret;
    }

private:
    std::shared_ptr<BoundIdentifier> m_identifier;
    MaterializedFunctionParameters m_parameters;
    int m_nsaa;
};

class MaterializedNativeFunctionDecl : public MaterializedFunctionDecl {
public:
    explicit MaterializedNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const& func_decl, MaterializedFunctionParameters parameters, int nsaa)
        : MaterializedFunctionDecl(func_decl, parameters, nsaa)
        , m_native_function_name(func_decl->native_function_name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedNativeFunctionDecl; }
    [[nodiscard]] std::string const& native_function_name() const { return m_native_function_name; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("{} -> \"{}\"", MaterializedFunctionDecl::to_string(), m_native_function_name);
    }

private:
    std::string m_native_function_name;
};

class MaterializedIntrinsicDecl : public MaterializedFunctionDecl {
public:
    explicit MaterializedIntrinsicDecl(std::shared_ptr<BoundIntrinsicDecl> const& decl, MaterializedFunctionParameters parameters, int nsaa)
        : MaterializedFunctionDecl(decl, parameters, nsaa)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedIntrinsicDecl; }
    [[nodiscard]] std::string to_string() const override
    {
        return format("intrinsic {}({}): {}", name(), parameters_to_string(), type());
    }
};

class MaterializedFunctionDef : public Statement {
public:
    MaterializedFunctionDef(std::shared_ptr<BoundFunctionDef> const& bound_def, std::shared_ptr<MaterializedFunctionDecl> func_decl, std::shared_ptr<Statement> statement, int stack_depth)
        : Statement(bound_def->token())
        , m_function_decl(move(func_decl))
        , m_statement(move(statement))
        , m_stack_depth(stack_depth)
    {
        if (m_stack_depth % 16)
            m_stack_depth += 16 - (m_stack_depth % 16);
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionDef; }
    [[nodiscard]] std::shared_ptr<MaterializedFunctionDecl> const& declaration() const { return m_function_decl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const { return m_function_decl->identifier(); }
    [[nodiscard]] std::string const& name() const { return identifier()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const { return identifier()->type(); }
    [[nodiscard]] MaterializedFunctionParameters const& parameters() const { return m_function_decl->parameters(); }
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const { return m_statement; }
    [[nodiscard]] int stack_depth() const { return m_stack_depth; }
    [[nodiscard]] std::string to_string() const override
    {
        if (m_statement != nullptr)
            return format("{}\n{}", m_function_decl, m_statement);
        return m_function_decl->to_string();
    }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret = { m_function_decl };
        if (m_statement) {
            ret.push_back(m_statement);
        }
        return ret;
    }

protected:
    std::shared_ptr<MaterializedFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
    int m_stack_depth { 0 };
};

class MaterializedFunctionCall : public BoundExpression {
public:
    MaterializedFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<MaterializedFunctionDecl> decl)
        : BoundExpression(call)
        , m_name(call->name())
        , m_arguments(move(arguments))
        , m_declaration(move(decl))
    {
    }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" type="{}")", name(), type());
    }

    [[nodiscard]] Nodes children() const override
    {
        Nodes ret;
        for (auto& arg : m_arguments) {
            ret.push_back(arg);
        }
        return ret;
    }

    [[nodiscard]] std::string to_string() const override
    {
        Strings args;
        for (auto& arg : m_arguments) {
            args.push_back(arg->to_string());
        }
        return format("{}({}): {}", name(), join(args, ","), type());
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedFunctionCall; }
    [[nodiscard]] std::string const& name() const { return m_name; }
    [[nodiscard]] BoundExpressions const& arguments() const { return m_arguments; }
    [[nodiscard]] std::shared_ptr<MaterializedFunctionDecl> const& declaration() const { return m_declaration; }

    [[nodiscard]] ObjectTypes argument_types() const
    {
        ObjectTypes ret;
        for (auto& arg : arguments()) {
            ret.push_back(arg->type());
        }
        return ret;
    }

private:
    std::string m_name;
    BoundExpressions m_arguments;
    std::shared_ptr<MaterializedFunctionDecl> m_declaration;
};

class MaterializedNativeFunctionCall : public MaterializedFunctionCall {
public:
    MaterializedNativeFunctionCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<MaterializedNativeFunctionDecl> decl)
        : MaterializedFunctionCall(call, move(arguments), move(decl))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedNativeFunctionCall; }
};

class MaterializedIntrinsicCall : public MaterializedFunctionCall {
public:
    MaterializedIntrinsicCall(std::shared_ptr<BoundFunctionCall> const& call, BoundExpressions arguments, std::shared_ptr<MaterializedIntrinsicDecl> decl, IntrinsicType intrinsic)
        : MaterializedFunctionCall(call, move(arguments), move(decl))
        , m_intrinsic(intrinsic)
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedIntrinsicCall; }
    [[nodiscard]] IntrinsicType intrinsic() const { return m_intrinsic; }

private:
    IntrinsicType m_intrinsic;
};

class MaterializedVariableDecl : public Statement, public MaterializedDeclaration {
public:
    explicit MaterializedVariableDecl(std::shared_ptr<BoundVariableDeclaration> const& var_decl, std::shared_ptr<VariableAddress> address, std::shared_ptr<BoundExpression> expression)
        : Statement(var_decl->token())
        , MaterializedDeclaration(move(address))
        , m_variable(var_decl->variable())
        , m_const(var_decl->is_const())
        , m_expression(move(expression))
    {
    }

    [[nodiscard]] std::string attributes() const override
    {
        return format(R"(name="{}" type="{}" is_const="{}" address="{}")", name(), type(), is_const(), address()->to_string());
    }

    [[nodiscard]] Nodes children() const override
    {
        if (m_expression)
            return { m_expression };
        return {};
    }

    [[nodiscard]] std::string to_string() const override
    {
        auto keyword = (m_const) ? "const" : "var";
        if (m_expression)
            return format("{} {}: {} {}", keyword, m_variable, m_expression, address()->to_string());
        return format("{} {} {}", keyword, m_variable, address()->to_string());
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedVariableDecl; }
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& variable() const { return m_variable; }
    [[nodiscard]] std::string const& name() const { return variable()->name(); }
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const { return variable()->type(); }
    [[nodiscard]] bool is_const() const { return m_const; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const { return m_expression; }

    [[nodiscard]] std::shared_ptr<ObjectType> const& declared_type() const override
    {
        return type();
    }

private:
    std::shared_ptr<BoundIdentifier> m_variable;
    bool m_const { false };
    std::shared_ptr<BoundExpression> m_expression;
};

class MaterializedVariableAccess : public BoundVariableAccess {
public:
    MaterializedVariableAccess(std::shared_ptr<BoundExpression> const& expr, std::shared_ptr<VariableAddress> address)
        : BoundVariableAccess(expr->token(), expr->type())
        , m_address(move(address))
    {
    }

    [[nodiscard]] std::shared_ptr<VariableAddress> const address() const { return m_address; }
private:
    std::shared_ptr<VariableAddress> m_address;
};

class MaterializedIdentifier : public MaterializedVariableAccess {
public:
    MaterializedIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
        : MaterializedVariableAccess(identifier, move(address))
        , m_identifier(identifier->name())
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedIdentifier; }
    [[nodiscard]] std::string const& name() const { return m_identifier; }
    [[nodiscard]] std::string attributes() const override { return format(R"(name="{}" type="{}" address="{}")", name(), type(), address()->to_string()); }
    [[nodiscard]] std::string to_string() const override { return format("{}: {} [{}]", name(), type()->to_string(), address()->to_string()); }

private:
    std::string m_identifier;
};

class MaterializedIntIdentifier : public MaterializedIdentifier {
public:
    MaterializedIntIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
        : MaterializedIdentifier(identifier, move(address))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedIntIdentifier; }
};

class MaterializedStructIdentifier : public MaterializedIdentifier {
public:
    MaterializedStructIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
        : MaterializedIdentifier(identifier, move(address))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedStructIdentifier; }
};

class MaterializedArrayIdentifier : public MaterializedIdentifier {
public:
    MaterializedArrayIdentifier(std::shared_ptr<BoundIdentifier> const& identifier, std::shared_ptr<VariableAddress> address)
        : MaterializedIdentifier(identifier, move(address))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedArrayIdentifier; }
};

class MaterializedMemberAccess : public MaterializedVariableAccess {
public:
    MaterializedMemberAccess(std::shared_ptr<BoundMemberAccess> const& member_access,
            std::shared_ptr<MaterializedVariableAccess> strukt, std::shared_ptr<MaterializedIdentifier> member)
        : MaterializedVariableAccess(member_access, member->address())
        , m_struct(move(strukt))
        , m_member(move(member))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedMemberAccess; }
    [[nodiscard]] std::shared_ptr<MaterializedVariableAccess> const& structure() const { return m_struct; }
    [[nodiscard]] std::shared_ptr<MaterializedIdentifier> const& member() const { return m_member; }
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{} address={}")", type(), address()->to_string()); }
    [[nodiscard]] Nodes children() const override { return { m_struct, m_member }; }
    [[nodiscard]] std::string to_string() const override { return format("{}.{}: {} {}", structure(), member(), type()->to_string(), address()->to_string()); }

private:
    std::shared_ptr<MaterializedVariableAccess> m_struct;
    std::shared_ptr<MaterializedIdentifier> m_member;
};

class MaterializedArrayAccess : public MaterializedVariableAccess {
public:
    MaterializedArrayAccess(std::shared_ptr<BoundArrayAccess> const& array_access,
            std::shared_ptr<MaterializedVariableAccess> array, std::shared_ptr<BoundExpression> index, int element_size)
        : MaterializedVariableAccess(array_access, std::make_shared<ArrayElementAddress>(array->address(), element_size))
        , m_array(move(array))
        , m_element_size(element_size)
        , m_index(move(index))
    {
    }

    [[nodiscard]] SyntaxNodeType node_type() const override { return SyntaxNodeType::MaterializedArrayAccess; }
    [[nodiscard]] std::shared_ptr<MaterializedVariableAccess> const& array() const { return m_array; }
    [[nodiscard]] std::shared_ptr<BoundExpression> const& index() const { return m_index; }
    [[nodiscard]] std::string attributes() const override { return format(R"(type="{}" element_size="{}")", type(), element_size()); }
    [[nodiscard]] Nodes children() const override { return { m_array, m_index }; }
    [[nodiscard]] std::string to_string() const override { return format("{}[{}]: {} {}", array(), index(), type()->to_string(), address()->to_string()); }
    [[nodiscard]] int element_size() const { return m_element_size; }

private:
    std::shared_ptr<MaterializedVariableAccess> m_array;
    int m_element_size;
    std::shared_ptr<BoundExpression> m_index;
};

}
