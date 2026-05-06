---

## Troubleshooting and Validation

- If the build fails, check:
  - CMake version (>=3.16)
  - Conan version (2.x, installed in the correct Python environment)
  - Compiler supports C++20 (GCC, Clang, or MSYS2/UCRT64 on Windows)
  - All dependencies are installed (see README_GNU_CMake.md for platform-specific setup)
- If the Python extension (opfpy) cannot be imported:
  - Ensure you are using the correct Python version and the .venv is activated
  - Add the build output directory (`pythonlib/bin`) to your PYTHONPATH or sys.path
  - Rebuild the extension after any changes to C++ or pybind11 code
- For more details, see [build_opfpy.md](pythonlib/build_opfpy.md) and the main project README.


# Prerequisites and Setup (C++/Python Integration)

All development and testing should be performed inside the `pythonlib` folder, but all core OPF logic is implemented in C++ and exposed to Python via pybind11. Pure Python implementations are deprecated and only used for test scaffolding or data handling.

**Directory structure:**
```
pythonlib/
  .python-version
  .venv/                # Virtual environment (created with uv)
  pyproject.toml        # Project metadata
  README.md
  src/
    pybind_stub.cpp     # pybind11 binding source
  include/
    opf/                # forwarding headers for the Python-facing build layout
  bin/
    opfpy.*             # Built Python extension module (from C++/pybind11)
    opfpy_cython_deps.txt
  lib/                  # Archive/import-library output for the extension target
  (Python test and helper scripts)
include_cpp/opf/         # C++ headers for OPF core
```

## Environment and Build Steps

