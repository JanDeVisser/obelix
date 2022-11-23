add_library(
        oblcrt
        STATIC
        io.c
        main.c
        puts.c
        string.c
        fsize.c
        enum.c)

install(TARGETS oblcrt
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(FILES obelix.h DESTINATION include)

add_subdirectory(arch/${CMAKE_SYSTEM_PROCESSOR})
