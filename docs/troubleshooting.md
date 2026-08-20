# Troubleshooting

## CMake Cannot Find Qt

If CMake reports that it cannot find `Qt6Config.cmake` or `Qt5Config.cmake`,
pass your Qt installation prefix:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
```

On macOS with the Qt online installer this may look like:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.2/macos
```

## qmake Cannot Find Qt

Use the qmake that belongs to the Qt version you want:

```sh
/path/to/Qt/6.x/gcc_64/bin/qmake ../q2c/q2c.pro
```

## Output File Already Exists

q2c does not overwrite files by default:

```sh
q2c -i app.pro -o CMakeLists.txt
```

Use `--force` when you intentionally want to replace the output:

```sh
q2c --force -i app.pro -o CMakeLists.txt
```

## Direction Detection Fails

q2c infers direction from the input file name. Use an explicit direction for
unusual file names:

```sh
q2c --qmake-to-cmake -i project.txt -o CMakeLists.txt
q2c --cmake-to-qmake -i build-script.txt -o project.pro
```

## Generated Output Contains Warnings

Warnings are expected for constructs that do not map cleanly between build
systems, such as arbitrary CMake commands, qmake condition functions, generator
expressions, or CMake target properties.

Review warning comments in the generated file:

```text
# q2c warning: set_target_properties is not fully represented at line 83
```

## `--check` Fails On qmake Input

qmake projects currently need enough structure to identify a target, unless the
project is a `TEMPLATE = subdirs` project. A missing `TARGET` in a normal app or
library project is treated as a parse failure.

## Installed Binary Cannot Find Qt

The CMake install path enables RPATH propagation from the linked Qt libraries.
If a binary still cannot find Qt, verify that the Qt runtime libraries are
available at the same prefix used for `CMAKE_PREFIX_PATH`, or run q2c from a
Qt-enabled environment.

## CI Differences

The GitHub Actions workflow installs Qt with `jurplel/install-qt-action` and
then runs both qmake and CMake builds. If local builds behave differently, first
compare the Qt version, compiler, and build generator.

