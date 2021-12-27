enable_testing()

add_executable(
        JV80CpuTest
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
        JV80CpuTest
        gtest_main
        jv80cpu)

include(GoogleTest)
gtest_discover_tests(JV80CpuTest)
