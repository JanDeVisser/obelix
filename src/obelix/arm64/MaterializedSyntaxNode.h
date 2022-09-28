/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <obelix/BoundSyntaxNode.h>
#include <obelix/arm64/VariableAddress.h>

namespace Obelix {

class MaterializedDeclaration {
public:
    explicit MaterializedDeclaration() = default;

    [[nodiscard]] virtual std::shared_ptr<VariableAddress> address() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ObjectType> const& declared_type() const = 0;
};

NODE_CLASS(MaterializedFunctionParameter, BoundIdentifier, public MaterializedDeclaration)
public:
    enum class ParameterPassingMethod {
        Register,
        Stack
    };

    MaterializedFunctionParameter(std::shared_ptr<BoundIdentifier> const&, std::shared_ptr<VariableAddress>, ParameterPassingMethod, int);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<ObjectType> const& declared_type() const override;
    [[nodiscard]] std::shared_ptr<VariableAddress> address() const override;
    [[nodiscard]] ParameterPassingMethod method() const;
    [[nodiscard]] int where() const;

private:
    std::shared_ptr<VariableAddress> m_address;
    ParameterPassingMethod m_method;
    int m_where;
};

using MaterializedFunctionParameters = std::vector<std::shared_ptr<MaterializedFunctionParameter>>;

NODE_CLASS(MaterializedFunctionDecl, Statement)
public:
    explicit MaterializedFunctionDecl(std::shared_ptr<BoundFunctionDecl> const&, MaterializedFunctionParameters, int, int);
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> type() const;
    [[nodiscard]] MaterializedFunctionParameters const& parameters() const;
    [[nodiscard]] int nsaa() const;
    [[nodiscard]] int stack_depth() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string label() const;

protected:
    [[nodiscard]] std::string parameters_to_string() const;

private:
    std::shared_ptr<BoundIdentifier> m_identifier;
    MaterializedFunctionParameters m_parameters;
    int m_nsaa;
    int m_stack_depth;
};

NODE_CLASS(MaterializedNativeFunctionDecl, MaterializedFunctionDecl)
public:
    explicit MaterializedNativeFunctionDecl(std::shared_ptr<BoundNativeFunctionDecl> const&, MaterializedFunctionParameters, int);
    [[nodiscard]] std::string const& native_function_name() const;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_native_function_name;
};

NODE_CLASS(MaterializedIntrinsicDecl, MaterializedFunctionDecl)
public:
    MaterializedIntrinsicDecl(std::shared_ptr<BoundIntrinsicDecl> const&, MaterializedFunctionParameters, int);
    [[nodiscard]] std::string to_string() const override;
};

NODE_CLASS(MaterializedFunctionDef, Statement)
public:
    MaterializedFunctionDef(std::shared_ptr<BoundFunctionDef> const&, std::shared_ptr<MaterializedFunctionDecl>, std::shared_ptr<Statement>, int);
    [[nodiscard]] std::shared_ptr<MaterializedFunctionDecl> const& declaration() const;
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& identifier() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const;
    [[nodiscard]] MaterializedFunctionParameters const& parameters() const;
    [[nodiscard]] std::shared_ptr<Statement> const& statement() const;
    [[nodiscard]] int stack_depth() const;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string label() const;

protected:
    std::shared_ptr<MaterializedFunctionDecl> m_function_decl;
    std::shared_ptr<Statement> m_statement;
    int m_stack_depth { 0 };
};

NODE_CLASS(MaterializedFunctionCall, BoundExpression)
public:
    MaterializedFunctionCall(std::shared_ptr<BoundFunctionCall> const&, BoundExpressions, std::shared_ptr<MaterializedFunctionDecl>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] BoundExpressions const& arguments() const;
    [[nodiscard]] std::shared_ptr<MaterializedFunctionDecl> const& declaration() const;
    [[nodiscard]] ObjectTypes argument_types() const;

private:
    std::string m_name;
    BoundExpressions m_arguments;
    std::shared_ptr<MaterializedFunctionDecl> m_declaration;
};

NODE_CLASS(MaterializedNativeFunctionCall, MaterializedFunctionCall)
public:
    MaterializedNativeFunctionCall(std::shared_ptr<BoundFunctionCall> const&, BoundExpressions, std::shared_ptr<MaterializedNativeFunctionDecl>);
};

NODE_CLASS(MaterializedIntrinsicCall, MaterializedFunctionCall)
public:
    MaterializedIntrinsicCall(std::shared_ptr<BoundFunctionCall> const&, BoundExpressions, std::shared_ptr<MaterializedIntrinsicDecl>, IntrinsicType);
    [[nodiscard]] IntrinsicType intrinsic() const;

private:
    IntrinsicType m_intrinsic;
};

