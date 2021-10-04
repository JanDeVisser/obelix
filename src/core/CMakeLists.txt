add_library(
        oblcore
        SHARED
        Arguments.cpp
        Dictionary.cpp
        Format.cpp
        List.cpp
        Object.cpp
        StringUtil.cpp
)

install(TARGETS oblcore
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

add_subdirectory(test)

