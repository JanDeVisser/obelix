find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_executable(tgrammar tgrammar.c)

target_link_libraries(tgrammar oblgrammar obllexer obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(tgrammar PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS tgrammar
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
