set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0048 NEW)


add_executable(dump_instructions
        src/dump_instructions.cpp src/cpu/alu.h src/cpu/opcodes.h
        )
target_link_libraries(dump_instructions emucomponents)

add_subdirectory("src/cpu")
add_subdirectory("src/gui")
add_subdirectory("src/test")
