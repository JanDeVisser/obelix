add_library(
        jv80cpu
        SHARED
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
)

target_link_libraries(jv80cpu oblcore)

add_executable(
        emu_cmdline
        emu.cpp
)

target_link_libraries(emu_cmdline jv80cpu oblcore)

install(TARGETS jv80cpu emu_cmdline
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )

add_subdirectory(test)
