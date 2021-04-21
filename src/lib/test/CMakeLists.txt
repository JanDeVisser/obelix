#include_directories(${CHECK_INCLUDE_DIRS})
#link_directories(${CHECK_LIBRARY_DIRS})
#
#add_library(
#  testlib SHARED
#  testlib.c
#)
#
#add_executable(tcore
#  tcore.c
#  tlist.c
#  tdict.c
#  tdatalist.c
#  tstr.c
#  tresolve.c)
#
#target_link_libraries(tcore obltest oblcore ${CHECK_LIBRARIES} ${SYSLIBS})
#
#add_executable(loadlib loadlib.c)
#target_link_libraries(loadlib oblcore ${SYSLIBS})
#
#add_executable(readfile readfile.c)
#target_link_libraries(readfile oblcore ${SYSLIBS})

add_executable(heaptest heaptest.cpp)
target_link_libraries(heaptest ${GTEST_LDFLAGS} oblcore ${SYSLIBS})
target_compile_options(heaptest PUBLIC ${GTEST_CFLAGS})

install(TARGETS heaptest)

include(CTest)
add_test(obelix_tests heaptest)
