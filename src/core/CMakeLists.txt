add_library(
        oblcore
        SHARED
        Arguments.cpp
        Boolean.cpp
        Dictionary.cpp
        Format.cpp
        Integer.cpp
        Iterator.cpp
        List.cpp
        Logging.cpp
        NativeFunction.cpp
        Object.cpp
        Range.cpp
        Regex.cpp
        String.cpp
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

