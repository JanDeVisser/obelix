enable_testing()

add_executable(
        LexerTest
        CommentTest.cpp
        KeywordTest.cpp
        LexerTest.cpp
        NumberTest.cpp
        QStringTest.cpp
        WhitespaceTest.cpp
)

target_link_libraries(
        LexerTest
        gtest_main
        oblcore
        obllexer
        dl
)

include(GoogleTest)
gtest_discover_tests(LexerTest)
