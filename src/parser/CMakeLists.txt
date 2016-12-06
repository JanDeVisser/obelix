add_library(oblparser SHARED parser.c parserlib.c)
target_link_libraries(oblparser PUBLIC obllexer oblcore obllexer)

install(TARGETS oblparser
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
