add_library(oblparser SHARED
  boundmethod.c
  bytecode.c
  closure.c
  generator.c
  grammar.c
  grammarparser.c
  instruction.c
  namespace.c
  object.c
  parser.c
  script.c
  stacktrace.c
  vm.c
)

target_link_libraries(oblparser PUBLIC oblcore obllexer)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblparser PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

add_executable(panoramix panoramix.c)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(panoramix PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
target_link_libraries(panoramix oblparser obllexer oblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

install(TARGETS oblparser DESTINATION lib)
install(TARGETS panoramix DESTINATION bin)
