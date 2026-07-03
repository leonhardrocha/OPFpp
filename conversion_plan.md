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

- **`GQueue` and `RealHeap` → `std::priority_queue` (done):** Both are replaced by `std::priority_queue` with lazy-deletion where decrease-key semantics are needed. The `color` bookkeeping (WHITE/GRAY/BLACK) is replaced by an explicit `std::vector<uint8_t>`.
- **`Set` (adjacency list) → `std::vector<int>` (done):** The linked-list `Set` used for kNN adjacency is replaced by `std::vector<int>` inside `Node::adj`.
- **`SgCTree` / `SgCTNode` → `opf::SgCTree` / `opf::SgCTNode` structs in `ComponentTree.hpp` (done):**  The C component-tree structs are ported to RAII-safe C++ structs using `std::vector` for children (`sons`). Helper functions `sgAreaOpen<T>`, `sgVolumeOpen<T>`, `sgCumSize`, `sgAreaLevel`, `sgVolumeLevel`, and `createSgMaxTree<T>` live in `include_cpp/opf/ComponentTree.hpp`.

## 2b. Algorithm Status

| C function | C++ equivalent | Status |
|---|---|---|
| `opf_OPFTraining` | `OPF::training` | ✅ done |
| `opf_OPFClassifying` | `OPF::classifying` | ✅ done |
| `opf_OPFknnClassify` | `OPF::knnClassifying` | ✅ done |
| `opf_OPFLearning` | `OPF::learning` | ✅ done |
| `opf_OPFPruning` | `OPF::pruning` | ✅ done |
| `opf_OPFSemiLearning` | `OPF::semiSupervisedLearning` | ✅ done |
| `opf_OPFClustering` | `OPF::clustering` | ✅ done |
| `opf_NormalizeFeatures` | `OPF::normalize` | ✅ done |
| `opf_Accuracy` | `OPF::accuracy` | ✅ done |
| `opf_MSTPrototypes` | `OPF::mstPrototypes` (private) | ✅ done |
| `opf_PDF` | `OPF::computePDF` | ✅ done — exposed as `OPF.compute_pdf(sg)` in Python |
| `opf_PDFtoKmax` | `OPF::pdfToKmax` (private) | ✅ done |
| `opf_CreateArcs` | `OPF::createArcs` | ✅ done |
| `opf_DestroyArcs` | `OPF::destroyArcs` | ✅ done |
| `opf_CreateArcs2` | `OPF::createArcs2` (private) | ✅ done |
| `opf_BestkMinCut` | `OPF::bestkMinCut` | ✅ done |
| `opf_NormalizedCut` | `OPF::normalizedCut` | ✅ done |
| `opf_NormalizedCutToKmax` | `OPF::normalizedCutToKmax` (private) | ✅ done |
| `opf_OPFClusteringToKmax` | `OPF::clusteringToKmax` (private) | ✅ done |
| `opf_ElimMaxBelowH` | `OPF::elimMaxBelowH` | ✅ done |
| `opf_ElimMaxBelowArea` | `OPF::elimMaxBelowArea` | ✅ done |
| `opf_ElimMaxBelowVolume` | `OPF::elimMaxBelowVolume` | ✅ done |
| `opf_EuclDist` | `distance::euclDist` | ✅ done |
| `opf_EuclDistLog` | `distance::euclDistLog` | ✅ done |
| `opf_GaussDist` | `distance::gaussDist` | ✅ done |
| `opf_ChiSquaredDist` | `distance::chiSquaredDist` | ✅ done |
| `opf_ManhattanDist` | `distance::manhattanDist` | ✅ done |
| `opf_CanberraDist` | `distance::canberraDist` | ✅ done |
| `opf_SquaredChordDist` | `distance::squaredChordDist` | ✅ done |
| `opf_SquaredChiSquaredDist` | `distance::squaredChiSquaredDist` | ✅ done |
| `opf_BrayCurtisDist` | `distance::brayCurtisDist` | ✅ done |
| `opf_ConfusionMatrix` | — | ⬜ not yet ported |
| `kMeans` | — | ⬜ not yet ported (not core OPF) |

## 2c. Python Binding Status

The following `OPF` class methods are exposed to Python via pybind11 (`pythonlib/src/pybind_stub.cpp`):

