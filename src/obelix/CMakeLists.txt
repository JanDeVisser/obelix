add_executable(
        obelix2
        main.cpp
)

target_link_libraries(
        obelix2
        obllexer
        oblcore
        ${LIBS}
)

