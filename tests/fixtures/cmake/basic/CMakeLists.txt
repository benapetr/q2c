cmake_minimum_required(VERSION 3.16)
project(cmake_fixture)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

add_executable(cmake_fixture
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    resources.qrc
)

target_compile_definitions(cmake_fixture PRIVATE HAS_CMAKE_FIXTURE)
target_include_directories(cmake_fixture PRIVATE include)
target_link_libraries(cmake_fixture PRIVATE Qt6::Core Qt6::Widgets)
