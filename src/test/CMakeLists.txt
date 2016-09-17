set(SOURCES
    testsuite.c
)
add_library(obltest SHARED ${SOURCES})
set(LIBS oblcore)
if(APPLE)
  set(LIBS ${CHECK_LIBRARIES} ${LIBS})
endif(APPLE)
target_link_libraries(obltest ${LIBS})


if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(obltest PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS obltest DESTINATION lib)
