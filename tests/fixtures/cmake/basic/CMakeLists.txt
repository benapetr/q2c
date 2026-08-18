cmake_minimum_required(VERSION 3.16)
project(cmake_fixture)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

set(APP_SOURCES
    main.cpp
    mainwindow.cpp
)

add_executable(cmake_fixture
    ${APP_SOURCES}
    mainwindow.h
    mainwindow.ui
    resources.qrc
)

if(WIN32)
    target_sources(cmake_fixture PRIVATE win.cpp)
    target_compile_definitions(cmake_fixture PRIVATE WIN_ONLY)
endif()

target_sources(cmake_fixture PRIVATE extra.cpp)
target_compile_definitions(cmake_fixture PRIVATE HAS_CMAKE_FIXTURE)
target_include_directories(cmake_fixture PRIVATE include)
target_compile_options(cmake_fixture PRIVATE -Wall)
target_link_options(cmake_fixture PRIVATE -pthread)
target_link_libraries(cmake_fixture PRIVATE Qt6::Core Qt6::Widgets)
qt_add_translations(cmake_fixture TS_FILES i18n/cmake_fixture.ts)
set_target_properties(cmake_fixture PROPERTIES AUTOMOC ON)
add_subdirectory(plugin)
