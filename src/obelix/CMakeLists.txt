set(LIBS)
set(INCLUDES)
find_package(Readline)
if(READLINE_FOUND)
    set(LIBS ${READLINE_LIBRARIES} ${LIBS})
    set(INCLUDES ${INCLUDES} ${READLINE_INCLUDE_DIRS})
endif(READLINE_FOUND)

add_library(
        oblparser
        SHARED
        BoundFunction.cpp
        Functions.cpp
        Parser.cpp
        Processor.cpp
        Runtime.cpp
        Scope.cpp
        Syntax.cpp
)

target_link_libraries(
        oblparser
        obllexer
)

add_executable(
        obelix
        main.cpp
)

target_link_libraries(
        obelix
        oblparser
        ${LIBS}
)

install(TARGETS oblparser obelix
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
