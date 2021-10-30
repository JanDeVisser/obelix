add_library(
        oblcore
        SHARED
        Arguments.cpp
        Dictionary.cpp
        Format.cpp
        List.cpp
        Logging.cpp
        NativeFunction.cpp
        Object.cpp
        Regex.cpp
        StringBuffer.cpp
        StringUtil.cpp
        Resolve.cpp
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

