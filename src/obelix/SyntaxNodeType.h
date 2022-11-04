/*
 * Copyright (c) ${YEAR}, Jan de Visser <jan@finiandarcy.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <core/Format.h>
#include <core/Logging.h>

namespace Obelix {

#define ENUMERATE_SYNTAXNODETYPES(S)    \
    S(SyntaxNode)                       \
    S(Statement)                        \
    S(Block)                            \
    S(FunctionBlock)                    \
    S(Compilation)                      \
    S(Module)                           \
    S(ExpressionType)                   \
    S(StringTemplateArgument)           \
    S(IntegerTemplateArgument)          \
    S(Expression)                       \
    S(ExpressionList)                   \
    S(EnumValue)                        \
    S(EnumDef)                          \
    S(TypeDef)                          \
    S(IntLiteral)                       \
    S(CharLiteral)                      \
    S(FloatLiteral)                     \
    S(StringLiteral)                    \
    S(BooleanLiteral)                   \
    S(StructLiteral)                    \
    S(ArrayLiteral)                     \
    S(Identifier)                       \
    S(Variable)                         \
    S(This)                             \
    S(BinaryExpression)                 \
    S(UnaryExpression)                  \
    S(CastExpression)                   \
    S(Assignment)                       \
    S(FunctionCall)                     \
    S(Import)                           \
    S(Pass)                             \
    S(Label)                            \
    S(Goto)                             \
    S(FunctionDecl)                     \
    S(NativeFunctionDecl)               \
    S(IntrinsicDecl)                    \
    S(FunctionDef)                      \
    S(ExpressionStatement)              \
    S(VariableDeclaration)              \
    S(StaticVariableDeclaration)        \
    S(LocalVariableDeclaration)         \
    S(GlobalVariableDeclaration)        \
    S(StructDefinition)                 \
    S(StructForward)                    \
    S(Return)                           \
    S(Break)                            \
    S(Continue)                         \
    S(Branch)                           \
    S(IfStatement)                      \
    S(WhileStatement)                   \
    S(ForStatement)                     \
    S(CaseStatement)                    \
    S(DefaultCase)                      \
    S(SwitchStatement)                  \
    S(MaterializedFunctionParameter)    \
    S(MaterializedFunctionDecl)         \
    S(MaterializedFunctionDef)          \
    S(MaterializedVariableDecl)         \
    S(MaterializedStaticVariableDecl)   \
    S(MaterializedLocalVariableDecl)    \
    S(MaterializedGlobalVariableDecl)   \
    S(MaterializedNativeFunctionDecl)   \
    S(MaterializedIntrinsicDecl)        \
    S(MaterializedIdentifier)           \
    S(MaterializedIntIdentifier)        \
    S(MaterializedStructIdentifier)     \
    S(MaterializedArrayIdentifier)      \
    S(MaterializedMemberAccess)         \
    S(MaterializedArrayAccess)          \
    S(MaterializedStaticVariableAccess) \
    S(MaterializedFunctionCall)         \
    S(MaterializedNativeFunctionCall)   \
    S(MaterializedIntrinsicCall)        \
    S(StatementExecutionResult)         \
    S(BoundStatement)                   \
    S(BoundPass)                        \
    S(BoundExpression)                  \
    S(BoundExpressionList)              \
    S(BoundIdentifier)                  \
    S(BoundVariable)                    \
    S(BoundCompilation)                 \
    S(BoundModule)                      \
    S(BoundLocalFunction)               \
    S(BoundImportedFunction)            \
    S(BoundMemberAccess)                \
    S(BoundMemberAssignment)            \
    S(BoundArrayAccess)                 \
    S(BoundIntLiteral)                  \
    S(BoundFloatLiteral)                \
    S(BoundStringLiteral)               \
    S(BoundBooleanLiteral)              \
    S(BoundTypeLiteral)                 \
    S(BoundModuleLiteral)               \
    S(BoundBinaryExpression)            \
    S(BoundUnaryExpression)             \
    S(BoundCastExpression)              \
    S(BoundFunctionCall)                \
    S(BoundNativeFunctionCall)          \
    S(BoundIntrinsicCall)               \
    S(BoundVariableDeclaration)         \
    S(BoundStaticVariableDeclaration)   \
    S(BoundLocalVariableDeclaration)    \
    S(BoundGlobalVariableDeclaration)   \
    S(BoundType)                        \
    S(BoundStructDefinition)            \
    S(BoundEnumDef)                     \
    S(BoundEnumValueDef)                \
    S(BoundEnumValue)                   \
    S(BoundTypeDef)                     \
    S(BoundReturn)                      \
    S(BoundExpressionStatement)         \
    S(BoundBranch)                      \
    S(BoundIfStatement)                 \
    S(BoundWhileStatement)              \
    S(BoundForStatement)                \
    S(BoundSwitchStatement)             \
    S(BoundFunctionDecl)                \
    S(BoundNativeFunctionDecl)          \
    S(BoundIntrinsicDecl)               \
    S(BoundAssignment)                  \
    S(BoundConditionalValue)            \
    S(BoundFunctionDef)

enum class SyntaxNodeType {
#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type) type,
    ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE
    NodeList,
    Count
};

constexpr char const* SyntaxNodeType_name(SyntaxNodeType type)
{
    switch (type) {
#undef ENUM_SYNTAXNODETYPE
#define ENUM_SYNTAXNODETYPE(type) \
    case SyntaxNodeType::type:    \
        return #type;
        ENUMERATE_SYNTAXNODETYPES(ENUM_SYNTAXNODETYPE)
#undef ENUM_SYNTAXNODETYPE
    case SyntaxNodeType::NodeList:    \
        return "NodeList";
    default:
        fatal("Unknown SyntaxNodeType value '{}'", (int)type);
    }
}

template<>
struct Converter<SyntaxNodeType> {
    static std::string to_string(SyntaxNodeType val)
    {
        return SyntaxNodeType_name(val);
    }

    static double to_double(SyntaxNodeType val)
    {
        return static_cast<double>(val);
    }

    static long to_long(SyntaxNodeType val)
    {
        return static_cast<long>(val);
    }
};

}
