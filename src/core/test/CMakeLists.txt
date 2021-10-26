enable_testing()

add_library(
        testlib
        SHARED
        testlib.cpp
)


add_executable(
        CoreTest
        Arguments.cpp
        CEscape.cpp
        Format.cpp
        Join.cpp
        ParsePairs.cpp
        Resolve.cpp
        Split.cpp
        Strip.cpp

        Object.cpp
)
target_link_libraries(
        CoreTest
        gtest_main
        oblcore
        testlib
)

include(GoogleTest)
gtest_discover_tests(CoreTest)