add_custom_command (
  MAIN_DEPENDENCY ${CMAKE_HOME_DIRECTORY}/share/grammar/uri.grammar
  OUTPUT urigrammar.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND panoramix -g ${CMAKE_HOME_DIRECTORY}/share/grammar/uri.grammar > urigrammar.c
)

set(SOURCES
  uri.c
  urigrammar.c
)

add_library(oblnet SHARED ${SOURCES})
add_library(soblnet STATIC ${SOURCES})
target_link_libraries(oblnet PUBLIC oblcore obllexer oblgrammar oblparser)

add_executable(uriparser uriparser.c)
target_link_libraries(uriparser
    oblnet
    oblparser
    oblgrammar
    obllexer
    oblcore
    ${SYSLIBS})

install(TARGETS oblnet soblnet
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
