add_library(oblparser SHARED parser.c parserlib.c)
target_link_libraries(oblparser PUBLIC obllexer oblcore obllexer)

install(TARGETS oblparser
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

if(CHECK_FOUND)
  add_subdirectory(test)
endif(CHECK_FOUND)
