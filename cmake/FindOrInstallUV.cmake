# FindOrInstallUV.cmake
# This CMake module checks for uv and installs it using the official installer if not found.
# It then installs conan using uv pip install conan in the pythonlib/.venv environment.

# --- Official UV installation methods (reference/documentation) ---
# Standalone installer (recommended):
#   Windows (PowerShell):
#     powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex"
#   macOS/Linux (bash):
#     /bin/bash -c "curl -LsSf https://astral.sh/uv/install.sh | sh"
#   Specific version (Windows):
#     powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/0.11.8/install.ps1 | iex"
#   Specific version (macOS/Linux):
#     /bin/bash -c "curl -LsSf https://astral.sh/uv/0.11.8/install.sh | sh"
#
# Homebrew:
#   brew install uv
#
# MacPorts:
#   sudo port install uv
#
# WinGet:
#   winget install --id=astral-sh.uv  -e
#
# Scoop:
#   scoop install main/uv
#
# PyPI (pipx):
#   pipx install uv
#
# PyPI (pip):
#   pip install uv
#
# Cargo (Rust):
#   cargo install --locked uv
#
# Direct download (GitHub Releases):
#   Download binaries or installer from https://github.com/astral-sh/uv/releases
#
# Docker:
#   ghcr.io/astral-sh/uv
#
# See https://docs.astral.sh/uv/getting-started/installation/ for details.


# --- Find or install uv ---
find_program(UV_EXECUTABLE NAMES uv PATHS $ENV{USERPROFILE} $ENV{HOME} PATH_SUFFIXES .local/bin bin)

if(NOT UV_EXECUTABLE)
    message(STATUS "uv not found, attempting to install...")
    if(WIN32)
        execute_process(COMMAND powershell -ExecutionPolicy ByPass -c "irm https://astral.sh/uv/install.ps1 | iex")
    elseif(APPLE)
        execute_process(COMMAND /bin/bash -c "curl -LsSf https://astral.sh/uv/install.sh | sh")
    else()
        execute_process(COMMAND /bin/bash -c "curl -LsSf https://astral.sh/uv/install.sh | sh")
    endif()
    find_program(UV_EXECUTABLE NAMES uv PATHS $ENV{USERPROFILE} $ENV{HOME} PATH_SUFFIXES .local/bin bin)
endif()

if(NOT UV_EXECUTABLE)
    message(FATAL_ERROR "uv could not be installed or found. Please install uv manually.")
else()
    message(STATUS "uv found at ${UV_EXECUTABLE}")
endif()

# --- Install conan using uv pip in pythonlib/.venv ---
execute_process(
    COMMAND ${UV_EXECUTABLE} pip install conan
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/pythonlib
    RESULT_VARIABLE CONAN_INSTALL_RESULT
)
if(NOT CONAN_INSTALL_RESULT EQUAL 0)
    message(WARNING "uv pip install conan failed. Attempting fallback using git submodule.")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/conan_src")
        execute_process(COMMAND git submodule add https://github.com/conan-io/conan conan_src WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
        execute_process(COMMAND git submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()
    execute_process(COMMAND ${UV_EXECUTABLE} pip install -e . WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/conan_src)
endif()
