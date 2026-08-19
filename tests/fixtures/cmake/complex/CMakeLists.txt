cmake_minimum_required(VERSION 3.20)
project(complex_cmake)

find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)

set(COMMON_SOURCES
    src/main.cpp
    src/window.cpp
)

set(COMMON_HEADERS
    include/window.h
    include/appconfig.h
)

qt_add_executable(complex_cmake
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
    ui/mainwindow.ui
    resources/app.qrc
)

target_sources(complex_cmake PRIVATE
    src/extra.cpp
    "src/file with space.cpp"
)

target_compile_definitions(complex_cmake PRIVATE
    COMPLEX_CMAKE
    VERSION=\"2.0\"
)

target_include_directories(complex_cmake PRIVATE
    include
    "third party/include"
)

target_link_directories(complex_cmake PRIVATE
    "third party/lib"
)

target_link_libraries(complex_cmake PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ssl
    crypto
)

target_compile_options(complex_cmake PRIVATE
    -Wall
    -Wextra
    "$<$<CONFIG:Debug>:-DDEBUG_ONLY>"
)

target_link_options(complex_cmake PRIVATE
    -pthread
)

qt_add_translations(complex_cmake
    TS_FILES
        i18n/complex_en.ts
        i18n/complex_de.ts
)

if(WIN32)
    target_sources(complex_cmake PRIVATE platform/win.cpp)
    target_compile_definitions(complex_cmake PRIVATE PLATFORM_WINDOWS)
endif()

if(APPLE)
    target_sources(complex_cmake PRIVATE platform/mac.mm)
    target_link_libraries(complex_cmake PRIVATE "-framework Cocoa")
endif()

if(UNIX)
    target_sources(complex_cmake PRIVATE platform/unix.cpp)
    target_compile_definitions(complex_cmake PRIVATE PLATFORM_UNIX)
endif()

add_subdirectory(plugins)
add_library(extra_plugin MODULE plugins/plugin.cpp)
set_target_properties(extra_plugin PROPERTIES AUTOMOC ON)
