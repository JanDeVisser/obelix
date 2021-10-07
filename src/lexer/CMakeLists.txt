add_library(
        obllexer
        SHARED
        IdentifierScanner.cpp
        KeywordScanner.cpp
        Lexer.cpp
        NumberScanner.cpp
        QStringScanner.cpp
        StringBuffer.cpp
        Token.cpp
        WhitespaceScanner.cpp
)

target_link_libraries(
        obllexer
        oblcore
)

install(TARGETS obllexer
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
