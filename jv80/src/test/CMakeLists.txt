enable_testing()

add_executable(
        emu_test
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
)

target_link_libraries(emu_test emucomponents ${GTEST_LDFLAGS})
target_compile_options(emu_test PUBLIC ${GTEST_CFLAGS})

include(GoogleTest)
gtest_discover_tests(emu_test)

