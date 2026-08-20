# q2c

q2c is a command-line converter between Qt qmake projects and CMake projects.
It can convert common `.pro` / `.pri` projects to `CMakeLists.txt`, and it can
convert common Qt CMake projects back to qmake `.pro` files.

The project is still under active development. q2c prefers readable generated
build files and explicit warnings over pretending every qmake or CMake scripting
feature can be converted perfectly.

## Requirements

- Qt 5 or Qt 6 development tools
- A C++17 compiler
- Either CMake 3.16+ or qmake

## Build

Using CMake:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

If Qt is not in CMake's default search path, pass its prefix:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

Using qmake:

```sh
mkdir -p release
cd release
qmake ../q2c/q2c.pro
make
```

## Test

Using CMake:

```sh
cmake -S . -B build -DQ2C_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Using qmake:

```sh
mkdir -p tests/build
cd tests/build
qmake ../tests.pro
make
./q2c_tests
```

CLI smoke tests:

```sh
tests/run_cli_tests.sh build/q2c
```

## Install And Package

Install from a CMake build:

```sh
cmake --install build
```

Create release archives:

```sh
scripts/make_release.sh
```

The CMake build uses CPack to create `.tar.gz` and `.zip` archives on every
platform, and DEB/RPM packages on Linux where the tools are available.

## Usage

q2c detects the conversion direction from the input file name:

- `.pro` and `.pri` files are treated as qmake input and converted to CMake.
- `CMakeLists.txt` and `.cmake` files are treated as CMake input and converted
  to qmake.

Examples:

```sh
q2c -i app.pro -o CMakeLists.txt
q2c --qmake-to-cmake --qt6 -i app.pro -o CMakeLists.txt
q2c --cmake-to-qmake -i CMakeLists.txt -o app.pro
q2c --check -i app.pro
q2c --dry-run -i app.pro
```

Existing output files are not overwritten unless `-f` or `--force` is used.

Useful options:

```text
--qmake-to-cmake     Force qmake input to CMake output
--cmake-to-qmake     Force CMake input to qmake output
--qt4, --qt5, --qt6  Select Qt generation style for qmake-to-CMake
--check              Parse and validate only
--dry-run            Print generated output to stdout
--force              Overwrite an existing output file
--version            Print the q2c version
```

## Documentation

- [Conversion Examples](docs/conversion-examples.md)
- [Supported Feature Matrix](docs/supported-features.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Architecture](docs/architecture.md)
- [Contributing](docs/contributing.md)

## Current Scope

q2c supports a practical static subset of qmake and CMake. It handles typical
Qt applications, libraries, resources, forms, translations, include paths,
defines, libraries, compiler/linker flags, subdirs, and simple platform scopes.

It does not execute arbitrary CMake or qmake scripts. Unsupported constructs are
reported as warnings when possible so the generated file can be reviewed.

