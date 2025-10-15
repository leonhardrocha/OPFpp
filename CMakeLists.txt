cmake_minimum_required(VERSION 3.10)

project(HelloWorld)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories for all generators
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)


# --- Library ---
add_library(helloworld_lib src/helloworld.cpp)
target_include_directories(helloworld_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Executable ---
add_executable(helloworld src/main.cpp)
target_link_libraries(helloworld PRIVATE helloworld_lib)

# --- Testing ---
enable_testing()
add_executable(run_tests tests/test_helloworld.cpp)
target_link_libraries(run_tests PRIVATE helloworld_lib)
add_test(NAME HelloWorldLibTest COMMAND $<TARGET_FILE:run_tests>)
