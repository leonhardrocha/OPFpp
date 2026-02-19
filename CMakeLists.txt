cmake_minimum_required(VERSION 3.16)
project(LibOPF_CPP CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the OPF C++ library
# Core library (implementation used by executables and tests)
# Note: OPF.hpp is header-only with inline implementations; don't compile OPF.cpp
add_library(opf_core
    src_cpp/file.cpp
    src_cpp/Distance.cpp
)

target_include_directories(opf_core PUBLIC include_cpp)

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
# Each executable includes its own src_cpp/opf_*.cpp with main() defined (not guarded)
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

# Test API wrappers are implemented in the src_cpp files; no separate test library needed.

# Testing
enable_testing()

# Configure test helper header with the build directory so tests can call
# executables by absolute path (works across platforms and when CTest runs).
configure_file(${CMAKE_SOURCE_DIR}/tests_cpp/opf_test_config.h.in
               ${CMAKE_BINARY_DIR}/opf_test_config.h @ONLY)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/tests_cpp)

add_executable(test_example1 tests_cpp/test_example1.cpp)
target_sources(test_example1 PRIVATE
    src_cpp/opf_split.cpp
    src_cpp/opf_train.cpp
    src_cpp/opf_classify.cpp
    src_cpp/opf_accuracy.cpp
)
target_compile_definitions(test_example1 PRIVATE OPF_UNIT_TEST)
target_link_libraries(test_example1 opf_core)
add_test(NAME test_example1 COMMAND $<TARGET_FILE:test_example1>)

add_executable(test_example2 tests_cpp/test_example2.cpp)
target_sources(test_example2 PRIVATE
    src_cpp/opf_split.cpp
    src_cpp/opf_learn.cpp
    src_cpp/opf_classify.cpp
    src_cpp/opf_accuracy.cpp
)
target_compile_definitions(test_example2 PRIVATE OPF_UNIT_TEST)
target_link_libraries(test_example2 opf_core)
add_test(NAME test_example2 COMMAND $<TARGET_FILE:test_example2>)

add_executable(test_example3 tests_cpp/test_example3.cpp)
target_sources(test_example3 PRIVATE
    src_cpp/opf_distance.cpp
    src_cpp/opf_split.cpp
    src_cpp/opf_train.cpp
    src_cpp/opf_classify.cpp
    src_cpp/opf_accuracy.cpp
)
target_compile_definitions(test_example3 PRIVATE OPF_UNIT_TEST)
target_link_libraries(test_example3 opf_core)
add_test(NAME test_example3 COMMAND $<TARGET_FILE:test_example3>)

add_executable(test_example4 tests_cpp/test_example4.cpp)
target_sources(test_example4 PRIVATE
    src_cpp/opf_distance.cpp
    src_cpp/opf_split.cpp
    src_cpp/opf_learn.cpp
    src_cpp/opf_classify.cpp
    src_cpp/opf_accuracy.cpp
)
target_compile_definitions(test_example4 PRIVATE OPF_UNIT_TEST)
target_link_libraries(test_example4 opf_core)
add_test(NAME test_example4 COMMAND $<TARGET_FILE:test_example4>)

add_executable(test_example5 tests_cpp/test_example5.cpp)
target_sources(test_example5 PRIVATE
    src_cpp/opf_split.cpp
    src_cpp/opf_cluster.cpp
    src_cpp/opf_knn_classify.cpp
    src_cpp/opf_accuracy.cpp
)
target_compile_definitions(test_example5 PRIVATE OPF_UNIT_TEST)
target_link_libraries(test_example5 opf_core)
add_test(NAME test_example5 COMMAND $<TARGET_FILE:test_example5>)

add_executable(test_example6 tests_cpp/test_example6.cpp)
target_sources(test_example6 PRIVATE
    src_cpp/opf_split.cpp
    src_cpp/opf_semi.cpp
    src_cpp/opf_classify.cpp
    src_cpp/opf_accuracy.cpp
)
target_compile_definitions(test_example6 PRIVATE OPF_UNIT_TEST)
target_link_libraries(test_example6 opf_core)
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