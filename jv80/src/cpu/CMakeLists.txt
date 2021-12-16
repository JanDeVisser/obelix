add_library(
        emucomponents
        STATIC
        addressregister.cpp
        alu.cpp
        backplane.cpp
        clock.cpp
        component.cpp
        controller.cpp
        iochannel.cpp
        memory.cpp
        microcode.h
        register.cpp
        systembus.cpp
)

add_executable(
        emu_cmdline
        emu.cpp
)

target_link_libraries(emu_cmdline emucomponents)

install(TARGETS emucomponents emu_cmdline
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
