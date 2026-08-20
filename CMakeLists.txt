cmake_minimum_required(VERSION 3.16)

project(q2c
    VERSION 0.1.0
    DESCRIPTION "Command-line converter between qmake and CMake Qt projects"
    HOMEPAGE_URL "https://github.com/benapetr/q2c"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

include(GNUInstallDirs)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

set(Q2C_CORE_SOURCES
    q2c/buildmodel.cpp
    q2c/cmakegenerator.cpp
    q2c/cmakeparser.cpp
    q2c/configuration.cpp
    q2c/generic.cpp
    q2c/logs.cpp
    q2c/project.cpp
    q2c/qmakegenerator.cpp
    q2c/qmakeparser.cpp
)

set(Q2C_CORE_HEADERS
    q2c/buildmodel.h
    q2c/cmakegenerator.h
    q2c/cmakeparser.h
    q2c/configuration.h
    q2c/generic.h
    q2c/logs.h
    q2c/project.h
    q2c/qmakegenerator.h
    q2c/qmakeparser.h
)

add_library(q2c_core STATIC
    ${Q2C_CORE_SOURCES}
    ${Q2C_CORE_HEADERS}
)
target_include_directories(q2c_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/q2c>
)
target_link_libraries(q2c_core PUBLIC Qt${QT_VERSION_MAJOR}::Core)
target_compile_definitions(q2c_core PUBLIC Q2C_VERSION="${PROJECT_VERSION}")

add_executable(q2c
    q2c/main.cpp
    q2c/terminalparser.cpp
    q2c/terminalparser.h
)
target_link_libraries(q2c PRIVATE q2c_core)

option(Q2C_BUILD_TESTS "Build q2c unit tests" ON)
if(Q2C_BUILD_TESTS)
    enable_testing()
    add_executable(q2c_tests
        tests/main.cpp
    )
    target_link_libraries(q2c_tests PRIVATE q2c_core)
    target_compile_definitions(q2c_tests PRIVATE
        TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
    )
    add_test(NAME q2c_tests COMMAND q2c_tests)
endif()

install(TARGETS q2c
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES README.md ROADMAP.md
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
)
install(DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    FILES_MATCHING PATTERN "*.md"
)

set(CPACK_PACKAGE_NAME "q2c")
set(CPACK_PACKAGE_VENDOR "q2c contributors")
set(CPACK_PACKAGE_CONTACT "https://github.com/benapetr/q2c")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_GENERATOR "TGZ;ZIP")
if(UNIX AND NOT APPLE)
    list(APPEND CPACK_GENERATOR "DEB" "RPM")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "q2c contributors")
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6 | libqt5core5a")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
endif()
set(CPACK_SOURCE_IGNORE_FILES
    "/\\\\.git/"
    "/build/"
    "/release/"
    "/debug/"
    "/tests/build/"
    "\\\\.o$"
    "\\\\.obj$"
    "\\\\.user$"
)
include(CPack)
