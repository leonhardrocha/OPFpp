# LibOPF C to C++20 Conversion Plan

This document outlines a plan to convert the LibOPF library from its current C implementation to modern C++20. The primary goals are to improve data structure efficiency, enhance portability, enable better handling of large datasets, and increase code safety and maintainability.

## 1. General Guidelines

- **Target C++20:** Leverage C++20 features like concepts, ranges, and improved smart pointers for more expressive and efficient code.
- **Safety and Simplicity:** Prioritize code that is easy to understand and maintain. Use modern C++ idioms to prevent common C-style errors, such as memory leaks and buffer overflows.
- **Performance:** Ensure high performance for large-scale datasets by focusing on efficient data structures, memory layout, and algorithms.
- **Modularity:** Encapsulate logic within classes and namespaces to create a clean, modular, and extensible API.

## 2. Data Structure Conversion

The core of the conversion will be redesigning the C-style data structures to leverage C++'s object-oriented and generic programming features.

### `Subgraph` and `SNode` -> `class Subgraph` and `class Node`

The `SNode` and `Subgraph` structs will be replaced by C++ classes.

**`Node` Class:**

A template `Node` class will be created to allow for flexibility in the feature vector type.

```cpp
template<typename T>
class Node {
public:
    // ... methods ...

private:
    float pathval;
    float dens;
    float radius;
    int label;
    int root;
    int pred;
    int truelabel;
    int position;
    char status;
    char relevant;
    int nplatadj;

    // Use a shared_ptr for feature vectors to avoid duplication.
    // This is especially efficient when multiple nodes might reference the same external feature data.
    std::shared_ptr<std::vector<T>> feat;

    // Adjacency list
    std::vector<int> adj;
};
```

**`Subgraph` Class:**

The `Subgraph` class will manage a collection of `Node` objects.

```cpp
template<typename T>
class Subgraph {
public:
    // ... methods for training, classification, etc. ...

private:
    std::vector<Node<T>> nodes;
    int nfeats;
    int bestk;
    int nlabels;
    float df;
    float mindens;
    float maxdens;
    float K;
    std::vector<int> ordered_list_of_nodes;
};
```

**Key Improvements:**

- **Templating:** Using templates (`typename T`) for feature vectors makes the library adaptable to different data types (e.g., `float`, `double`, or custom types) without code duplication.
- **Smart Pointers:** `std::shared_ptr` for `feat` allows multiple nodes to share the same feature data without copying, reducing memory footprint, especially with large feature vectors. It also handles memory deallocation automatically.
- **`std::vector`:** Using `std::vector` for collections of nodes and adjacency lists provides automatic memory management, bounds checking (optional), and a rich set of member functions.

### Utility Data Structures

The custom C implementations of utility data structures will be replaced with their more efficient and robust C++ Standard Library equivalents.

- **`GQueue` and `RealHeap` -> `std::priority_queue`:** The C++ priority queue is a container adapter that provides a heap-based implementation, making it a perfect replacement for both `GQueue` and `RealHeap`. It's optimized and part of the standard, ensuring portability.
- **`Set` -> `std::set` or `std::unordered_set`:** The linked-list based `Set` will be replaced by `std::set` (a balanced binary search tree) or `std::unordered_set` (a hash table) for more efficient set operations.
- **`SgCTree` -> `class ComponentTree`:** A new `ComponentTree` class will be created, using `std::vector` to manage children and `std::weak_ptr` for parent pointers to prevent circular references and memory leaks.

## 3. Memory Management and Performance

- **Data Alignment:** For performance-critical data, especially feature vectors, we can use `alignas` to ensure data structures are aligned to cache line boundaries, which can significantly speed up memory access.

  ```cpp
  alignas(64) std::vector<float> aligned_features;
  ```

- **Avoid Memory Duplication:**
  - Use references (`&` and `const&`) to pass large objects (like `Subgraph`) to functions to avoid costly copies.
  - Use `std::move` to efficiently transfer ownership of resources when copies are not needed.
- **RAII (Resource Acquisition Is Initialization):** C++'s RAII principle, embodied by classes and smart pointers, will be used throughout the codebase to ensure that resources (memory, files, etc.) are automatically released when they go out of scope, preventing leaks.

## 4. API and Functionality Conversion

The procedural C API will be refactored into an object-oriented C++ API.

- **From C-style functions to C++ methods:**
  - `opf_OPFTraining(Subgraph *sg)` will become `subgraph.train()`.
  - `opf_OPFClassifying(Subgraph *sgtrain, Subgraph *sg)` will become `classifier.classify(test_subgraph)`.
- **`opf_*.c` programs:** The executables in the `src` directory will be rewritten to be lightweight wrappers around the new C++ library, demonstrating its usage.

## 5. Build System

The existing `Makefile` should be replaced with a modern, cross-platform build system like **CMake**. This will make it easier to build the library on different operating systems and with different compilers, and also simplifies the management of dependencies.

## 6. Step-by-Step Conversion Roadmap

1.  **Project Setup:** Initialize a new C++20 project using CMake.
2.  **Core Data Structures:** Implement the templated `Node` and `Subgraph` classes.
3.  **Utility Conversion:** Replace the C utility data structures with their C++ STL counterparts.
4.  **Porting Logic:** Incrementally port the logic from the C functions (`OPF.c`, `util/*.c`) into methods of the new C++ classes.
5.  **Executables:** Rewrite the `opf_*.c` programs to use the new C++ library.
6.  **Testing:** Implement a suite of unit tests using a framework like GTest or Catch2 to ensure the correctness of the new implementation and prevent regressions.

By following this plan, LibOPF can be transformed into a modern, efficient, and maintainable C++20 library, well-suited for high-performance computing and large-scale data analysis.
