#!/usr/bin/env bash
# scripts/build_opfpy.sh
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "$PROJECT_DIR"

OS="$(uname -s)"

if [ "$OS" = "Darwin" ]; then
    echo "==> Detected environment: macOS"
    # Adiciona explicitamente o Homebrew no PATH para garantir que o Ninja seja encontrado
    export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
    
    CMAKE="cmake"
    CMAKE_PRESET="macos-gcc"
    BUILD_DIR="${PROJECT_DIR}/build/macos-gcc"
    VENV_PYTHON="${PROJECT_DIR}/pythonlib/.venv/bin/python"
    CONAN="${PROJECT_DIR}/pythonlib/.venv/bin/conan"
    EXT_EXTENSION="so"
else
    echo "==> Detected environment: Windows (UCRT64)"
    CMAKE="/ucrt64/bin/cmake"
    CMAKE_PRESET="gcc"
    BUILD_DIR="${PROJECT_DIR}/build/gcc"
    VENV_PYTHON="${PROJECT_DIR}/pythonlib/.venv/Scripts/python.exe"
    CONAN="conan"
    EXT_EXTENSION="pyd"
fi

BIN_DIR="${PROJECT_DIR}/pythonlib/bin"

# Limpeza profunda de caches corrompidos anteriores
echo "==> Cleaning old broken presets and user overrides..."
rm -f CMakeUserPresets.json
rm -rf "${BUILD_DIR}"

if [ ! -f "$VENV_PYTHON" ]; then
    echo "ERROR: .venv Python not found at $VENV_PYTHON"
    exit 1
fi

# Executa o Conan ANTES do CMake para gerar o arquivo onde a nova versão do CMake espera
# Executa o Conan ANTES do CMake para gerar o arquivo onde a nova versão do CMake espera
echo "==> Forcing Conan dependency installation..."
if [ "$OS" = "Darwin" ]; then
    # Injeta variáveis para evitar que o Conan crie arquivos de presets conflitantes
    export CONAN_CMAKE_GENERATOR="Ninja"
    
    "$CONAN" install "${PROJECT_DIR}" \
        -pr:b=default -pr:h=default -s build_type=Debug \
        --output-folder="${BUILD_DIR}" --build=missing
    
    # Cria o link simbólico que o seu CMakeLists.txt (linha 16) espera fixo
    mkdir -p "${BUILD_DIR}"
    ln -sf "${BUILD_DIR}/build/Debug/generators/conan_toolchain.cmake" "${BUILD_DIR}/conan_toolchain.cmake"
fi

echo "==> Configuring CMake (preset: ${CMAKE_PRESET})..."
"$CMAKE" --preset "${CMAKE_PRESET}" -S "${PROJECT_DIR}"

echo "==> Building opfpy target..."
"$CMAKE" --build "${BUILD_DIR}" --target opfpy

echo "==> Writing Cython dependency manifest..."
EXT_FILE=$(find "${BIN_DIR}" -name "opfpy*.${EXT_EXTENSION}" | head -1)
if [ -n "$EXT_FILE" ]; then
    echo "$EXT_FILE" > "${BIN_DIR}/opfpy_cython_deps.txt"
fi

echo "==> Build complete."
