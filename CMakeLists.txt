cmake_minimum_required(VERSION 3.16)
project(obelix)

cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0115 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall")

set(obelix_NAME Obelix)
set(obelix_VERSION_MAJOR 2)
set(obelix_VERSION_MINOR 0)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})
set(obelix_DIR ${CMAKE_INSTALL_PREFIX})
set(obelix_DATADIR ${CMAKE_INSTALL_PREFIX}/share)

configure_file(
        "${PROJECT_SOURCE_DIR}/config.h.in"
        "${PROJECT_SOURCE_DIR}/config.h"
)

include_directories("${PROJECT_SOURCE_DIR}/src" "${PROJECT_SOURCE_DIR}")
add_subdirectory(src)
add_subdirectory(share)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


message("Arch: ${CMAKE_SYSTEM_PROCESSOR}")

enable_testing()
add_test(NAME CoreTest COMMAND CoreTest)
add_test(NAME LexerTest COMMAND LexerTest)
add_test(NAME ParserTest COMMAND ParserTest)
add_test(NAME AsmTest COMMAND AsmTest)
add_test(NAME JV80CpuTest COMMAND JV80CpuTest)
