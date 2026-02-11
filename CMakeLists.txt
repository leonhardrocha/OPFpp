# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.15)

# Project definition
project(pyopf LANGUAGES CXX)

# The C++ standard is set by the Conan toolchain.
# This is a fallback in case the project is configured without it.
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

## Python development components Starting with CMake 3.12
# Use the CMake-provided FindPython module to locate Python and its development components.
# This is required to build Python extensions and link against the Python library.
find_package(Python COMPONENTS Interpreter Development REQUIRED)
link_libraries(pybind11 PRIVATE Python::Python)

# Find dependencies provided by Conan.
# The Conan-generated CMakeDeps files make this possible.
# Add the Conan generators folder to the search path

find_package(GTest REQUIRED)
find_package(pybind11 REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# 3. Create your module
# Add project subdirectories.
# Their CMakeLists should link against the targets provided by find_package
# (e.g., pybind11::pybind11, GTest::gtest).
add_subdirectory(src)
add_subdirectory(python)
add_subdirectory(apps)
add_subdirectory(tests)
