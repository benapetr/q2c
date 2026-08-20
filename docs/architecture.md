# Architecture

q2c has three layers:

1. Input parsers read qmake or CMake text.
2. A neutral build model stores project intent.
3. Output generators write CMake or qmake text.

This keeps conversion logic from depending directly on either build-system
syntax.

## Core Model

The neutral model lives in:

- `q2c/buildmodel.h`
- `q2c/buildmodel.cpp`

Important types:

- `BuildProject`: project name, global settings, warnings, and targets.
- `BuildTarget`: target name, type, files, Qt modules, defines, include paths,
  libraries, options, install rules, subdirectories, and conditional scopes.
- `BuildConditionalScope`: platform or condition-specific additions.

Warnings are part of the model so generators can include them in output files.

## qmake Parser

Files:

- `q2c/qmakeparser.h`
- `q2c/qmakeparser.cpp`

The qmake parser handles assignments, list operators, variable expansion,
includes, common qmake variables, and simple scopes. It does not evaluate qmake
semantics deeply; condition functions are preserved and warning comments are
emitted.

## CMake Parser

Files:

- `q2c/cmakeparser.h`
- `q2c/cmakeparser.cpp`

The CMake parser reads a practical static subset of CMake command calls. It
tracks simple variables, target commands, Qt package discovery, simple
conditionals, and unsupported-command warnings. It does not execute CMake.

## CMake Generator

Files:

- `q2c/cmakegenerator.h`
- `q2c/cmakegenerator.cpp`

The CMake generator produces target-based CMake. Qt 5/6 output uses `AUTOMOC`,
`AUTOUIC`, and `AUTORCC`; Qt 4 output uses explicit wrapping commands. It maps
qmake libraries, include paths, defines, compiler flags, linker flags,
resources, forms, translations, subdirs, and platform scopes.

## qmake Generator

Files:

- `q2c/qmakegenerator.h`
- `q2c/qmakegenerator.cpp`

The qmake generator emits readable `.pro` files with multiline assignments. It
maps CMake target data back to qmake variables and translates simple CMake
platform conditions to qmake scopes. Additional CMake targets and generator
expressions are called out with warnings.

## CLI Flow

Files:

- `q2c/main.cpp`
- `q2c/terminalparser.cpp`
- `q2c/project.cpp`

`main.cpp` parses CLI options, detects direction, reads input, loads the project
model through `Project`, and writes or prints generated output. `Project` is a
small facade that delegates parsing and generation.

## Tests

Tests live under:

- `tests/main.cpp`
- `tests/fixtures`
- `tests/run_cli_tests.sh`

The test runner is intentionally a lightweight Qt console program. It exercises
parsers, generators, snapshots, round trips, unsupported inputs, and fixture
coverage without introducing a separate test framework dependency.

