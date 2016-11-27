set(SOURCES
    token.c
    lexercfg.c
    scannercfg.c
    lexer.c
    scanner.c
    whitespace.c
    identifier.c
    comment.c
    qstring.c
    number.c
    keyword.c
    lexa.c
)
add_library(obllexer SHARED ${SOURCES})
add_library(sobllexer STATIC ${SOURCES})

target_link_libraries(obllexer PUBLIC oblcore)

add_executable(lexa lexamain.c)
target_compile_definitions(lexa PRIVATE OBL_STATIC)
target_link_libraries(lexa sobllexer soblcore ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(obllexer PRIVATE _GNU_SOURCE)
  target_compile_definitions(sobllexer PRIVATE _GNU_SOURCE)
  target_compile_definitions(lexa PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS lexa obllexer sobllexer
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