1. **Install prerequisites:**
  - Python 3.14+ (recommended: use .venv in pythonlib)
  - [uv](https://github.com/astral-sh/uv) (for Python env and pip)
  - Conan 2 (installed via `uv pip install conan`)
  - CMake 3.16+
  - C++20 compiler (GCC, Clang, or MSYS2/UCRT64 on Windows)

2. **Create and activate the Python virtual environment:**
  ```sh
  cd pythonlib
  uv venv .venv
  # Windows:
  .venv\Scripts\activate
  # Linux/macOS:
  source .venv/bin/activate
  ```

3. **Install Conan 2 in the venv (if not already):**
  ```sh
  uv pip install conan
  ```



4. **Automated build (recommended for MSYS2/UCRT64):**
  - From the project root, run:
    ```sh
    bash ./build_opfpy.sh
    ```
  - This script will activate the venv, run Conan, configure CMake, build the extension into `pythonlib/bin`, and write the Cython dependency manifest to `pythonlib/bin/opfpy_cython_deps.txt`.
  - See [build_opfpy.md](pythonlib/build_opfpy.md) for manual step-by-step details if needed.


5. **Test the Python extension:**
  - See [build_opfpy.md](pythonlib/build_opfpy.md) for test and troubleshooting details.
  - Example:
    ```sh
    cd pythonlib
    python -c "import sys; sys.path.insert(0, './bin'); import opfpy; print(opfpy.hello())"
    ```

6. **Run Python tests (must use the C++ backend):**
  - All core OPF logic (distance, data structures, workflows) must call into the C++ extension (opfpy), not reimplement in Python.
  - Example:
    ```sh
    python -m unittest test_distance.py  # test_distance.py must use opfpy for all OPF logic
    ```

**Note:**
- If running from the project root, always `cd pythonlib` first for Python tests.
- The `.venv` folder must be created and activated before running any Python scripts.
- The C++ extension must be rebuilt after any changes to the C++ code or bindings.


# Python Library Development Plan for OPF (Optimum-Path Forest)

This plan outlines the phased development of a Python library for OPF workflows, using C++/pybind11 bindings for all core logic. Each phase includes checkboxes for implementation, testing, and manual validation. All validation must be against the C++ backend. At the end of all phases, user validation is required.

---


## Phase 1: Data Structures & File I/O (C++ backend)
- [x] Implement core data structures (`Node`, `Subgraph`) in C++ (see include_cpp/opf/Node.hpp, Subgraph.hpp)
- [x] Expose data structures to Python via pybind11 (expand pybind_stub.cpp)
- [x] Build `opfpy` from `pythonlib/src/pybind_stub.cpp` and output module artifacts to `pythonlib/bin`
- [x] Generate Cython dependency manifest in `pythonlib/bin/opfpy_cython_deps.txt`
- [x] Add dedicated `.pyx/.pxd` wrappers (optional Cython-facing layer) on top of `opfpy`
  - `pythonlib/src/opfpy_cython.pxd` — cdef class declarations (`Node`, `Subgraph`) for `cimport`
  - `pythonlib/src/opfpy_cython.pyx` — full Cython wrapper with typed properties, model I/O, and free functions
- [x] Integrate Cython-facing wrappers into `pythonlib` packaging and tests
  - CMakeLists.txt transpiles `.pyx` → `.cpp` via `cython --cplus -3`, then compiles to `pythonlib/bin/opfpy_cython.pyd`
  - `pythonlib/test_opfpy_cython.py` — 12 tests covering Node/Subgraph wrappers, model I/O, original-file loading, roundtrip read/write, and free functions
- [x] Implement OPF binary dataset file reader/writer in C++ and expose to Python
- [x] Unit tests for data structures and file I/O (Python tests must use opfpy)
- [x] **Test Results:**
  - `python -m unittest test_opfpy_bindings -v` — **7 tests, OK** (Node/Subgraph bindings, model I/O, original-file loading, roundtrip read/write, file creation, missing-file error handling)
  - `python -m unittest test_opfpy_cython -v` — **12 tests, OK** (Cython Node/Subgraph wrappers, `_raw` access, model I/O roundtrip, original-file loading, free-function read/write with both `opfpy.Subgraph` and `cy.Subgraph`)
  - Combined: `python -m unittest test_opfpy_bindings test_opfpy_cython -v` — **19 tests, OK**
- [x] **Validation Requirements:**
  - [x] Can load and save OPF datasets without data loss (C++/Python roundtrip)
  - [x] Data structures match expected fields and types (Python matches C++ reference)
- [x] **Merge with main branch**
  - [x] Push new commit to squash merge with main
---


## Phase 2: Distance Computation (C++ backend)
- [x] Implement distance functions (Euclidean, Chi-Square, etc.) in C++ (`include_cpp/opf/Distance.hpp`, `src_cpp/Distance.cpp`)
- [x] Expose distance functions to Python via pybind11 (`pythonlib/src/pybind_stub.cpp`)
  - `eucl_dist`, `chi_squared_dist`, `manhattan_dist`, `canberra_dist`, `squared_chord_dist`, `squared_chi_squared_dist`, `bray_curtis_dist`
- [ ] Implement distance matrix computation in C++ and expose to Python
- [x] Unit tests for distance functions (Python tests must use opfpy)
  - `pythonlib/test_opfpy_distance.py` — 8 tests covering all 7 distance functions + empty vector edge case
- [x] **Test Results:**
  - `python -m unittest test_opfpy_distance -v` — **8 tests, OK**
- [x] **Validation Requirements:**
  - [x] Distances match C++ implementation for sample data (Python vs C++)
  - [x] Handles edge cases (identical points, empty sets)

---


## Phase 3: Supervised OPF Workflow (C++ backend)
- [x] Expose dataset splitting (train/eval/test) from C++ to Python (`opfpy.split_subgraph`)
- [x] Expose supervised training (OPF train/learn) from C++ to Python (`opfpy.OPF().train`, `opfpy.OPF().learn`)
- [x] Expose classification (OPF classify) from C++ to Python (`opfpy.OPF().classify`)
- [x] Expose accuracy computation from C++ to Python (`opfpy.OPF().accuracy`)
- [x] Unit tests for each workflow step (Python tests must use opfpy)
  - `pythonlib/test_opfpy_supervised.py` — 3 tests covering split, train+classify+accuracy, and learn
- [x] **Test Results:**
  - `python -m unittest test_opfpy_supervised -v` — **3 tests, OK**
  - Bug found in C/C++ OPF classifier accuracy if not all labels are present in eval set (divides by zero → NaN). Fixed in Phase 5 (`accuracy()` in `OPF.hpp`).
  - No output files (.out, .acc) are  generated, it is diferent from the C version, but it is ok as it is object-based (C++)
- [x] **Validation Requirements:**
  - [x] Classification accuracy matches C++ reference for known datasets
  

---


## Phase 4: Unsupervised OPF Workflow (C++ backend)
- [x] Expose clustering (OPF cluster) from C++ to Python (`opfpy.OPF().cluster`)
- [x] Expose k-NN graph and best-k search from C++ to Python (`opfpy.OPF().knn_classify`)
  - Note: `bestk` search (BestKMinCut) is a placeholder in C++; `bestk` property is exposed on `Subgraph`
- [x] Expose label propagation from C++ to Python (`opfpy.propagate_cluster_labels`)
- [x] Expose semi-supervised learning from C++ to Python (`opfpy.OPF().semi_supervised`)
- [x] Unit tests for clustering and label propagation (Python tests must use opfpy)
  - `pythonlib/test_opfpy_unsupervised.py` — 6 tests covering clustering, nlabels, label propagation, k-NN classify, semi-supervised with and without eval
- [x] **Test Results:**
  - `python -m unittest test_opfpy_unsupervised -v` — **6 tests, OK**
- [x] **Validation Requirements:**
  - [x] Clustering results match C++ reference for sample data
  - [x] Cluster labels are correctly assigned and propagated

---


## Phase 5: Utilities & Tools (C++ backend)
- [x] Expose normalization, info, and fold utilities from C++ to Python
- [x] Expose precomputed distance file support from C++ to Python
- [x] Unit tests for utilities (Python tests must use opfpy)
- [x] Fix `accuracy()` divide-by-zero when a class covers all nodes (FP rate set to 0)
- [x] Fix `pruning()` to match upstream `opf_OPFPruning` semantics:
  - Added `classifyingAndMarkNodes` private helper that tracks the conqueror training node
  - Corrected loop condition to `fabs(current_acc - old_acc) <= desired_acc` (tolerance, not floor)
  - Added max-iterations cap (100)
  - Added per-iteration `setPred(-1)` + `setRelevant(0)` reset on training nodes
  - Added second `training()` call after pruning before measuring new accuracy
- [x] Source tree refactored: legacy C `src/` removed, `src_cpp/` renamed to `src/`
- [x] `LibOPF` upstream C reference added as git submodule
- [x] **Test Results:** 54/54 tests pass (Phases 1–5, no regressions)
- [x] **Validation Requirements:**
  - [x] Utilities produce correct outputs for sample data
  - [x] All file formats are compatible with C++/C reference

---


## Phase 6: Integration & Documentation
- [ ] Integrate all C++/pybind11 modules into a single Python package (opfpy)
- [ ] Write user documentation and API reference (Python and C++)
- [ ] Provide example scripts for all workflows (Python scripts using opfpy)
- [ ] **Test Results:** (to be filled by user)
- [ ] **Validation Requirements:**
  - [ ] All workflows run end-to-end on example datasets (Python calls C++ backend)
  - [ ] Documentation is clear and complete

---


## Final Validation
- [ ] All tests in each phase are passing (Python tests use C++ backend)
- [ ] User has manually checked all validation requirements
- [ ] User confirms the library is ready for release

---


> Please fill in the test results and validation checkboxes as you complete each phase. All validation must be against the C++ backend via opfpy. When all phases are complete and validated, the library is ready for use!
**Important:**
> All CMake and Conan commands must be run from the MSYS2 UCRT64 environment (using the UCRT64 shell), **not** from the Windows native CMake or command prompt. Running CMake/Conan from the wrong environment will cause path errors, generator issues, and build failures. Always launch the UCRT64 shell and activate your Python environment before building or installing dependencies.
