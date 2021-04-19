add_library(oblast SHARED
        ast.c
        ast_stmt.c
        parse_to_ast.c
        libast.h
        ../../include/ast.h
)

target_link_libraries(oblast PUBLIC oblcore)

install(TARGETS oblast
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

if(CHECK_FOUND)
    add_subdirectory(test)
endif(CHECK_FOUND)
