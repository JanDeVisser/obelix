add_library(
        oblasm
        SHARED
        Assembler.cpp
        Buffer.cpp
        Bytes.cpp
        Directive.cpp
        Image.cpp
        Instruction.cpp
        Opcode.cpp
        Segment.cpp
        String.cpp
)

target_link_libraries(
        oblasm
        obllexer
        oblcore
        ${LIBS}
)

enable_testing()

add_executable(
        AsmTest
        AsmTest.cpp
)

target_link_libraries(
        AsmTest
        gtest_main
        oblparser
        oblasm
)

include(GoogleTest)
gtest_discover_tests(AsmTest)


install(TARGETS oblasm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
