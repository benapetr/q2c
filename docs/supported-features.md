# Supported Feature Matrix

This matrix describes the implemented converter surface. q2c is intentionally a
static converter; it does not execute qmake or CMake scripts.

## qmake Input To CMake Output

| Feature | Status | Notes |
| --- | --- | --- |
| `TARGET`, `TEMPLATE` | Supported | `app`, `lib`, and `subdirs` are mapped. |
| `CONFIG` | Supported | C++ standard, debug/release, plugin/testcase are modeled. |
| `QT` modules | Supported | Qt 4, Qt 5, Qt 6 output styles are available. |
| `SOURCES`, `HEADERS` | Supported | Quoted paths and multiline lists are supported. |
| `FORMS`, `RESOURCES` | Supported | Qt 5/6 output uses automatic Qt handling. Qt 4 uses wrapping commands. |
| `TRANSLATIONS` | Supported | Emitted as CMake translation variables and Qt helper calls for Qt 5/6. |
| `DEFINES`, `INCLUDEPATH`, `DEPENDPATH` | Supported | Generated as target-local CMake properties. |
| `LIBS` | Supported | `-L`, `-l`, plain libraries, and macOS frameworks are handled. |
| `QMAKE_CXXFLAGS`, `QMAKE_LFLAGS` | Supported | Generated as target compile/link options. |
| `INSTALLS` | Partial | Preserved as comments for manual review. |
| `include(...)` | Supported | `.pri` files are loaded relative to the current qmake file. |
| Platform scopes | Supported | `win32`, `unix`, `linux`, `macx`, `msvc`, and `gcc` are mapped. |
| qmake condition functions | Partial | Common functions are preserved as raw conditions with warnings. |
| Arbitrary qmake functions | Unsupported | Reported as warnings when detected. |

## CMake Input To qmake Output

| Feature | Status | Notes |
| --- | --- | --- |
| `cmake_minimum_required`, `project` | Supported | Project name and minimum version are captured. |
| `set(...)` variables | Supported | Simple list variables expand in supported commands. |
| `find_package(Qt4/Qt5/Qt6)` | Supported | Components map to `QT += ...`. |
| `add_executable`, `qt_add_executable` | Supported | Maps to `TEMPLATE = app`. |
| `add_library`, `qt_add_library` | Supported | Maps to `TEMPLATE = lib`; `MODULE` is treated as a plugin target. |
| `add_subdirectory` | Supported | Maps to `SUBDIRS += ...`. |
| `target_sources` | Supported | Sources, headers, forms, resources, and translations are classified. |
| `target_link_libraries` | Supported | Qt imported targets map to `QT`; other libraries map to `LIBS`. |
| `target_link_directories` | Supported | Maps to `LIBS += -L...`. |
| `target_include_directories` | Supported | Maps to `INCLUDEPATH`. |
| `target_compile_definitions` | Supported | Maps to `DEFINES`. |
| `target_compile_options` | Partial | Plain options map to `QMAKE_CXXFLAGS`; generator expressions warn. |
| `target_link_options` | Supported | Maps to `QMAKE_LFLAGS`. |
| `qt_add_translations` | Supported | `TS_FILES` maps to `TRANSLATIONS`. |
| `if`, `else`, `endif` | Partial | Simple platform conditions map to qmake scopes. |
| `set_target_properties` | Partial | Emits a warning because many properties have no qmake equivalent. |
| Generator expressions | Partial | Preserved where parsed, with qmake review warnings. |
| Arbitrary CMake commands | Unsupported | Reported as warnings. |

## Test Coverage

The repository includes fixtures for:

- Basic and complex qmake projects
- Basic and complex CMake projects
- Qt 4, Qt 5, Qt 6 generation
- Console apps, libraries, subdirs, translations, resources, forms, and scopes
- Snapshot tests for generated output
- Round-trip checks for supported features
- Negative and unsupported-input cases
- CLI integration smoke tests

