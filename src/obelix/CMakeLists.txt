set(LIBS)
set(INCLUDES)
find_package(Readline)
if(READLINE_FOUND)
    set(LIBS ${READLINE_LIBRARIES} ${LIBS})
    set(INCLUDES ${INCLUDES} ${READLINE_INCLUDE_DIRS})
endif(READLINE_FOUND)

add_executable(
        obelix
        main.cpp
        Architecture.cpp
        Config.cpp
        FoldConstants.cpp
        Lower.cpp
        Parser.cpp
        ResolveOperators.cpp
        BoundSyntaxNode.h
        Context.h
        Symbol.h
        Syntax.h
        SyntaxNodeType.h
        arm64/ARM64.cpp
        arm64/ARM64Context.cpp
        arm64/ARM64Intrinsics.cpp
        arm64/ARM64Materialize.cpp
        arm64/MaterializedSyntaxNode.cpp
        arm64/Mnemonic.cpp
        arm64/VariableAddress.cpp
        bind/BindContext.cpp
        bind/BindTypes.cpp
        boundsyntax/ControlFlow.cpp
        boundsyntax/Expression.cpp
        boundsyntax/Function.cpp
        boundsyntax/Literal.cpp
        boundsyntax/Statement.cpp
        boundsyntax/Typedef.cpp
        boundsyntax/Variable.cpp
        interp/InterpIntrinsics.cpp
        interp/Interpret.cpp
        syntax/ControlFlow.cpp
        syntax/Expression.cpp
        syntax/Function.cpp
        syntax/Literal.cpp
        syntax/Statement.cpp
        syntax/Syntax.cpp
        syntax/Type.cpp
        syntax/Typedef.cpp
        syntax/Variable.cpp
        transpile/c/CTranspiler.cpp
        transpile/c/CTranspilerIntrinsics.cpp
        type/MethodDescription.cpp
        type/Template.cpp
        type/Type.cpp
)

target_link_libraries(
        obelix
        oblcore
        obllexer
        ${LIBS}
)

install(TARGETS obelix
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

# add_subdirectory(test)
