enable_testing()

add_executable(
        ParserTest
        FoldConstants.cpp
)

target_link_libraries(
        ParserTest
        gtest_main
        oblparser
)

include(GoogleTest)
gtest_discover_tests(ParserTest)
