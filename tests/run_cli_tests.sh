#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
Q2C_BINARY="${Q2C_BINARY:-${1:-$ROOT_DIR/release/q2c}}"

if [[ ! -x "$Q2C_BINARY" ]]; then
    echo "q2c binary is not executable: $Q2C_BINARY" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

"$Q2C_BINARY" --version | grep -q "q2c version"

"$Q2C_BINARY" --check -i "$ROOT_DIR/tests/fixtures/qmake/complex/complex.pro"
"$Q2C_BINARY" --check --cmake-to-qmake -i "$ROOT_DIR/tests/fixtures/cmake/complex/CMakeLists.txt"

"$Q2C_BINARY" --qmake-to-cmake --dry-run --qt6 \
    -i "$ROOT_DIR/tests/fixtures/qmake/complex/complex.pro" > "$TMP_DIR/complex.cmake"
grep -q "find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)" "$TMP_DIR/complex.cmake"
grep -q "target_link_libraries(complex_app PRIVATE \"-framework Security\")" "$TMP_DIR/complex.cmake"
grep -q "qt_add_translations(complex_app TS_FILES" "$TMP_DIR/complex.cmake"

"$Q2C_BINARY" --cmake-to-qmake --dry-run \
    -i "$ROOT_DIR/tests/fixtures/cmake/complex/CMakeLists.txt" > "$TMP_DIR/complex.pro"
grep -q "TARGET = complex_cmake" "$TMP_DIR/complex.pro"
grep -q "QT += \\\\" "$TMP_DIR/complex.pro"
grep -q "CMake generator expressions require manual qmake review" "$TMP_DIR/complex.pro"

if "$Q2C_BINARY" --strict --cmake-to-qmake --check \
    -i "$ROOT_DIR/tests/fixtures/cmake/complex/CMakeLists.txt" >/dev/null 2>"$TMP_DIR/strict.err"; then
    echo "strict mode unexpectedly accepted a warning-producing input" >&2
    exit 1
fi
grep -q "Strict mode failed" "$TMP_DIR/strict.err"

"$Q2C_BINARY" --warnings json --cmake-to-qmake --check \
    -i "$ROOT_DIR/tests/fixtures/cmake/complex/CMakeLists.txt" >/dev/null 2>"$TMP_DIR/warnings.jsonl"
grep -q '"type":"warning"' "$TMP_DIR/warnings.jsonl"
grep -q '"line":83' "$TMP_DIR/warnings.jsonl"

mkdir -p "$TMP_DIR/out"
"$Q2C_BINARY" --qmake-to-cmake --qt6 --output-dir "$TMP_DIR/out" \
    -i "$ROOT_DIR/tests/fixtures/qmake/library/library.pro"
test -f "$TMP_DIR/out/CMakeLists.txt"

printf 'old contents\n' > "$TMP_DIR/out/CMakeLists.txt"
"$Q2C_BINARY" --qmake-to-cmake --qt6 --backup --output-dir "$TMP_DIR/out" \
    -i "$ROOT_DIR/tests/fixtures/qmake/library/library.pro"
test -f "$TMP_DIR/out/CMakeLists.txt.bak"
grep -q "old contents" "$TMP_DIR/out/CMakeLists.txt.bak"

if "$Q2C_BINARY" --qmake-to-cmake --check -i "$ROOT_DIR/tests/fixtures/qmake/negative/missing_target.pro"; then
    echo "missing-target qmake fixture unexpectedly passed --check" >&2
    exit 1
fi

if "$Q2C_BINARY" --definitely-not-an-option >/dev/null 2>&1; then
    echo "unknown CLI option unexpectedly succeeded" >&2
    exit 1
fi

echo "CLI integration tests passed"
