set(SOURCES
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

add_library(oblvm SHARED ${SOURCES})
add_library(soblvm STATIC ${SOURCES})

target_link_libraries(oblvm PUBLIC oblcore)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblvm PRIVATE _GNU_SOURCE)
  target_compile_definitions(soblvm PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS oblvm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

