add_library(
        obllexer
        SHARED
        IdentifierScanner.cpp
        KeywordScanner.cpp
        Lexer.h
        NumberScanner.cpp
        QStringScanner.cpp
        StringBuffer.cpp
        Token.cpp
        Tokenizer.cpp
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
