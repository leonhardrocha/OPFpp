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