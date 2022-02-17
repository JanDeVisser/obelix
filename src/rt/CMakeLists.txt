enable_language(ASM-ATT)
add_library(
        oblrt
        STATIC
        close.s
        exit.s
        memcpy.s
        memset.s
        open.s
        puthex.s
        putint.s
        read.s
        start.s
        string_alloc.s
        string_concat.s
        strncpy.s
        to_string.s
        write.s
)

install(TARGETS oblrt
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
