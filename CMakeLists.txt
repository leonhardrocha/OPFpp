cmake_minimum_required(VERSION 3.16)

# 1. Defina onde o toolchain REALMENTE deve estar (dentro da pasta de build atual)
set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")

# 2. Se não existir, roda a instalação
if(NOT EXISTS "${CONAN_TOOLCHAIN_PATH}")
    message(STATUS "Toolchain do Conan não encontrado em ${CONAN_TOOLCHAIN_PATH}. Gerando...")
    include(cmake/FindOrInstallMSYS2.cmake)
    include(cmake/FindOrInstallConan.cmake)
endif()

# 3. Força o CMake a usar o arquivo que acabou de ser gerado
set(CMAKE_TOOLCHAIN_FILE "${CONAN_TOOLCHAIN_PATH}" CACHE PATH "" FORCE)

project(LibOPF_CPP CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPFPY_SRC_DIR     "${CMAKE_SOURCE_DIR}/pythonlib/src")
set(OPFPY_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/pythonlib/include")
set(OPFPY_BIN_DIR     "${CMAKE_SOURCE_DIR}/pythonlib/bin")
set(OPFPY_LIB_DIR     "${CMAKE_SOURCE_DIR}/pythonlib/lib")
foreach(_d ${OPFPY_SRC_DIR} ${OPFPY_INCLUDE_DIR} ${OPFPY_BIN_DIR} ${OPFPY_LIB_DIR})
    file(MAKE_DIRECTORY "${_d}")
endforeach()

# ---------------------------------------------------------------------------
# Configuração Isolada do Python & Pybind11 (.venv do Windows)
# ---------------------------------------------------------------------------
# CORREÇÃO: O seu .venv real está localizado em pythonlib/.venv
if(DEFINED ENV{VIRTUAL_ENV})
    set(MY_VENV_PATH "$ENV{VIRTUAL_ENV}")
else()
    # Fallback exato para a estrutura do seu projeto
    set(MY_VENV_PATH "${CMAKE_SOURCE_DIR}/pythonlib/.venv")
endif()

# Força o CMake a olhar APENAS para dentro da pasta correta do seu .venv
set(Python_ROOT_DIR "${MY_VENV_PATH}")
set(Python_FIND_STRATEGY LOCATION)
set(Python_FIND_VIRTUALENV ONLY) # IMPEDE terminantemente buscar o Python do MSYS2
set(Python_FIND_REGISTRY NEVER)   # Ignora o registro global do Windows

# Salva caminhos originais para restaurar depois
set(OLD_CMAKE_PROGRAM_PATH ${CMAKE_PROGRAM_PATH})
set(CMAKE_PROGRAM_PATH "${MY_VENV_PATH}/Scripts" "${MY_VENV_PATH}/bin")

# Encontra o interpretador do seu .venv do Windows (Python 3.14.3)
find_package(Python REQUIRED COMPONENTS Interpreter Development)
message(STATUS "[PROTETOR] Python REAL do .VENV encontrado: ${Python_EXECUTABLE}")

# Localiza o pybind11 instalado via pip/uv no site-packages do Windows
set(pybind11_DIR "${MY_VENV_PATH}/Lib/site-packages/pybind11/share/cmake/pybind11")
set(PYBIND11_FINDPYTHON ON)

# Append the exact folder where Conan 2 aggregates module configuration files
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/build/Debug/generators")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/generators")

# Now CMake can successfully resolve pybind11
find_package(pybind11 REQUIRED)
message(STATUS "[PROTETOR] Pybind11 do .VENV carregado de: ${pybind11_DIR}")

# Restaura os caminhos padrões do CMake para os passos seguintes do compilador
set(CMAKE_PROGRAM_PATH ${OLD_CMAKE_PROGRAM_PATH})

# ---------------------------------------------------------------------------
# Target Principal: opfpy
# ---------------------------------------------------------------------------
add_library(opfpy MODULE
    ${OPFPY_SRC_DIR}/pybind_stub.cpp
    src/file.cpp
    src/Distance.cpp
)

target_include_directories(opfpy PRIVATE
    "${OPFPY_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(opfpy PRIVATE
    pybind11::module
)

set_target_properties(opfpy PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${OPFPY_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OPFPY_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OPFPY_LIB_DIR}"
)

if(WIN32)
    set_target_properties(opfpy PROPERTIES SUFFIX ".pyd")
endif()

