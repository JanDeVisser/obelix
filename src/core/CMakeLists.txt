add_library(
        oblcore
        STATIC
        Error.cpp
        FileBuffer.cpp
        Format.h
        Logging.cpp
        Process.cpp
        Resolve.cpp
        ScopeGuard.h
        StringBuffer.cpp
        StringUtil.cpp
)

target_link_libraries(
        oblcore
        dl
)

install(TARGETS oblcore
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)
