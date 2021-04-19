add_library(oblruntime SHARED
        boundmethod.c
        closure.c
        generator.c
        namespace.c
        object.c
        script.c
)

target_link_libraries(oblruntime PUBLIC oblast oblcore)

install(TARGETS oblruntime
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
