find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})

add_library(
  oblttestlib SHARED
  testlib.c
)

add_executable(tlist tlist.c)
target_link_libraries(tlist obltest oblcore ${CHECK_LIBRARIES} subunit ${SYSLIBS})

add_executable(tstr tstr.c)
target_link_libraries(tstr obltest oblcore ${CHECK_LIBRARIES} subunit ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(oblttestlib PRIVATE _GNU_SOURCE)
  target_compile_definitions(tlist PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)
