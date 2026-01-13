# Plan for Modernizing the OPF Codebase

## 1. Introduction

This document outlines a plan to modernize the Optimum-Path Forest (OPF) classifier codebase. The project's goal is to create a robust, cross-platform C++ library with Python bindings.

Currently, the codebase is in a transitional state, featuring modern C++ interfaces (like the `opf::Subgraph` class) but with a core logic that still relies on legacy C-style implementations (raw pointers, manual memory management, C-style I/O). This creates a disconnect and hinders maintainability and stability.

The primary goal of this plan is to complete the transition to modern, idiomatic C++, refactor the build system for full MSVC and GCC compatibility, and establish a solid foundation for testing and Python integration.

## 2. Phase 1: Unify Data Structures & Build System

This phase focuses on resolving the core inconsistencies in the codebase and ensuring it builds reliably on all target platforms.

### Task 2.1: Consolidate the `Subgraph` Data Structure
This is the most critical step to unify the codebase. The C-style functions must be refactored to work with the modern `opf::Subgraph` class.

- **Action:** Refactor all functions currently using `Subgraph *` and C-style struct access (e.g., `sg->nnodes`, `sg->node[i]`).
- **Action:** Convert these functions into methods of the `opf::Subgraph` class or helper functions that operate on it.
- **Action:** Replace all usage of `sg->nnodes` with `sg->nodes.size()` and `sg->node[i]` with `sg->nodes[i]`.
- **Action:** Remove any lingering old C-style `Subgraph` or `SNode` struct definitions. The primary data structure should be `opf::Subgraph` from `include/opf/Subgraph.hpp`.
- **Action:** Migrate the logic from C-style functions (e.g., `opf_OPFTraining`) into the corresponding `opf::Subgraph` class methods (e.g., `Subgraph::Train`).

### Task 2.2: Stabilize CMake for GCC
Address the compiler errors to enable cross-platform development and testing.

- **Action:** Verify the MSYS2 environment, ensuring the `g++` compiler is correctly installed and accessible in the system's PATH.
- **Action:** Update the `CMakePresets.json` for GCC to ensure it correctly identifies the Ninja build tool and the `g++` compiler via the Conan toolchain file.
- **Action:** Ensure that CMake's `find_package` commands for `GTest` and `pybind11` work seamlessly for both MSVC and GCC presets.

### Task 2.3: Modernize Core Data Structures (`RealHeap`, `GQueue`)
The helper data structures used by the OPF algorithms need to be brought up to modern C++ standards.

- **Action:** Analyze `RealHeap.hpp`/`.cpp` and `GQueue.hpp`/`.cpp`.
- **Action:** Refactor them from C-style structs with global functions (`CreateRealHeap`, `DestroyRealHeap`) into self-contained C++ classes.
- **Action:** Use `std::vector` for internal data storage instead of manually allocated arrays.
- **Action:** Implement constructors and destructors to manage resources automatically (RAII). Class methods should replace global functions (e.g., `heap.Insert(item)` instead of `InsertRealHeap(heap, item)`).

## 3. Phase 2: Refactor Core Logic to Idiomatic C++

This phase focuses on eliminating C-style patterns that are error-prone and verbose.

### Task 3.1: Eliminate Manual Memory Management
Replace all manual memory operations with safer, modern alternatives.

- **Action:** Systematically search the codebase for `malloc`, `calloc`, `free`, and raw `new`/`delete`.
- **Action:** Replace dynamically allocated arrays (e.g., `float*`) with `std::vector`.
- **Action:** Replace owning raw pointers (`Subgraph*`) returned by functions with `std::unique_ptr<Subgraph>`.
- **Action:** Refactor functions that take double pointers (`Subgraph** sg`) to modify the caller's pointer, and use references to smart pointers instead (e.g., `std::unique_ptr<Subgraph>& sg`).

### Task 3.2: Replace C-Style Practices
Adopt safer and more expressive C++ idioms.

- **Action:** Replace C-style I/O (`FILE*`, `fopen`, `fread`, `fwrite`) with C++ file streams (`<fstream>`). Serialization logic should be part of the `Subgraph` class.
- **Action:** Replace preprocessor macros (`MIN`, `MAX`, `NIL`) with `std::min`, `std::max`, and `nullptr` or scoped constants.
- **Action:** Eliminate global variables (e.g., `opf_PrecomputedDistance`, `opf_DistanceValue`). These should become member variables of the `opf::Subgraph` class or a new configuration struct.
- **Action:** Convert C-style function pointers (`DistanceFunction`) to `std::function` to provide more flexibility (e.g., allowing lambdas).

## 4. Phase 3: Enhance Generalization with Templates

Once the codebase is stable, C++ templates can be introduced to make the algorithms more generic and reusable.

### Task 4.1: Template the Distance Functions
- **Action:** Refactor the distance calculation functions (`EuclideanDistance`, etc.) into templates that can operate on iterators or generic container types, not just `std::vector<float>`.

### Task 4.2: Template Core Data Structures (Advanced)
- **Action:** As a further enhancement, consider templating `SNode` and `Subgraph` on the feature type (e.g., `template<typename FeatureType> class Subgraph`). This would allow the OPF algorithm to be used with different data types (`double`, `int`, etc.) without code duplication.

## 5. Phase 4: Testing and Python Integration

With a modernized codebase, testing and Python integration become more robust and manageable.

### Task 5.1: Expand C++ Unit Tests
- **Action:** Utilize the existing `gtest` framework to write comprehensive unit tests for the newly refactored classes (`Subgraph`, `RealHeap`, `GQueue`).
- **Action:** Add test cases that cover the core OPF algorithms: training, classification, and clustering.
- **Action:** Ensure tests are run for both MSVC and GCC builds in the CI pipeline.

### Task 5.2: Update and Verify Python Bindings
- **Action:** Review and update `python/opf_bindings.cpp`.
- **Action:** Use `pybind11` to expose the modern `opf::Subgraph` class and its methods to Python.
- **Action:** Pay close attention to object lifetime and ownership to prevent memory leaks between C++ and Python.
- **Action:** Create a `tests/python` directory and add a suite of `pytest` tests to validate the Python API, ensuring it feels intuitive and "Pythonic".