| C++ method | Python binding | Notes |
|---|---|---|
| `OPF::training` | `OPF.train(sg)` | Supervised training |
| `OPF::classifying` | `OPF.classify(train, test)` | Supervised classification |
| `OPF::learning` | `OPF.learn(train, eval)` | Iterative learning |
| `OPF::accuracy` | `OPF.accuracy(sg)` | Label vs truelabel accuracy |
| `OPF::createArcs` | `OPF.create_arcs(sg, k)` | Builds k-NN adjacency, `df`, `bestk`, and node `radius` |
| `OPF::destroyArcs` | `OPF.destroy_arcs(sg)` | Clears adjacency and plateau lists |
| `OPF::bestkMinCut` | `OPF.bestk_min_cut(sg, kmin, kmax)` | Native best-k search via normalized cut |
| `OPF::clustering` | `OPF.cluster(sg)` | Unsupervised clustering |
| `OPF::knnClassifying` | `OPF.knn_classify(train, test)` | k-NN classification |
| `OPF::semiSupervisedLearning` | `OPF.semi_supervised(labeled, unlabeled)` | Semi-supervised |
| `OPF::computePDF` | `OPF.compute_pdf(sg)` | Gaussian PDF density; requires `adj` and `sg.df` set |
| `OPF::normalize` | `OPF.normalize(sg)` | Z-score normalization |
| `OPF::pruning` | `OPF.pruning(train, eval, tol)` | Iterative node pruning |

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

## 5b. Deferred Type Refactor Tasks

The current C++ port preserves most of the original C member typing. A broader type-system cleanup is possible, but should be treated as explicit future work because it affects algorithms, binary I/O, bindings, and compatibility.

### Original C Member Groups

**`SNode` groups from `LibOPF/include/util/subgraph.h`:**

- `float`: `pathval`, `dens`, `radius`
- `int`: `label`, `root`, `pred`, `truelabel`, `position`, `nplatadj`
- `char`: `status`, `relevant`
- `float*`: `feat`
- `Set*`: `adj`

**`Subgraph` groups from `LibOPF/include/util/subgraph.h`:**

- `SNode*`: `node`
- `int`: `nnodes`, `nfeats`, `bestk`, `nlabels`
- `float`: `df`, `mindens`, `maxdens`, `K`
- `int*`: `ordered_list_of_nodes`

### Corresponding C++ Port Groups

- Scalar-cost group: `pathval`, `dens`, `radius`
- Signed integer state/index group: `label`, `root`, `pred`, `truelabel`, `position`, `nplatadj`
- Byte-flag group: `status`, `relevant`
- Feature-storage group: `feat`
- Adjacency group: `adj`
- Subgraph integer metadata group: `nfeats`, `bestk`, `nlabels`
- Subgraph float metadata group: `df`, `mindens`, `maxdens`, `K`
- Subgraph ordering group: `ordered_list_of_nodes`

### Future Tasks

- Replace all raw `-1` sentinel uses in the C++ port with `NIL` for consistency and auditability.
- Introduce explicit fixed-width type aliases in `common.hpp` for `Node` and `Subgraph` state instead of bare `int` and `char`.
- Keep sentinel-bearing fields signed, especially `pred`, and likely `root`, using `int32_t` rather than `uint32_t`.
- Treat `nplatadj` as a candidate for `uint32_t`, but only after verifying all arithmetic, serialization, and bindings.
- Defer any change to `label` and `truelabel` until label-domain assumptions and file-format compatibility are reviewed.
- Keep `status` and `relevant` as byte-sized flags (`uint8_t`) unless a stricter enum or boolean wrapper is introduced.
- Defer refactoring `pathval`, `dens`, and `radius` into configurable numeric aliases or template parameters until OPF algorithm internals and binary model I/O are updated together.
- Review Python bindings and tests alongside any C++ type migration so exposed property types and `NIL` semantics remain stable.
- If fixed-width integer migration is pursued, version or document the binary file format because current model I/O writes multiple fields using `sizeof(int)`.

### Notes

- `pred` currently depends on `NIL = -1`, so changing it to an unsigned type would obscure sentinel semantics.
- Two's-complement representation does not make `uint32_t(-1)` a good public sentinel design; comparisons, debugging, Python exposure, and intent all become less clear.
- `nplatadj` is the clearest non-negative field in the reviewed integer group.
- `status` and `relevant` are already low-risk byte flags.
- `pathval`, `dens`, and `radius` are used broadly as `float` throughout OPF internals, so changing their type is a broader algorithm-plus-I/O refactor, not a local `Node`-only edit.

## 6. Step-by-Step Conversion Roadmap

1.  **Project Setup:** Initialize a new C++20 project using CMake.
2.  **Core Data Structures:** Implement the templated `Node` and `Subgraph` classes.
3.  **Utility Conversion:** Replace the C utility data structures with their C++ STL counterparts.
4.  **Porting Logic:** Incrementally port the logic from the C functions (`OPF.c`, `util/*.c`) into methods of the new C++ classes.
5.  **Executables:** Rewrite the `opf_*.c` programs to use the new C++ library.
6.  **Testing:** Implement a suite of unit tests using a framework like GTest or Catch2 to ensure the correctness of the new implementation and prevent regressions.

By following this plan, LibOPF can be transformed into a modern, efficient, and maintainable C++20 library, well-suited for high-performance computing and large-scale data analysis.
