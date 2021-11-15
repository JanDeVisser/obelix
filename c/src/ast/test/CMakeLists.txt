add_executable(tast tast.cpp)
target_link_libraries(tast oblast oblparser oblgrammar obllexer ${GTEST_LDFLAGS} oblcore ${SYSLIBS})
target_compile_options(tast PUBLIC ${GTEST_CFLAGS})

add_executable(ast_basicflow ast_basicflow.cpp fixture.h)
target_link_libraries(ast_basicflow oblast oblparser oblgrammar obllexer ${GTEST_LDFLAGS} oblcore ${SYSLIBS})
target_compile_options(ast_basicflow PUBLIC ${GTEST_CFLAGS})

add_executable(ast_forloop ast_forloop.cpp fixture.h)
target_link_libraries(ast_forloop oblast oblparser oblgrammar obllexer ${GTEST_LDFLAGS} oblcore ${SYSLIBS})
target_compile_options(ast_forloop PUBLIC ${GTEST_CFLAGS})

if(CMAKE_COMPILER_IS_GNUCC)
  target_compile_definitions(tast PRIVATE _GNU_SOURCE)
  target_compile_definitions(ast_basicflow PRIVATE _GNU_SOURCE)
endif(CMAKE_COMPILER_IS_GNUCC)

install(TARGETS tast ast_basicflow ast_forloop)

include(CTest)
add_test(obelix_tests tast ast_basicflow ast_forloop)


