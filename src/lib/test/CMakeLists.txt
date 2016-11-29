find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_library(
  testlib SHARED
  testlib.c
)

add_executable(tcore tcore.c tlist.c tdict.c tstr.c tresolve.c)
target_link_libraries(tcore obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})
