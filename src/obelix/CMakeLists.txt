add_executable(
        obelix2
        main.cpp
        BoundFunction.h
        Scope.h
        Syntax.h
        BoundFunction.cpp)

target_link_libraries(
        obelix2
        obllexer
        oblcore
        ${LIBS}
)

