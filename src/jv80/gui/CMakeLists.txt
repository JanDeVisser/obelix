find_package(Qt6 COMPONENTS Test REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS Svg REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
#include_directories("${Qt6Widgets_INCLUDE_DIRS}" "${Qt6Svg_INCLUDE_DIRS}")

add_definitions("-DNDEBUG")
add_executable(
        emu
        blockcursorstyle.cpp
        commands.cpp
        componentview.cpp
        cputhread.cpp
        main.cpp
        mainwindow.cpp
        memdump.cpp
        qled.cpp
        systembusview.cpp
        terminal.cpp
)

target_link_libraries(
        emu
        PRIVATE
        jv80cpu
        Qt${QT_VERSION_MAJOR}::Svg
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Core
)

install(TARGETS emu
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
