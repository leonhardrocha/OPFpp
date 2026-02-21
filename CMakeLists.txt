cmake_minimum_required(VERSION 3.16)
project(LibOPF_CPP CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_executable(test_ecs_components tests_cpp/test_ecs_components.cpp)
target_include_directories(test_ecs_components PRIVATE ${CMAKE_SOURCE_DIR}/include_cpp)
target_link_libraries(test_ecs_components gtest gtest_main)
add_test(NAME test_ecs_components COMMAND $<TARGET_FILE:test_ecs_components>)

add_executable(test_ecs_split_system tests_cpp/test_ecs_split_system.cpp)
target_include_directories(test_ecs_split_system PRIVATE ${CMAKE_SOURCE_DIR}/include_cpp)
target_link_libraries(test_ecs_split_system gtest gtest_main)
add_test(NAME test_ecs_split_system COMMAND $<TARGET_FILE:test_ecs_split_system>)

add_executable(test_ecs_io_system tests_cpp/test_ecs_io_system.cpp)
target_include_directories(test_ecs_io_system PRIVATE ${CMAKE_SOURCE_DIR}/include_cpp)
target_link_libraries(test_ecs_io_system gtest gtest_main)
add_test(NAME test_ecs_io_system COMMAND $<TARGET_FILE:test_ecs_io_system>)

add_executable(test_ecs_train_system tests_cpp/test_ecs_train_system.cpp)
target_include_directories(test_ecs_train_system PRIVATE ${CMAKE_SOURCE_DIR}/include_cpp)
target_link_libraries(test_ecs_train_system gtest gtest_main)
add_test(NAME test_ecs_train_system COMMAND $<TARGET_FILE:test_ecs_train_system>)

add_executable(test_ecs_accuracy_system tests_cpp/test_ecs_accuracy_system.cpp)
target_include_directories(test_ecs_accuracy_system PRIVATE ${CMAKE_SOURCE_DIR}/include_cpp)
target_link_libraries(test_ecs_accuracy_system gtest gtest_main)
add_test(NAME test_ecs_accuracy_system COMMAND $<TARGET_FILE:test_ecs_accuracy_system>)

# ECS ClassifySystem tests
add_executable(test_ecs_classify_system tests_cpp/test_ecs_classify_system.cpp)
target_include_directories(test_ecs_classify_system PRIVATE ${CMAKE_SOURCE_DIR}/include_cpp)
target_link_libraries(test_ecs_classify_system gtest gtest_main)
add_test(NAME test_ecs_classify_system COMMAND $<TARGET_FILE:test_ecs_classify_system>)

set_tests_properties(
    test_example1
    test_example2
    test_example3
    test_example4
    test_example5
    test_example6
    test_ecs_components
    test_ecs_split_system
    test_ecs_io_system
    test_ecs_train_system
    test_ecs_classify_system
    test_ecs_accuracy_system
    PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# ECS example executable (header-only ECS runtime with example usage)
add_executable(ecs_example src_cpp/ecs_example.cpp)
target_include_directories(ecs_example PRIVATE ${CMAKE_SOURCE_DIR}/include_cpp)

