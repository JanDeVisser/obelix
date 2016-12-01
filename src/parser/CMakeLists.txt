set(SOURCES parser.c parserlib.c)
add_library(oblparser SHARED ${SOURCES})
add_library(soblparser STATIC ${SOURCES})

target_link_libraries(oblparser PUBLIC obllexer oblcore obllexer)

install(TARGETS oblparser
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
