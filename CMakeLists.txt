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
    src/NativeStyle.cpp
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
    src/NativeStyle.h
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
    # /utf-8 treats source files as UTF-8 and emits UTF-8 string literals.
    # Without it, MSVC reads source as the active code page (cp1252 on most
    # Windows installs), mangles em-dashes / umlauts / any non-ASCII, and
    # the plugin shows mojibake in its UI.
    target_compile_options(speakerview PRIVATE
        /W4 /permissive- /EHsc /wd4100 /utf-8
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
# NOTE: We do NOT bundle the MSVC runtime DLLs here. TS3 scans every .dll
# in its plugins/ directory for the ts3plugin_* exports, so shipping the
# VC runtime alongside would cause 8 bogus "plugin function not found"
# errors on every TS3 launch. Users must install the official
# VC++ 2015–2022 x64 redistributable (aka.ms/vs/17/release/vc_redist.x64.exe)
# — see README.
add_custom_command(TARGET speakerview POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/speakerview.ini"
        "$<TARGET_FILE_DIR:speakerview>/speakerview.ini"
)
