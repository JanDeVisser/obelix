enable_testing()

add_executable(
        ParserTest
        Assignment.cpp
        DictionaryLiteral.cpp
        ListLiteral.cpp
        DeclareVar.cpp)

target_link_libraries(
        ParserTest
        gtest_main
        oblparser
)

include(GoogleTest)
gtest_discover_tests(ParserTest)