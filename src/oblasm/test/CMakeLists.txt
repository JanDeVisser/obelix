enable_testing()

add_executable(
        AsmTest
        Directive.cpp
        Misc.cpp
        Mov.cpp
)

target_link_libraries(
        AsmTest
        gtest_main
        oblasm
)

include(GoogleTest)
gtest_discover_tests(AsmTest)
