find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_executable(tnet tnet.c)

target_link_libraries(tnet oblnet oblparser oblgrammar obllexer obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})
