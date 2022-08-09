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
        ARM64.cpp
        ARM64Context.cpp
        ARM64Intrinsics.cpp
        ARM64Materialize.cpp
        BindTypes.cpp
        Config.cpp
        FoldConstants.cpp
        Lower.cpp
        Parser.cpp
        ResolveOperators.cpp
        Syntax.cpp
        Type.cpp
        BoundSyntaxNode.h
        Context.h
        MaterializedSyntaxNode.h
        Symbol.h
        SyntaxNodeType.h
)

target_link_libraries(
        obelix
        oblcpu
        oblasm
        ${LIBS}
)

install(TARGETS obelix
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

# add_subdirectory(test)
