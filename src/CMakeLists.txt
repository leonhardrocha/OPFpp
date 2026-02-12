# src/CMakeLists.txt

# Find all source files for the library
file(GLOB OPF_SOURCES "*.cpp" "opf/*.cpp" "opf/common/*.cpp" "opf/supervised/*.cpp" "opf/unsupervised/*.cpp")

# Create the static library 'pyopf_static'
add_library(pyopf_static STATIC ${OPF_SOURCES})

# Make headers in 'include/' available to targets that link against 'pyopf_static'
target_include_directories(pyopf_static PUBLIC "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/include/opf")
target_link_libraries(pyopf_static PRIVATE nlohmann_json::nlohmann_json)