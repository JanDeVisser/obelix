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
        BindTypes.cpp
        BoundSyntaxNode.cpp
        Config.cpp
        FoldConstants.cpp
        Lower.cpp
        Parser.cpp
        ResolveOperators.cpp
        Syntax.cpp
        BoundSyntaxNode.h
        Context.h
        Symbol.h
        SyntaxNodeType.h
        arm64/ARM64.cpp
        arm64/ARM64Context.cpp
        arm64/ARM64Intrinsics.cpp
        arm64/ARM64Materialize.cpp
        arm64/MaterializedSyntaxNode.cpp
        arm64/Mnemonic.cpp
        arm64/VariableAddress.cpp
        interp/InterpIntrinsics.cpp
        interp/Interpret.cpp
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
