find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})

add_executable(tlexer tlexer.c)

target_link_libraries(tlexer obllexer obltest oblcore ${CHECK_LIBRARIES} subunit ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} rt m)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(tlexer PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

