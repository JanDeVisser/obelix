enable_language(ASM-ATT)
add_library(
        oblrt
        STATIC
        start.s
)

install(TARGETS oblrt
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
