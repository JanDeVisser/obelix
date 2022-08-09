enable_testing()

add_library(
        testlib
        SHARED
        testlib.cpp
	)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-dynamic")
endif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")

add_executable(
        CoreTest
        CEscape.cpp
        Format.cpp
        Join.cpp
        ParsePairs.cpp
        Resolve.cpp
        Split.cpp
        Strip.cpp
)
target_link_libraries(
        CoreTest
        gtest_main
        oblcore
        testlib
        dl
)

include(GoogleTest)
gtest_discover_tests(CoreTest)
