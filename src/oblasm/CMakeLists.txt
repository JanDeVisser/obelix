add_library(
        oblasm
        SHARED
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

install(TARGETS oblasm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