# Fallbacks de Include para segurança
set(OPF_PYBIND11_INCLUDE_DIRS "${pybind11_INCLUDE_DIRS}")
if(NOT OPF_PYBIND11_INCLUDE_DIRS)
    set(OPF_PYBIND11_INCLUDE_DIRS "${pybind11_include_dir}")
endif()
if(NOT OPF_PYBIND11_INCLUDE_DIRS)
    set(OPF_PYBIND11_INCLUDE_DIRS "${Python_INCLUDE_DIRS}")
endif()

set(OPF_DEPS_FILE "${OPFPY_BIN_DIR}/opfpy_cython_deps.txt")
add_custom_command(TARGET opfpy POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${OPF_DEPS_FILE}
        -DPYTHON_MODULE_FILE=$<TARGET_FILE:opfpy>
        -DPYBIND11_INCLUDE_DIRS=${OPF_PYBIND11_INCLUDE_DIRS}
        -P ${CMAKE_SOURCE_DIR}/cmake/WriteCythonDeps.cmake
)

# ---------------------------------------------------------------------------
# Cython wrapper: opfpy_cython
# ---------------------------------------------------------------------------
# Passo 1: Transpilar .pyx -> .cpp usando o cython.exe do .venv
set(CYTHON_PYX "${OPFPY_SRC_DIR}/opfpy_cython.pyx")
set(CYTHON_CPP "${CMAKE_BINARY_DIR}/opfpy_cython.cpp")

# No Windows, executáveis instalados via pip/uv ficam estritamente em .venv/Scripts
find_program(CYTHON_EXECUTABLE 
    NAMES cython.exe cython3 cython
    HINTS "${MY_VENV_PATH}/Scripts"
    NO_DEFAULT_PATH # Bloqueia o MSYS2 de injetar o cython deles
)

# --- Detecção Robusta e Nativa do Cython ---
# Pergunta diretamente ao Python do .venv se o Cython consegue ser importado
execute_process(
    COMMAND "${Python_EXECUTABLE}" -c "import sys; import Cython; print(Cython.__version__)"
    OUTPUT_VARIABLE CYTHON_VERSION_DETECTED
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE CYTHON_IMPORT_RESULT
)

if(CYTHON_IMPORT_RESULT EQUAL 0)
    message(STATUS "Cython encontrado via Python venv! Versão: ${CYTHON_VERSION_DETECTED}")
    
    # Se o seu script precisar do caminho absoluto para o executável do cython 'binário':
    if(WIN32)
        set(CYTHON_EXECUTABLE "${Python_ROOT_DIR}/Scripts/cython.exe")
    else()
        set(CYTHON_EXECUTABLE "${Python_ROOT_DIR}/bin/cython")
    endif()
    
    message(STATUS "Usando executável do Cython em: ${CYTHON_EXECUTABLE}")
else()
    message(FATAL_ERROR "Cython não encontrado no .venv (${Python_ROOT_DIR}). Instale com: uv pip install cython")
endif()


if(NOT CYTHON_EXECUTABLE)
    message(FATAL_ERROR "Cython não encontrado no .venv (${MY_VENV_PATH}). Instale com: uv pip install cython")
endif()
message(STATUS "[PROTETOR] Cython do .VENV encontrado com sucesso: ${CYTHON_EXECUTABLE}")

add_custom_command(
    OUTPUT  "${CYTHON_CPP}"
    COMMAND "${CYTHON_EXECUTABLE}" --cplus -3
            "${CYTHON_PYX}"
            -o "${CYTHON_CPP}"
    DEPENDS "${CYTHON_PYX}" "${OPFPY_SRC_DIR}/opfpy_cython.pxd"
    COMMENT "Transpiling opfpy_cython.pyx -> opfpy_cython.cpp"
)

# Passo 2: Compilar o C++ gerado em uma extensão Python (.pyd)
add_library(opfpy_cython MODULE "${CYTHON_CPP}")

target_include_directories(opfpy_cython PRIVATE
    "${Python_INCLUDE_DIRS}"
    "${OPF_PYBIND11_INCLUDE_DIRS}"
)

target_link_libraries(opfpy_cython PRIVATE
    pybind11::module
)

set_target_properties(opfpy_cython PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${OPFPY_BIN_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OPFPY_BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OPFPY_LIB_DIR}"
)

if(WIN32)
    set_target_properties(opfpy_cython PROPERTIES SUFFIX ".pyd")
endif()

# Garante que opfpy seja construído primeiro devido à dependência em tempo de execução
add_dependencies(opfpy_cython opfpy)
