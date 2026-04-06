cmake_minimum_required(VERSION 3.16)
project(PsPixel VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

qt6_standard_project_setup()

# Generate version header from CMake project version
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h"
)

# Include directories
include_directories(include ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/pixelcanvas.cpp
    src/ui/bottom/zoombar.cpp
    src/ui/top/saveexportdialog.cpp
    src/ui/top/newprojectdialog.cpp
    src/ui/top/keybindsdialog.cpp
    src/ui/top/autoupdater.cpp
    src/ui/start/startscreen.cpp
    src/fileformat/pspxformat.cpp
    src/ui/tools/basetool.cpp
    src/ui/tools/brushtool.cpp
    src/ui/tools/erasertool.cpp
    src/ui/tools/eyedroppertool.cpp
    src/ui/tools/paintbuckettool.cpp
    src/ui/tools/linetool.cpp
    src/ui/tools/rectangletool.cpp
    src/ui/tools/circletool.cpp
    src/ui/tools/selecttool.cpp
    src/ui/tools/toolmanager.cpp
    src/ui/tools/toolpalette.cpp
    src/ui/history/historymanager.cpp
    src/ui/history/historypalette.cpp
    src/ui/layers/layermanager.cpp
    src/ui/layers/layerpalette.cpp
)

# Header files
set(HEADERS
    include/mainwindow.h
    include/pixelcanvas.h
    include/constants.h
    src/ui/bottom/zoombar.h
    src/ui/top/saveexportdialog.h
    src/ui/top/newprojectdialog.h
    src/ui/top/keybindsdialog.h
    src/ui/top/autoupdater.h
    src/ui/start/startscreen.h
    src/fileformat/pspxformat.h
    src/ui/tools/basetool.h
    src/ui/tools/brushtool.h
    src/ui/tools/erasertool.h
    src/ui/tools/eyedroppertool.h
    src/ui/tools/paintbuckettool.h
    src/ui/tools/linetool.h
    src/ui/tools/rectangletool.h
    src/ui/tools/circletool.h
    src/ui/tools/selecttool.h
    src/ui/tools/toolmanager.h
    src/ui/tools/toolpalette.h
    src/ui/history/historymanager.h
    src/ui/history/historypalette.h
    src/ui/layers/layer.h
    src/ui/layers/layermanager.h
    src/ui/layers/layerpalette.h
)

# Resource files
set(RESOURCES
    resources.qrc
)

# Windows resource file for icon (version injected from CMake)
if(WIN32)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/app.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/app.rc"
    )
    set(RESOURCES ${RESOURCES} "${CMAKE_CURRENT_BINARY_DIR}/app.rc")
endif()

# Create executable
qt6_add_executable(PsPixel ${SOURCES} ${HEADERS} ${RESOURCES})

# Add Qt resources
qt6_add_resources(PsPixel "resources"
    PREFIX "/"
    FILES
        assets/icon.png
        assets/icons/save.png
        assets/icons/trash.png
        assets/icons/home.png
        assets/icons/bolt.png
        assets/icons/brush.png
        assets/icons/eraser.png
        assets/icons/line.png
        assets/icons/paintbucket.png
        assets/icons/eyedropper.png
        assets/icons/clear.png
        assets/icons/new.png
        assets/icons/history.png
        assets/icons/rectangle.png
        assets/icons/circle.png
        assets/icons/select.png
)

# Workaround Qt 6.7 + GCC 15 _Float16 multiple definition bug
if(MINGW)
    target_link_options(PsPixel PRIVATE -Wl,--allow-multiple-definition)
endif()

# Link libraries
target_link_libraries(PsPixel PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network)

# Set output directory
set_target_properties(PsPixel PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
) 