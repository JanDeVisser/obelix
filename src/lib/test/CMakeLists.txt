find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_library(
  testlib SHARED
  testlib.c
)

add_executable(tcore tcore.c tlist.c tdict.c tstr.c tresolve.c)
target_link_libraries(tcore obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(testlib PRIVATE _GNU_SOURCE)
  target_compile_definitions(tcore PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
