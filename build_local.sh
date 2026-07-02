#!/bin/bash
set -e

# =====================================================================
# PASSO 1: DETECTAR O SISTEMA OPERACIONAL
# =====================================================================
echo "=== 1. Detectando Sistema Operacional ==="
OS_TYPE="linux"
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OS" == "Windows_NT" ]]; then
    OS_TYPE="windows"
    echo "Ambiente detectado: Windows"
else
    echo "Ambiente detectado: Linux Nativo"
fi

# =====================================================================
# PASSO 2: GARANTIR INFRAESTRUTURA (MSYS2 + COMPILADORES)
# =====================================================================
echo -e "\n=== 2. Verificando Infraestrutura de Compilação ==="
if [ "$OS_TYPE" == "windows" ]; then
    MSYS_PATH="/c/msys64"
    UCRT_BIN="$MSYS_PATH/ucrt64/bin"
    
    if [ ! -d "$MSYS_PATH" ]; then
        echo "MSYS2 não encontrado em C:\msys64!"
        echo "Baixando e instalando o MSYS2 automaticamente..."
        
        powershell.exe -Command "Invoke-WebRequest -Uri 'https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-x86_64-latest.exe' -OutFile 'msys2_installer.exe'"
        
        echo "Executando instalação silenciosa do MSYS2..."
        ./msys2_installer.exe --in --msys64-dir C:\\msys64 --confirm-command --accept-messages --root C:\\msys64
        rm msys2_installer.exe
        echo "MSYS2 instalado com sucesso em C:\msys64!"
    fi

    echo "Garantindo dependências do UCRT64 (GCC, CMake, Ninja, Python, UV)..."
    if [ -f "setup_ucrt64.sh" ]; then
        $MSYS_PATH/usr/bin/bash.exe -l -c "$(pwd)/setup_ucrt64.sh"
    else
        $MSYS_PATH/usr/bin/bash.exe -l -c "pacman -Sy --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-python mingw-w64-ucrt-x86_64-uv"
    fi

    # Injeta temporariamente o ambiente UCRT64 no PATH para os próximos passos usarem o Python correto
    export PATH="$UCRT_BIN:$MSYS_PATH/usr/bin:$PATH"
fi

# =====================================================================
# PASSO 3: GERENCIAR AMBIENTE VIRTUAL (VENV)
# =====================================================================
echo -e "\n=== 3. Gerenciando Ambiente Virtual Python (venv) ==="
if [ -n "$CONDA_DEFAULT_ENV" ] || [ -n "$VIRTUAL_ENV" ]; then
    echo "Identificado ambiente virtual já ativo no terminal."
    PYTHON_EXE=$(which python3 || which python)
else
    echo "Nenhum ambiente virtual ativo detectado. Criando isolamento..."
    if [ ! -d ".venv" ]; then
        echo "Criando um novo ambiente virtual em './.venv'..."
        BASE_PYTHON=$(which python3 || which python)
        $BASE_PYTHON -m venv .venv
    fi
    
    echo "Ativando o ambiente virtual './.venv'..."
    if [ "$OS_TYPE" == "windows" ]; then
        source .venv/Scripts/activate
    else
        source .venv/bin/activate
    fi
    PYTHON_EXE=$(which python3 || which python)
fi

PYTHON_VERSION=$($PYTHON_EXE -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
echo "Executável Python ativo: $PYTHON_EXE (v$PYTHON_VERSION)"

# =====================================================================
# PASSO 4: INSTALAR DEPENDÊNCIAS, CONAN E COMPILAR (O RESTANTE DO BUILD)
# =====================================================================
echo -e "\n=== 4. Garantindo Dependências do Python (Conan e PyBind11) ==="
if ! $PYTHON_EXE -m conan --version &> /dev/null; then
    echo "Conan não encontrado no venv. Instalando via pip..."
    $PYTHON_EXE -m pip install --upgrade pip setuptools wheel
    $PYTHON_EXE -m pip install conan pybind11
else
    echo "Conan e PyBind11 já estão prontos no ambiente."
    $PYTHON_EXE -m pip install pybind11 --upgrade --quiet
fi

echo -e "\n=== 5. Gerando Perfil e Instalando Pacotes C++ via Conan ==="
mkdir -p build_conan
cd build_conan
$PYTHON_EXE -m conan profile detect --force > /dev/null 2>&1

if [ "$OS_TYPE" == "windows" ]; then
    $PYTHON_EXE -m conan install .. --output-folder=. --build=missing \
        -s build_type=Release \
        -s compiler=gcc \
        -s compiler.version=$(gcc -dumpversion | cut -d. -f1) \
        -s compiler.libcxx=libstdc++11
else
    $PYTHON_EXE -m conan install .. --output-folder=. --build=missing -s build_type=Release
fi
cd ..

echo -e "\n=== 6. Configurando CMake ==="
mkdir -p build_local
cd build_local

CMAKE_FLAGS=(
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=../build_conan/conan_provider.cmake"
    "-DPYTHON_EXECUTABLE=$PYTHON_EXE"
    "-DPYTHON_VERSION=$PYTHON_VERSION"
)

if [ "$OS_TYPE" == "windows" ]; then
    CMAKE_FLAGS+=("-G" "Ninja")
fi

cmake ../OPFpp "${CMAKE_FLAGS[@]}"

echo -e "\n=== 7. Compilando o Core C++ ==="
cmake --build . --config Release -j$(nproc 2>/dev/null || echo 4)

echo -e "\n=== 8. Instalando o wrapper do submódulo em Modo Editável ==="
cd ../OPFpp/pythonlib
$PYTHON_EXE -m pip install -e .

echo -e "\n========================================="
echo " Build concluído com sucesso no $OS_TYPE! "
echo "========================================="