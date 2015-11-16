add_custom_command (
  MAIN_DEPENDENCY grammar.grammar
  OUTPUT grammar.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND panoramix -g ${CMAKE_CURRENT_LIST_DIR}/grammar.grammar > grammar.c
)

add_executable(obelix
	grammar.c
 	loader.c
 	obelix.c
 	scriptparse.c)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(obelix PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

target_link_libraries(obelix oblparser oblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

install(TARGETS obelix DESTINATION bin)
install(FILES grammar.grammar DESTINATION share)
