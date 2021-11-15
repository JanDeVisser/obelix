add_library(
        oblvm
        SHARED
        VirtualMachine.cpp
)

target_link_libraries(
        oblvm
        oblcore
)

add_executable(
        vm
        vm.cpp
)

target_link_libraries(
        vm
        oblvm
        oblcore
        ${LIBS}
)


#add_executable(
#        oblasm
#        oblasm.cpp
#)
#
#target_link_libraries(
#        oblasm
#        oblvm
#        obllexer
#        oblcore
#        ${LIBS}
#)

install(TARGETS oblvm vm # oblasm
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib)

#add_subdirectory(test)
