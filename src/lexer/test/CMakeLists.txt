enable_testing()

add_executable(
        LexerTest
        KeywordTest.cpp
        LexerTest.cpp
        QStringTest.cpp
        WhitespaceTest.cpp
)

target_link_libraries(
        LexerTest
        gtest_main
        oblcore
        obllexer
)

include(GoogleTest)
gtest_discover_tests(LexerTest)