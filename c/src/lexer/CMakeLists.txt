add_library(obllexer SHARED
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
    position.c
    lexa.c
)
target_link_libraries(obllexer PUBLIC oblcore)

add_executable(lexa lexamain.c)
target_link_libraries(lexa obllexer oblcore ${SYSLIBS})

install(TARGETS lexa obllexer
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
