include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_executable(tast tast.c)

target_link_libraries(tast oblast obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(tast PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS tast)

