
add_custom_command (
  MAIN_DEPENDENCY ${CMAKE_HOME_DIRECTORY}/share/grammar/obelix.grammar
  OUTPUT oblgrammar.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND panoramix -g ${CMAKE_HOME_DIRECTORY}/share/grammar/obelix.grammar > oblgrammar.c
)

add_library(scriptparse SHARED scriptparse.c)
target_link_libraries(scriptparse oblast oblparser oblgrammar obllexer oblcore ${SYSLIBS})

set(SOURCES oblgrammar.c loader.c obelix.c)
set(LIBS ${SYSLIBS})
set(INCLUDES "")
if(READLINE_FOUND)
  set(LIBS ${READLINE_LIBRARIES} ${LIBS})
  set(INCLUDES ${INCLUDES} ${READLINE_INCLUDE_DIRS})
endif(READLINE_FOUND)

add_executable(obelix ${SOURCES})
target_link_libraries(obelix scriptparse oblipc oblnet oblruntime oblast oblparser oblgrammar obllexer oblcore ${LIBS})
target_include_directories(obelix PRIVATE ${INCLUDES})

install(TARGETS scriptparse obelix
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
