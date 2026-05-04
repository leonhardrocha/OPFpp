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

set(OPF_3RDPARTY_SRC_DIR "${CMAKE_SOURCE_DIR}/tools/3rdparty/src")
set(OPF_3RDPARTY_BIN_DIR "${CMAKE_SOURCE_DIR}/tools/3rdparty/bin")
file(MAKE_DIRECTORY "${OPF_3RDPARTY_SRC_DIR}")
file(MAKE_DIRECTORY "${OPF_3RDPARTY_BIN_DIR}")

set(Python_FIND_STRATEGY LOCATION)
set(Python_FIND_VIRTUALENV FIRST)
if(DEFINED ENV{VIRTUAL_ENV})
    set(Python_ROOT_DIR "$ENV{VIRTUAL_ENV}")
endif()

set(PYBIND11_FINDPYTHON ON)
find_package(Python REQUIRED COMPONENTS Interpreter Development)
# Tenta encontrar o utilitário config no ambiente UCRT64
find_program(PYBIND11_CONFIG NAMES pybind11-config PATHS "D:/msys64/ucrt64/bin" NO_DEFAULT_PATH)

if(PYBIND11_CONFIG)
    # Executa o utilitário para obter o diretório do CMake
    execute_process(
        COMMAND ${PYBIND11_CONFIG} --cmakedir
        OUTPUT_VARIABLE PYBIND11_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Executa para obter o sufixo da extensão (ex: .cp314-mingw_x86_64.pyd)
    execute_process(
        COMMAND ${PYBIND11_CONFIG} --extension-suffix
        OUTPUT_VARIABLE PY_EXT_SUFFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message(STATUS "Pybind11 Config encontrado: ${PYBIND11_DIR}")
    
    # Define o caminho para o find_package encontrar a versão do UCRT64
    set(pybind11_DIR "${PYBIND11_DIR}")
endif()

find_package(pybind11 REQUIRED)



add_library(opfpy MODULE
    src_cpp/pybind_stub.cpp
    src_cpp/file.cpp
)

target_include_directories(opfpy PRIVATE
    "${CMAKE_SOURCE_DIR}/include_cpp"
)

target_link_libraries(opfpy PRIVATE
    pybind11::module
)

set_target_properties(opfpy PROPERTIES
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${OPF_3RDPARTY_SRC_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${OPF_3RDPARTY_SRC_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${OPF_3RDPARTY_BIN_DIR}"
)

if(WIN32)
    set_target_properties(opfpy PROPERTIES SUFFIX ".pyd")
endif()

get_target_property(OPF_PYBIND11_INCLUDE_DIRS pybind11::headers INTERFACE_INCLUDE_DIRECTORIES)
if(NOT OPF_PYBIND11_INCLUDE_DIRS)
    get_target_property(OPF_PYBIND11_INCLUDE_DIRS pybind11::pybind11 INTERFACE_INCLUDE_DIRECTORIES)
endif()

set(OPF_DEPS_FILE "${OPF_3RDPARTY_BIN_DIR}/opfpy_cython_deps.txt")
add_custom_command(TARGET opfpy POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${OPF_DEPS_FILE}
        -DPYTHON_MODULE_FILE=$<TARGET_FILE:opfpy>
        -DPYBIND11_INCLUDE_DIRS=${OPF_PYBIND11_INCLUDE_DIRS}
        -P ${CMAKE_SOURCE_DIR}/cmake/WriteCythonDeps.cmake
)