# Contributing

Contributions are welcome. The project is still evolving, so focused changes
with fixtures are easiest to review.

## Development Setup

Build with CMake:

```sh
cmake -S . -B build -DQ2C_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Or build with qmake:

```sh
mkdir -p release tests/build
cd release
qmake ../q2c/q2c.pro
make
cd ../tests/build
qmake ../tests.pro
make
./q2c_tests
```

## Before Sending A Change

Run:

```sh
ctest --test-dir build --output-on-failure
tests/run_cli_tests.sh build/q2c
```

If you used qmake:

```sh
./tests/build/q2c_tests
tests/run_cli_tests.sh release/q2c
```

## Adding Parser Or Generator Support

When adding support for a new qmake or CMake construct:

- Add or update a fixture under `tests/fixtures`.
- Assert the parsed `BuildProject` contents in `tests/main.cpp`.
- Assert the generated output.
- Add a snapshot if the whole generated file matters.
- Add a warning test when the feature is only partially supported.

Prefer mapping into the neutral build model instead of adding direct
parser-to-generator shortcuts.

## Coding Style

- Keep code compatible with C++17 and Qt 5/6 Core.
- Prefer focused helpers over large string-building blocks.
- Keep warnings actionable and line-aware when the parser has line context.
- Do not silently drop unsupported build-system behavior.

## Release Workflow

Update `CHANGELOG.md`, then build release artifacts:

```sh
scripts/make_release.sh
```

To draft changelog entries from commits:

```sh
scripts/generate_changelog.sh
```

