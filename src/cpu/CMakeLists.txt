add_library(
        oblcpu
        STATIC
        addressregister.cpp
        alu.cpp
        backplane.cpp
        clock.cpp
        component.cpp
        controller.cpp
        emulator.cpp
        iochannel.cpp
        memory.cpp
        microcode.h
        register.cpp
        systembus.cpp
        ComponentContainer.cpp ComponentContainer.h ConnectedComponent.cpp ConnectedComponent.h)

target_link_libraries(oblcpu oblcore)

add_executable(
        emu_cmdline
        emu.cpp
)

target_link_libraries(emu_cmdline oblcpu oblcore)

install(TARGETS oblcpu emu_cmdline
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )

add_subdirectory(test)
