---

## Troubleshooting and Validation

- If the build fails, check:
  - CMake version (>=3.16)
  - Conan version (2.x, installed in the correct Python environment)
  - Compiler supports C++20 (GCC, Clang, or MSYS2/UCRT64 on Windows)
  - All dependencies are installed (see README_GNU_CMake.md for platform-specific setup)
- If the Python extension (opfpy) cannot be imported:
  - Ensure you are using the correct Python version and the .venv is activated
  - Add the build output directory (../tools/3rdparty) to your PYTHONPATH or sys.path
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
  (Python test and helper scripts)
tools/3rdparty/
  opfpy.*               # Built Python extension module (from C++/pybind11)
src_cpp/
  pybind_stub.cpp       # pybind11 binding source (expand for full OPF API)
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
  - This script will activate the venv, run Conan, configure CMake, build the extension, and copy it to tools/3rdparty.
  - See [build_opfpy.md](pythonlib/build_opfpy.md) for manual step-by-step details if needed.


5. **Test the Python extension:**
  - See [build_opfpy.md](pythonlib/build_opfpy.md) for test and troubleshooting details.
  - Example:
    ```sh
    cd pythonlib
    python -c "import sys; sys.path.insert(0, '../tools/3rdparty'); import opfpy; print(opfpy.hello())"
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
- [x] Build `opfpy` from `src_cpp/pybind_stub.cpp` and output module artifacts to `tools/3rdparty/src`
- [x] Generate Cython dependency manifest in `tools/3rdparty/bin/opfpy_cython_deps.txt`
- [ ] Add dedicated `.pyx/.pxd` wrappers (optional Cython-facing layer) on top of `opfpy`
- [ ] Integrate Cython-facing wrappers into `pythonlib` packaging and tests
- [ ] Implement OPF binary dataset file reader/writer in C++ and expose to Python
- [ ] Unit tests for data structures and file I/O (Python tests must use opfpy)
- [ ] **Test Results:**
  - Pending re-run after CMake output path migration (`tools/3rdparty/src`) and dependency manifest generation (`tools/3rdparty/bin`)
- [ ] **Validation Requirements:**
  - [ ] Can load and save OPF datasets without data loss (C++/Python roundtrip)
  - [ ] Data structures match expected fields and types (Python matches C++ reference)

---


## Phase 2: Distance Computation (C++ backend)
- [ ] Implement distance functions (Euclidean, Chi-Square, etc.) in C++
- [ ] Expose distance functions to Python via pybind11
- [ ] Implement distance matrix computation in C++ and expose to Python
- [ ] Unit tests for distance functions and matrix (Python tests must use opfpy)
- [ ] **Test Results:**
- [ ] **Validation Requirements:**
  - [ ] Distances match C++ implementation for sample data (Python vs C++)
  - [ ] Handles edge cases (identical points, empty sets)

---


## Phase 3: Supervised OPF Workflow (C++ backend)
- [ ] Expose dataset splitting (train/eval/test) from C++ to Python
- [ ] Expose supervised training (OPF train/learn) from C++ to Python
- [ ] Expose classification (OPF classify) from C++ to Python
- [ ] Expose accuracy computation from C++ to Python
- [ ] Unit tests for each workflow step (Python tests must use opfpy)
- [ ] **Test Results:** (to be filled by user)
- [ ] **Validation Requirements:**
  - [ ] Classification accuracy matches C++ reference for known datasets
  - [ ] Output files (.out, .acc) are correctly generated

---


## Phase 4: Unsupervised OPF Workflow (C++ backend)
- [ ] Expose clustering (OPF cluster) from C++ to Python
- [ ] Expose k-NN graph and best-k search from C++ to Python
- [ ] Expose label propagation from C++ to Python
- [ ] Unit tests for clustering and label propagation (Python tests must use opfpy)
- [ ] **Test Results:** (to be filled by user)
- [ ] **Validation Requirements:**
  - [ ] Clustering results match C++ reference for sample data
  - [ ] Cluster labels are correctly assigned and propagated

---


## Phase 5: Utilities & Tools (C++ backend)
- [ ] Expose normalization, info, and fold utilities from C++ to Python
- [ ] Expose precomputed distance file support from C++ to Python
- [ ] Unit tests for utilities (Python tests must use opfpy)
- [ ] **Test Results:** (to be filled by user)
- [ ] **Validation Requirements:**
  - [ ] Utilities produce correct outputs for sample data
  - [ ] All file formats are compatible with C++/C reference

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
