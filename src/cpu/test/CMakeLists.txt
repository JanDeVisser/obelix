enable_testing()

add_executable(
        CpuTest
        addressregister.cpp
        alu.cpp
        arithmetic.cpp
        clock.cpp
        controller.cpp
        inout.cpp
        io.cpp
        jump.cpp
        memory.cpp
        pushfl.cpp
        register.cpp
        stack.cpp
        swap.cpp
        Harness.cpp
        Indexed.cpp
)

target_link_libraries(
        CpuTest
        gtest_main
        oblcpu)

include(GoogleTest)
gtest_discover_tests(CpuTest)
