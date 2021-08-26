add_executable(lexertest
        lexertest.cpp
        comment.cpp
        keyword.cpp
        number.cpp
        qstring.cpp
)

target_link_libraries(lexertest ${GTEST_LDFLAGS} obllexer obltest oblcore ${SYSLIBS})
target_compile_options(lexertest PUBLIC ${GTEST_CFLAGS})

install(TARGETS lexertest)

include(CTest)
add_test(obelix_tests lexertest)
