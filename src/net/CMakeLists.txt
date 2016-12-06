if(WIN32)
  set(PATHMOD ${PROJECT_BINARY_DIR}/src/lib;${PROJECT_BINARY_DIR}/src/lexer;${PROJECT_BINARY_DIR}/src/grammar)
  add_custom_command (
    MAIN_DEPENDENCY ${CMAKE_HOME_DIRECTORY}/share/grammar/uri.grammar
    OUTPUT urigrammar.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E env "PATH=$ENV{PATH};${PATHMOD}" ${PROJECT_BINARY_DIR}/src/grammar/panoramix.exe -g ${CMAKE_HOME_DIRECTORY}/share/grammar/uri.grammar > urigrammar.c
  )
else(WIN32)
  add_custom_command (
    MAIN_DEPENDENCY ${CMAKE_HOME_DIRECTORY}/share/grammar/uri.grammar
    OUTPUT urigrammar.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND panoramix -g ${CMAKE_HOME_DIRECTORY}/share/grammar/uri.grammar > urigrammar.c
  )
endif(WIN32)

add_library(oblnet SHARED
  socket.c
  uri.c
  urigrammar.c
)

set(LIBS oblcore obllexer oblgrammar oblparser)
if(HAVE_WINSOCK2_H)
  set(LIBS ${LIBS} ws2_32)
endif(HAVE_WINSOCK2_H)
target_link_libraries(oblnet PUBLIC ${LIBS})

add_executable(uriparser uriparser.c)
target_link_libraries(uriparser
    oblnet
    oblparser
    oblgrammar
    obllexer
    oblcore
    ${SYSLIBS})

install(TARGETS oblnet uriparser
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
