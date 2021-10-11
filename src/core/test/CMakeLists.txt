enable_testing()

add_executable(
        CoreTest
        Arguments.cpp
        CEscape.cpp
        Join.cpp
        ParsePairs.cpp
        Split.cpp
        Strip.cpp

        Object.cpp
)
target_link_libraries(
        CoreTest
        gtest_main
        oblcore
)

include(GoogleTest)
gtest_discover_tests(CoreTest)