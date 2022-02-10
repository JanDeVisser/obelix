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
        BindTypes.cpp
        #BoundFunction.cpp
        #Execute.cpp
        FoldConstants.cpp
        Functions.cpp
        Intrinsics.cpp
        Lower.cpp
        OutputJV80.cpp
        Parser.cpp
        Syntax.cpp
        Symbol.h
        Context.h
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
