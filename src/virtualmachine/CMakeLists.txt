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

install(TARGETS oblvm soblvm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