NODE_CLASS(MaterializedVariableDecl, Statement, public MaterializedDeclaration)
public:
    MaterializedVariableDecl(std::shared_ptr<BoundVariableDeclaration> const&, size_t, std::shared_ptr<BoundExpression>);
    MaterializedVariableDecl(std::shared_ptr<BoundVariableDeclaration> const&, std::shared_ptr<BoundExpression>);
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] std::shared_ptr<BoundIdentifier> const& variable() const;
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::shared_ptr<ObjectType> const& type() const;
    [[nodiscard]] bool is_const() const;
    [[nodiscard]] size_t offset() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& expression() const;
    [[nodiscard]] std::shared_ptr<VariableAddress> address() const override;
    [[nodiscard]] std::shared_ptr<ObjectType> const& declared_type() const override;

private:
    std::shared_ptr<BoundIdentifier> m_variable;
    bool m_const { false };
    std::shared_ptr<BoundExpression> m_expression;
    size_t m_offset { 0 };
};

NODE_CLASS(MaterializedStaticVariableDecl, MaterializedVariableDecl)
public:
    MaterializedStaticVariableDecl(std::shared_ptr<BoundVariableDeclaration> const&, std::shared_ptr<BoundExpression>);
    [[nodiscard]] virtual std::string label() const;
    [[nodiscard]] std::shared_ptr<VariableAddress> address() const override;
};

NODE_CLASS(MaterializedLocalVariableDecl, MaterializedStaticVariableDecl)
public:
    MaterializedLocalVariableDecl(std::shared_ptr<BoundVariableDeclaration> const&, std::shared_ptr<BoundExpression>);
};

NODE_CLASS(MaterializedGlobalVariableDecl, MaterializedStaticVariableDecl)
public:
    MaterializedGlobalVariableDecl(std::shared_ptr<BoundVariableDeclaration> const&, std::shared_ptr<BoundExpression>);
};

ABSTRACT_NODE_CLASS(MaterializedVariableAccess, BoundVariableAccess)
public:
    MaterializedVariableAccess(std::shared_ptr<BoundExpression> const&, std::shared_ptr<VariableAddress>);
    [[nodiscard]] std::shared_ptr<VariableAddress> const& address() const;

private:
    std::shared_ptr<VariableAddress> m_address;
};

NODE_CLASS(MaterializedIdentifier, MaterializedVariableAccess)
public:
    MaterializedIdentifier(std::shared_ptr<BoundIdentifier> const&, std::shared_ptr<VariableAddress>);
    [[nodiscard]] std::string const& name() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::string m_identifier;
};

NODE_CLASS(MaterializedIntIdentifier, MaterializedIdentifier)
public:
    MaterializedIntIdentifier(std::shared_ptr<BoundIdentifier> const&, std::shared_ptr<VariableAddress>);
};

NODE_CLASS(MaterializedStructIdentifier, MaterializedIdentifier)
public:
    MaterializedStructIdentifier(std::shared_ptr<BoundIdentifier> const&, std::shared_ptr<VariableAddress>);
};

NODE_CLASS(MaterializedArrayIdentifier, MaterializedIdentifier)
public:
    MaterializedArrayIdentifier(std::shared_ptr<BoundIdentifier> const&, std::shared_ptr<VariableAddress>);
};

NODE_CLASS(MaterializedMemberAccess, MaterializedVariableAccess)
public:
    MaterializedMemberAccess(std::shared_ptr<BoundMemberAccess> const&, std::shared_ptr<MaterializedVariableAccess>, std::shared_ptr<MaterializedIdentifier>);
    [[nodiscard]] std::shared_ptr<MaterializedVariableAccess> const& structure() const;
    [[nodiscard]] std::shared_ptr<MaterializedIdentifier> const& member() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;

private:
    std::shared_ptr<MaterializedVariableAccess> m_struct;
    std::shared_ptr<MaterializedIdentifier> m_member;
};

NODE_CLASS(MaterializedArrayAccess, MaterializedVariableAccess)
public:
    MaterializedArrayAccess(std::shared_ptr<BoundArrayAccess> const&, std::shared_ptr<MaterializedVariableAccess>, std::shared_ptr<BoundExpression>, int);
    [[nodiscard]] std::shared_ptr<MaterializedVariableAccess> const& array() const;
    [[nodiscard]] std::shared_ptr<BoundExpression> const& index() const;
    [[nodiscard]] std::string attributes() const override;
    [[nodiscard]] Nodes children() const override;
    [[nodiscard]] std::string to_string() const override;
    [[nodiscard]] int element_size() const;

private:
    std::shared_ptr<MaterializedVariableAccess> m_array;
    int m_element_size;
    std::shared_ptr<BoundExpression> m_index;
};

}
