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

install(TARGETS oblvm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
