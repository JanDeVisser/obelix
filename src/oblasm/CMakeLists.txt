add_library(
        oblasm
        STATIC
        Assembler.cpp
        AssemblyTypes.cpp
        Buffer.cpp
        Bytes.cpp
        Directive.cpp
        Image.cpp
        Instruction.cpp
        Segment.cpp
        String.cpp
)

target_link_libraries(
        oblasm
        obllexer
        oblcore
        ${LIBS}
)

add_executable(
        jv80asm
        main.cpp
)

target_link_libraries(
        jv80asm
        oblasm
        ${LIBS}
)

install(TARGETS oblasm jv80asm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
