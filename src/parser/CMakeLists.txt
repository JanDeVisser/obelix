add_library(oblparser SHARED
  parser.c
  parserlib.c
)

target_link_libraries(oblparser PUBLIC obllexer oblcore obllexer)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblparser PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS oblparser DESTINATION lib)

add_subdirectory(test)
