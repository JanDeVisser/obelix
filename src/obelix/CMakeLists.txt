set(LIBS)
set(INCLUDES)
find_package(Readline)
if(READLINE_FOUND)
    set(LIBS ${READLINE_LIBRARIES} ${LIBS})
    set(INCLUDES ${INCLUDES} ${READLINE_INCLUDE_DIRS})
endif(READLINE_FOUND)

add_library(
        oblparser
        STATIC
        BindTypes.cpp
        #BoundFunction.cpp
        #Execute.cpp
        FoldConstants.cpp
        Functions.cpp
        Lower.cpp
        Parser.cpp
        Syntax.cpp
        Symbol.h
        Context.h)

target_link_libraries(
        oblparser
        obllexer
)

add_executable(
        obelix
        main.cpp
        MacOSX.cpp
        OutputJV80.cpp
)

target_link_libraries(
        obelix
        oblcpu
        oblasm
        oblparser
        ${LIBS}
)

install(TARGETS oblparser obelix
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

# add_subdirectory(test)
