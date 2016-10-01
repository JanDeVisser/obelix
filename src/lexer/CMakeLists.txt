add_library(
        obllexer SHARED
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
        lexa.c
#        keywords.c
)

target_link_libraries(obllexer PUBLIC oblcore)

add_executable(lexa lexamain.c)

target_link_libraries(lexa obllexer oblcore ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(obllexer PRIVATE _GNU_SOURCE)
  target_compile_definitions(lexa PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS obllexer DESTINATION lib)
install(TARGETS lexa DESTINATION bin)

add_subdirectory(test)
