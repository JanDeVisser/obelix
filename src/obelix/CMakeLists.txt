add_executable(
        obelix2
        main.cpp
        BoundFunction.cpp
        NativeFunction.cpp
        Scope.h
        Syntax.h
)

target_link_libraries(
        obelix2
        obllexer
        oblcore
        ${LIBS}
)

install(TARGETS obelix2
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)



