add_library(
        oblcore
        SHARED
        Arguments.cpp
        Boolean.cpp
        Dictionary.cpp
        Error.cpp
        FileBuffer.cpp
        Format.cpp
        Integer.cpp
        Iterator.cpp
        List.cpp
        Logging.cpp
        NativeFunction.cpp
        Object.cpp
        Range.cpp
        Regex.cpp
        Resolve.cpp
        String.cpp
        StringBuffer.cpp
        StringUtil.cpp
        Type.cpp
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
