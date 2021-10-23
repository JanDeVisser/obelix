add_executable(
        obelix
        main.cpp
        BoundFunction.cpp
        Functions.cpp
        Parser.cpp
        Scope.h
        Syntax.h
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



