enable_language(ASM-ATT)
add_library(
        oblrt
        STATIC
        exit.s
        puthex.s
        putint.s
        start.s
        to_string.s
        write.s
)

install(TARGETS oblrt
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
