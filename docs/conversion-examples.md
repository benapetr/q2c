# Conversion Examples

## qmake To CMake

Convert a qmake project and let q2c infer the direction:

```sh
q2c -i app.pro -o CMakeLists.txt
```

Generate Qt 6 style CMake:

```sh
q2c --qmake-to-cmake --qt6 -i app.pro -o CMakeLists.txt
```

Preview without writing a file:

```sh
q2c --dry-run --qt6 -i tests/fixtures/qmake/complex/complex.pro
```

Example qmake input:

```qmake
TARGET = demo
TEMPLATE = app
QT += core widgets
CONFIG += c++17
SOURCES += main.cpp window.cpp
HEADERS += window.h
FORMS += window.ui
RESOURCES += app.qrc
win32: SOURCES += platform/win.cpp
```

Representative CMake output:

```cmake
cmake_minimum_required (VERSION 3.16.0)
project(demo)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
add_executable(demo ${demo_SOURCES} ${demo_HEADERS} ${demo_UI_FILES} ${demo_RESOURCE_FILES})
target_link_libraries(demo PRIVATE Qt6::Core Qt6::Widgets)
```

## CMake To qmake

Convert a CMake project:

```sh
q2c --cmake-to-qmake -i CMakeLists.txt -o app.pro
```

Preview without writing:

```sh
q2c --cmake-to-qmake --dry-run -i tests/fixtures/cmake/complex/CMakeLists.txt
```

Representative qmake output:

```qmake
TARGET = complex_cmake
TEMPLATE = app
QT += \
    core \
    widgets \
    network
SOURCES += \
    src/main.cpp \
    src/window.cpp
win32 {
    SOURCES += platform/win.cpp
}
```

## Validation Workflows

Check an input file without writing output:

```sh
q2c --check -i app.pro
q2c --check --cmake-to-qmake -i CMakeLists.txt
```

Run the full local validation suite:

```sh
cmake -S . -B build -DQ2C_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
tests/run_cli_tests.sh build/q2c
```

