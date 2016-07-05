set(SOURCES 
    testsuite.c
)
add_library(obltest SHARED ${SOURCES})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(obltest PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS obltest DESTINATION lib)
