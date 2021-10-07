add_library(
        oblcore
        SHARED
        Arguments.cpp
        Dictionary.cpp
        Format.cpp
        List.cpp
        Logging.cpp
        Object.cpp
        Regex.cpp
        StringUtil.cpp
)

install(TARGETS oblcore
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)

