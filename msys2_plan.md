### **Problem Analysis**

The project was failing to build with GCC and later with MSVC due to two main issues:
1.  **Fragmented CMake Project:** The project was not a single, unified CMake project. It was a collection of components linked only by a Visual Studio solution, which is not portable.
2.  **Incorrect Dependency Management:** An initial attempt to fix the project used `FetchContent` to build dependencies from source. This was a departure from the project's use of Conan (as defined in `conanfile.txt`) and introduced numerous toolchain, policy, and compilation errors.

### **Final Corrective Plan**

The successful strategy was to restore the project's original dependency management system (Conan) and integrate it correctly into a unified CMake project.

1.  **Unify the Project with a Root `CMakeLists.txt`:**
    *   **Action:** A new root `CMakeLists.txt` was created to define the project and all its sub-components (`src`, `apps`, `python`, `tests`).
    *   **Modification:** This file uses `find_package()` to locate dependencies. It assumes that the `conan install` command has been run beforehand to make these dependencies available to CMake.

2.  **Integrate Conan via `CMakePresets.json`:**
    *   **Action:** The `CMakePresets.json` file was modified to integrate with the Conan-generated toolchain.
    *   **Modification:** The presets (`msvc`, `gcc`) were simplified to remove all hardcoded compiler flags, paths, and policies. They now point the `CMAKE_TOOLCHAIN_FILE` variable to the `conan_toolchain.cmake` file that `conan install` generates in the build directory. This delegates all toolchain and dependency configuration to Conan.

3.  **Establish New Build Workflow:**
    *   **Action:** The project now requires a two-step build process.
    *   **Workflow:** 
        1. Run `conan install` for the desired configuration to download dependencies and generate the CMake toolchain. 
        2. Run `cmake --preset <name>` and `cmake --build <build_dir>` as usual.

### **Update: Correcting CMake Target Names in Sub-Projects**

A final configuration error was found in the `tests/cpp/CMakeLists.txt` file.

4.  **Update Test Library Linkage:**
    *   **Action:** The test executables were attempting to link against incorrect or outdated library target names.
    *   **Modification:** The `target_link_libraries` command in `tests/cpp/CMakeLists.txt` will be updated to use the correct target names: `pyopf_static` for the main project library and `gtest::gtest_main` for the Google Test library provided by Conan.