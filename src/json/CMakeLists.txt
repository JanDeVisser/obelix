if(WIN32)
  set(PATHMOD ${PROJECT_BINARY_DIR}/src/lib;${PROJECT_BINARY_DIR}/src/lexer;${PROJECT_BINARY_DIR}/src/grammar)
  add_custom_command (
    MAIN_DEPENDENCY ${CMAKE_HOME_DIRECTORY}/share/grammar/json.grammar
    OUTPUT jsongrammar.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E env "PATH=$ENV{PATH};${PATHMOD}" ${PROJECT_BINARY_DIR}/src/grammar/panoramix.exe -g ${CMAKE_HOME_DIRECTORY}/share/grammar/json.grammar > jsongrammar.c
  )
else(WIN32)
  add_custom_command (
    MAIN_DEPENDENCY ${CMAKE_HOME_DIRECTORY}/share/grammar/json.grammar
    OUTPUT jsongrammar.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND panoramix -g ${CMAKE_HOME_DIRECTORY}/share/grammar/json.grammar > jsongrammar.c
  )
endif(WIN32)

add_library(obljson SHARED
  json.c
  jsongrammar.c
)

target_link_libraries(obljson PUBLIC oblparser oblvm oblnet)

install(TARGETS obljson
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
