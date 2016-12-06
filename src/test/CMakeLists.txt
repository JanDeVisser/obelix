include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

set(SOURCES
    testsuite.c
)
add_library(obltest STATIC ${SOURCES})
target_link_libraries(obltest oblcore ${CHECK_LIBRARIES})

install(TARGETS obltest DESTINATION lib)
