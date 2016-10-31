find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

add_executable(tlexer tlexer.c comment.c qstring.c keyword.c number.c)

target_link_libraries(tlexer obllexer obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(tlexer PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS tlexer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)
