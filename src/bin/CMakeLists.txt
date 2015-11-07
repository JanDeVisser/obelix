add_custom_command (
  OUTPUT grammar.c
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/panoramix -g grammar.grammar
  DEPENDS grammar.grammar
)

add_executable(obelix
	grammar.c
 	loader.c
 	obelix.c
 	scriptparse.c)

target_link_libraries(obelix oblcore oblparser)

install(TARGETS obelix DESTINATION bin)