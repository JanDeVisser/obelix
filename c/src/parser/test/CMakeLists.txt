include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_executable(tparser tparser.c)

target_link_libraries(tparser oblparser oblgrammar obllexer obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(tparser PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
