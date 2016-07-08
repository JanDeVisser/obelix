add_library(
        obllexer SHARED
        token.c
        lexercfg.c
        scannercfg.c
        lexer.c
        scanner.c
        whitespace.c
        identifier.c
        lexa.c
#        comment.c
#        keywords.c
#        number.c
#        qstring.c
)

target_link_libraries(obllexer PUBLIC oblcore)

add_executable(lexa lexamain.c)

target_link_libraries(lexa obllexer oblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(obllexer PRIVATE _GNU_SOURCE)
  target_compile_definitions(lexa PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS obllexer DESTINATION lib)
install(TARGETS lexa DESTINATION bin)

add_subdirectory(test)