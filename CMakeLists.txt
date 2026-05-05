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
foreach(_d ${OPFPY_SRC_DIR} ${OPFPY_INCLUDE_DIR}
           ${OPFPY_BIN_DIR} ${OPFPY_LIB_DIR})
    file(MAKE_DIRECTORY "${_d}")
endforeach()

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
    ${OPFPY_SRC_DIR}/pybind_stub.cpp
    src_cpp/file.cpp
)

target_include_directories(opfpy PRIVATE
    "${OPFPY_INCLUDE_DIR}"
    "${CMAKE_SOURCE_DIR}/include_cpp"
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

set(OPF_PYBIND11_INCLUDE_DIRS "${pybind11_INCLUDE_DIRS}")
if(NOT OPF_PYBIND11_INCLUDE_DIRS)
    set(OPF_PYBIND11_INCLUDE_DIRS "${pybind11_INCLUDE_DIR}")
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
# Step 1: transpile .pyx -> .cpp using cython --cplus
set(CYTHON_PYX "${OPFPY_SRC_DIR}/opfpy_cython.pyx")
set(CYTHON_CPP "${CMAKE_BINARY_DIR}/opfpy_cython.cpp")

find_program(CYTHON_EXECUTABLE NAMES cython3 cython HINTS
    "${Python_EXECUTABLE}/../.."
    "$ENV{VIRTUAL_ENV}/Scripts"
    "$ENV{VIRTUAL_ENV}/bin"
    NO_DEFAULT_PATH
)
if(NOT CYTHON_EXECUTABLE)
    # Fallback: derive from Python interpreter path
    get_filename_component(_py_bin "${Python_EXECUTABLE}" DIRECTORY)
    find_program(CYTHON_EXECUTABLE NAMES cython3 cython
        HINTS "${_py_bin}" NO_DEFAULT_PATH)
endif()
if(NOT CYTHON_EXECUTABLE)
    message(FATAL_ERROR "Cython executable not found. Install it with: uv pip install cython")
endif()
message(STATUS "Cython found: ${CYTHON_EXECUTABLE}")

add_custom_command(
    OUTPUT  "${CYTHON_CPP}"
    COMMAND "${CYTHON_EXECUTABLE}" --cplus -3
            "${CYTHON_PYX}"
            -o "${CYTHON_CPP}"
    DEPENDS "${CYTHON_PYX}" "${OPFPY_SRC_DIR}/opfpy_cython.pxd"
    COMMENT "Transpiling opfpy_cython.pyx -> opfpy_cython.cpp"
)

# Step 2: compile the generated C++ into a Python extension
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

# opfpy_cython depends on opfpy being built first (runtime import)
add_dependencies(opfpy_cython opfpy)