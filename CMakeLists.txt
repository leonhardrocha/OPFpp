
# Ensure uv and conan are installed (cross-platform)
include(cmake/FindOrInstallUV.cmake)


# --- Conan auto-install logic ---
set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
if(NOT EXISTS "${CONAN_TOOLCHAIN_PATH}")
    message(STATUS "Conan toolchain not found. Running Conan to install dependencies...")
    # Get Python include path using sysconfig
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('include'))"
        OUTPUT_VARIABLE PYTHON_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(ENV{PYTHON_INCLUDE_DIR} "${PYTHON_INCLUDE_DIR}")
    message(STATUS "PYTHON_INCLUDE_DIR set to: $ENV{PYTHON_INCLUDE_DIR}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
            PYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR}
            conan install "${CMAKE_SOURCE_DIR}" -pr:b=default -pr:h=default -s build_type=Debug --output-folder="${CMAKE_BINARY_DIR}" --build=missing
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _conan_result
    )
    if(NOT _conan_result EQUAL 0)
        message(FATAL_ERROR "Conan install failed. Please check your Conan setup.")
    endif()
endif()

# Conan/pybind11 integration
include(${CONAN_TOOLCHAIN_PATH} OPTIONAL)


# Explicitly set Python include and library paths for pybind11 (legacy FindPythonLibs compatibility)
set(Python_INCLUDE_DIR "C:/Users/leonardo_rocha/AppData/Roaming/uv/python/cpython-3.14-windows-x86_64-none/Include")
set(Python_LIBRARY "C:/Users/leonardo_rocha/AppData/Roaming/uv/python/cpython-3.14-windows-x86_64-none/libs/python314.lib")

# Find pybind11 (from Conan)
find_package(pybind11 REQUIRED)

# Example: Minimal Python extension module (to be expanded)
add_library(opfpy MODULE src_cpp/pybind_stub.cpp)
target_link_libraries(opfpy PRIVATE opf_core pybind11::module)
set_target_properties(opfpy PROPERTIES PREFIX "" SUFFIX ".pyd")
install(TARGETS opfpy DESTINATION tools/3rdparty)
cmake_minimum_required(VERSION 3.16)
project(LibOPF_CPP CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Força o CMake a priorizar o Python do ambiente virtual ativo
set(Python_FIND_STRATEGY LOCATION)
set(Python_FIND_REGISTRY LAST)

# Add GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0  # Use the latest stable release
)
FetchContent_MakeAvailable(googletest)

# Define the OPF C++ library
add_library(opf_core
    src_cpp/file.cpp
    src_cpp/Distance.cpp
    src_cpp/opf_split.cpp
    src_cpp/opf_train.cpp
    src_cpp/opf_classify.cpp
    src_cpp/opf_accuracy.cpp
    src_cpp/opf_distance.cpp
    src_cpp/opf_learn.cpp
    src_cpp/opf_cluster.cpp
    src_cpp/opf_knn_classify.cpp
    src_cpp/opf_semi.cpp
)

target_include_directories(opf_core PUBLIC include_cpp)

# Guard main() definitions in wrapper sources when building the library
target_compile_definitions(opf_core PRIVATE OPF_UNIT_TEST)

# Define executables
add_executable(opf_train_cpp src_cpp/opf_train.cpp)
add_executable(opf_classify_cpp src_cpp/opf_classify.cpp)
add_executable(opf_accuracy_cpp src_cpp/opf_accuracy.cpp)
add_executable(opf_distance_cpp src_cpp/opf_distance.cpp)
add_executable(opf_info_cpp src_cpp/opf_info.cpp)
add_executable(opf_normalize_cpp src_cpp/opf_normalize.cpp)
add_executable(opf_cluster_cpp src_cpp/opf_cluster.cpp)
add_executable(opf_knn_classify_cpp src_cpp/opf_knn_classify.cpp)
add_executable(opf_merge_cpp src_cpp/opf_merge.cpp)
add_executable(opf_split_cpp src_cpp/opf_split.cpp)
add_executable(opf_fold_cpp src_cpp/opf_fold.cpp)
add_executable(opf_learn_cpp src_cpp/opf_learn.cpp)
add_executable(opf_pruning_cpp src_cpp/opf_pruning.cpp)
add_executable(opf_semi_cpp src_cpp/opf_semi.cpp)

# Link executables against the OPF C++ library
target_link_libraries(opf_train_cpp opf_core)
target_link_libraries(opf_classify_cpp opf_core)
target_link_libraries(opf_accuracy_cpp opf_core)
target_link_libraries(opf_distance_cpp opf_core)
target_link_libraries(opf_info_cpp opf_core)
target_link_libraries(opf_normalize_cpp opf_core)
target_link_libraries(opf_cluster_cpp opf_core)
target_link_libraries(opf_knn_classify_cpp opf_core)
target_link_libraries(opf_merge_cpp opf_core)
target_link_libraries(opf_split_cpp opf_core)
target_link_libraries(opf_fold_cpp opf_core)
target_link_libraries(opf_learn_cpp opf_core)
target_link_libraries(opf_pruning_cpp opf_core)
target_link_libraries(opf_semi_cpp opf_core)

# Testing
enable_testing()

# Include GoogleTest
include(GoogleTest)

# Configure test helper header with the build directory so tests can call
# executables by absolute path (works across platforms and when CTest runs).
configure_file(${CMAKE_SOURCE_DIR}/tests_cpp/opf_test_config.h.in
               ${CMAKE_BINARY_DIR}/opf_test_config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tests_cpp)

# Define GoogleTest-based tests
add_executable(test_example1 tests_cpp/test_example1.cpp)
target_link_libraries(test_example1 opf_core gtest gtest_main)
add_test(NAME test_example1 COMMAND $<TARGET_FILE:test_example1>)

add_executable(test_example2 tests_cpp/test_example2.cpp)
target_link_libraries(test_example2 opf_core gtest gtest_main)
add_test(NAME test_example2 COMMAND $<TARGET_FILE:test_example2>)

add_executable(test_example3 tests_cpp/test_example3.cpp)
target_link_libraries(test_example3 opf_core gtest gtest_main)
add_test(NAME test_example3 COMMAND $<TARGET_FILE:test_example3>)

add_executable(test_example4 tests_cpp/test_example4.cpp)
target_link_libraries(test_example4 opf_core gtest gtest_main)
add_test(NAME test_example4 COMMAND $<TARGET_FILE:test_example4>)

add_executable(test_example5 tests_cpp/test_example5.cpp)
target_link_libraries(test_example5 opf_core gtest gtest_main)
add_test(NAME test_example5 COMMAND $<TARGET_FILE:test_example5>)

add_executable(test_example6 tests_cpp/test_example6.cpp)
target_link_libraries(test_example6 opf_core gtest gtest_main)
add_test(NAME test_example6 COMMAND $<TARGET_FILE:test_example6>)

set_tests_properties(
    test_example1
    test_example2
    test_example3
    test_example4
    test_example5
    test_example6
    PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)


