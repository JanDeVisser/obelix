add_executable(
        obelix2
        main.cpp
        Scope.h)

target_link_libraries(
        obelix2
        obllexer
        oblcore
        ${LIBS}
)

