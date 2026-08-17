#!/usr/bin/env bash
# tools/build_opfpy.sh
# Builds the opfpy Python extension (C++/pybind11) using Conan and CMake.
#
# IMPORTANT: Must be run from the MSYS2 UCRT64 terminal in VS Code.
# Open the UCRT64 terminal profile (configured in .vscode/settings.json),
# then run from the project root:  bash ./tools/build_opfpy.sh
# or from the tools/ folder:       bash ./build_opfpy.sh
#
# All build tools (CMake, GCC) come from the UCRT64 environment.
# Conan is run automatically by CMake via FindOrInstallConan.cmake,
# using the .venv Python (pythonlib/.venv/Scripts/python.exe).
#
# Do NOT run with Windows-native bash (WSL).
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "$PROJECT_DIR"

CMAKE="/ucrt64/bin/cmake"
BUILD_DIR="${PROJECT_DIR}/build/gcc"
BIN_DIR="${PROJECT_DIR}/pythonlib/bin"
VENV_PYTHON="${PROJECT_DIR}/pythonlib/.venv/Scripts/python.exe"

if [ ! -x "$CMAKE" ]; then
    echo "ERROR: UCRT64 cmake not found at $CMAKE"
    echo "       Ensure you are running this script from the UCRT64 terminal."
    exit 1
fi

if [ ! -f "$VENV_PYTHON" ]; then
    echo "ERROR: .venv Python not found at $VENV_PYTHON"
    echo "       Create it with: cd pythonlib && uv venv .venv && uv pip install conan"
    exit 1
fi

echo "==> Using CMake: $($CMAKE --version | head -1)"
echo "==> Using .venv Python for Conan: $("$VENV_PYTHON" --version)"

echo "==> Configuring CMake (preset: gcc)..."
echo "    (Conan install will run automatically via FindOrInstallConan.cmake if needed)"
"$CMAKE" --preset gcc -S "${PROJECT_DIR}"

echo "==> Building opfpy target..."
"$CMAKE" --build "${BUILD_DIR}" --target opfpy

echo "==> Writing Cython dependency manifest..."
PYD=$(find "${BIN_DIR}" -name "opfpy*.pyd" | head -1)
if [ -n "$PYD" ]; then
    echo "$PYD" > "${BIN_DIR}/opfpy_cython_deps.txt"
    echo "    Manifest: ${BIN_DIR}/opfpy_cython_deps.txt"
fi

echo "==> Build complete. Extension output:"
ls -lh "${BIN_DIR}"/opfpy*.pyd 2>/dev/null || echo "    WARNING: No .pyd found in ${BIN_DIR}"
