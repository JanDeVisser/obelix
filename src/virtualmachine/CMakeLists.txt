add_library(oblvm SHARED
  bytecode.c
  debug.c
  instruction.c
  stacktrace.c
  vm.c
)

target_link_libraries(oblvm PUBLIC oblcore)

install(TARGETS oblvm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
