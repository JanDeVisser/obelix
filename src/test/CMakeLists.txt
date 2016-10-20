include_directories(${CHECK_INCLUDE_DIRS})
link_directories(${CHECK_LIBRARY_DIRS})

set(SOURCES
    testsuite.c
)
add_library(obltest STATIC ${SOURCES})

set(LIBS oblcore)
set(LIBS ${CHECK_LIBRARIES} ${LIBS})
target_link_libraries(obltest ${LIBS})


if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(obltest PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS obltest DESTINATION lib)
