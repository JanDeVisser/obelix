add_library(oblvm SHARED
  boundmethod.c
  bytecode.c
  closure.c
  generator.c
  instruction.c
  namespace.c
  object.c
  script.c
  stacktrace.c
  vm.c
)

target_link_libraries(oblvm PUBLIC oblcore)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblvm PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS oblvm DESTINATION lib)
