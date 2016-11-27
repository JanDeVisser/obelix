add_custom_command (
  MAIN_DEPENDENCY ${CMAKE_HOME_DIRECTORY}/share/grammar/obelix.grammar
  OUTPUT oblgrammar.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND panoramix -g ${CMAKE_HOME_DIRECTORY}/share/grammar/obelix.grammar > oblgrammar.c
)

add_library(scriptparse SHARED scriptparse.c)
add_executable(obelix
  oblgrammar.c
  loader.c
  obelix.c
  server.c
)

target_link_libraries(scriptparse oblvm oblparser oblgrammar obllexer oblcore ${SYSLIBS})
target_link_libraries(obelix scriptparse oblnet oblvm oblparser oblgrammar obllexer oblcore ${SYSLIBS})

install(TARGETS scriptparse obelix
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
