add_executable(
        obelix
        main.cpp
        BoundFunction.cpp
        Functions.cpp
        Parser.cpp
        Runtime.cpp
        Scope.cpp
        Syntax.cpp
)

target_link_libraries(
        obelix
        obllexer
        oblcore
        ${LIBS}
)

install(TARGETS obelix
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)



