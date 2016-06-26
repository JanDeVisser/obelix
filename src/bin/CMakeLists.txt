add_custom_command (
  MAIN_DEPENDENCY oblgrammar.grammar
  OUTPUT oblgrammar.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND panoramix -g ${CMAKE_CURRENT_LIST_DIR}/oblgrammar.grammar > oblgrammar.c
)

add_executable(obelix
    oblgrammar.c
    loader.c
    obelix.c
    scriptparse.c
    server.c
)

target_link_libraries(obelix obllexer oblparser oblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

install(TARGETS obelix DESTINATION bin)
install(FILES oblgrammar.grammar DESTINATION share)
