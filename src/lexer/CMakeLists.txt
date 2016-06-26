add_library(
        obllexer SHARED
        lexer.c
        token.c
        comment.c
        identifier.c
        keywords.c
        number.c
        qstring.c
        whitespace.c
        scannercfg.c
)

target_link_libraries(obllexer PUBLIC oblcore)

add_executable(lexa lexa.c)

target_link_libraries(lexa obllexer oblcore ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} m)

install(TARGETS obllexer DESTINATION lib)
install(TARGETS lexa DESTINATION bin)
