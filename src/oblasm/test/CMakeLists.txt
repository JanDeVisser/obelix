enable_testing()

add_executable(
        AsmTest
        Directive.cpp
        General.cpp
        Instructions.cpp)

target_link_libraries(
        AsmTest
        gtest_main
        oblasm
)

include(GoogleTest)
gtest_discover_tests(AsmTest)
