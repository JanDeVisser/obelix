add_library(
        oblcrt
        STATIC
        main.c
        puts.c
        string.c
        fsize.c)

install(TARGETS oblcrt
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(FILES obelix.h DESTINATION include)

add_subdirectory(arch/${CMAKE_SYSTEM_PROCESSOR})
