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

if(MSVC)
    # Otimizações para Visual Studio
    add_compile_options($<$<CONFIG:Release>:/Ox /Ot /GT /GL /Oi>)
else()
    # Fix climits.h warnings on GCC/Clang (Linux/macOS)
    add_compile_definitions(__STDC_WANT_IEC_60559_TYPES_EXT__)
    add_compile_options(-fno-stack-protector) 
    # Otimizações para GCC e Clang (Linux/macOS)
    # -O3: Otimização máxima
    # -march=native: Usa instruções específicas da sua CPU (ex: AVX2)
    # -flto: Link Time Optimization (otimiza entre arquivos .cpp)
    

    add_compile_options($<$<CONFIG:Release>:-O3 -march=native -flto -ffast-math)
    add_link_options($<$<CONFIG:Release>:-flto>)
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
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)