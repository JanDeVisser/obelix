add_executable(tast tast.cpp)

target_link_libraries(tast oblast oblparser oblgrammar obllexer ${GTEST_LDFLAGS} oblcore ${SYSLIBS})
target_compile_options(tast PUBLIC ${GTEST_CFLAGS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(tast PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS tast)

include(CTest)
add_test(obelix_tests tast)


