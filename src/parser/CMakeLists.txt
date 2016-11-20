set(SOURCES parser.c parserlib.c)

add_library(oblparser SHARED ${SOURCES})
add_library(soblparser STATIC ${SOURCES})

target_link_libraries(oblparser PUBLIC obllexer oblcore obllexer)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblparser PRIVATE _GNU_SOURCE)
  target_compile_definitions(soblparser PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS oblparser
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
