add_library(
        oblcrt
        STATIC
        puts.c
        string.c)

install(TARGETS oblcrt
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
install(FILES crt.h DESTINATION include)

add_subdirectory(arch/${CMAKE_SYSTEM_PROCESSOR})
