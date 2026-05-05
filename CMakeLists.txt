cmake_minimum_required(VERSION 3.21)
project(speakerview CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# TS3 plugin SDK headers: either via TS3_SDK_DIR or dropped into ./sdk/
if(NOT DEFINED TS3_SDK_DIR OR TS3_SDK_DIR STREQUAL "")
    set(TS3_SDK_INCLUDE "${CMAKE_SOURCE_DIR}/sdk")
else()
    set(TS3_SDK_INCLUDE "${TS3_SDK_DIR}/include")
endif()

if(NOT EXISTS "${TS3_SDK_INCLUDE}/plugin_definitions.h")
    message(FATAL_ERROR
        "TS3 Plugin SDK headers not found at ${TS3_SDK_INCLUDE}. "
        "Pass -DTS3_SDK_DIR=<path-to-pluginsdk> or drop the 4 headers into ./sdk/. "
        "See sdk/README.md.")
endif()

find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Widgets)

set(SRC
    src/plugin.cpp
    src/PluginContext.cpp
    src/TS3Bridge.cpp
    src/SpeakerTracker.cpp
    src/ConfigModel.cpp
    src/OverlayWidget.cpp
    src/SpeakerRowWidget.cpp
    src/OverlayController.cpp
    src/ConfigDialog.cpp
)

set(HDR
    src/PluginContext.h
    src/TS3Bridge.h
    src/SpeakerTracker.h
    src/ConfigModel.h
    src/OverlayWidget.h
    src/SpeakerRowWidget.h
    src/OverlayController.h
    src/ConfigDialog.h
)

add_library(speakerview SHARED ${SRC} ${HDR})

target_include_directories(speakerview PRIVATE
    "${TS3_SDK_INCLUDE}"
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(speakerview PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

if(MSVC)
    target_compile_options(speakerview PRIVATE
        /W4 /permissive- /EHsc /wd4100
    )
    target_compile_definitions(speakerview PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

set_target_properties(speakerview PROPERTIES
    OUTPUT_NAME "speakerview"
    PREFIX ""
    SUFFIX ".dll"
)

# Copy plugin .ini alongside the DLL so both can be grabbed from the same
# CI artifact folder.
add_custom_command(TARGET speakerview POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/speakerview.ini"
        "$<TARGET_FILE_DIR:speakerview>/speakerview.ini"
)

# Bundle the MSVC runtime redistributable DLLs next to speakerview.dll so
# the plugin loads even on TS3 installations without the VC++ redist
# installed. TS3 itself ships without these DLLs on some user machines,
# which triggers STATUS_INVALID_IMAGE_FORMAT (0xc0000020) at load time.
# We stick to /MD (dynamic CRT) to stay heap-compatible with Qt.
if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    foreach(dll ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})
        add_custom_command(TARGET speakerview POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${dll}"
                "$<TARGET_FILE_DIR:speakerview>/"
        )
    endforeach()
endif()
