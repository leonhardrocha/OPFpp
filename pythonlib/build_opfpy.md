# Building the opfpy Python Extension (C++/pybind11)

This guide describes how to configure and build the C++/pybind11-based Python extension module (opfpy) for the OPF library.

---

## Two-Environment Workflow

This project uses **two separate Python environments** for different purposes:

| Purpose | Environment | Python path |
|---------|------------|-------------|
| **Build** (compile C++, run Conan/CMake) | MSYS2 UCRT64 | `D:/msys64/ucrt64/bin/python3.14.exe` |
| **Test** (run Python unit tests) | Windows `.venv` | `pythonlib/.venv/Scripts/python.exe` |

> **IMPORTANT:** Always build from the **UCRT64 terminal** in VS Code (the `UCRT64` terminal profile defined in `.vscode/settings.json`). Never use WSL bash or the Windows `.venv` Python for building.

---

## Step 1: Open the UCRT64 Terminal

In VS Code, open a new terminal and select the **UCRT64** profile from the dropdown. This launches MSYS2 bash with:
- `PATH` including `D:/msys64/ucrt64/bin` and `D:/msys64/usr/bin`
- `MSYSTEM=UCRT64`
- All UCRT64 runtime DLLs available

---

## Step 2: Build the Extension (Automated)

From the project root in the UCRT64 terminal:
```sh
bash ./build_opfpy.sh
```

This script will:
1. Use `/ucrt64/bin/python3.14` to install Conan and run `conan install`
2. Configure CMake using the `gcc` preset (which uses `D:/msys64/ucrt64/bin/gcc.exe` and `g++.exe`)
3. Build the `opfpy` target, outputting `opfpy.pyd` to `pythonlib/bin`
4. Write the Cython dependency manifest to `pythonlib/bin/opfpy_cython_deps.txt`

---

## Step 3: Manual Build Steps (if needed)

All commands below must be run in the UCRT64 terminal:

```sh
# 1. Install Conan using UCRT64 Python
/ucrt64/bin/python3.14 -m pip install conan

# 2. Install dependencies with Conan
/ucrt64/bin/python3.14 -m conan install . --output-folder=build/gcc --build=missing -s build_type=Debug

# 3. Configure CMake with the gcc preset
/ucrt64/bin/cmake --preset gcc

# 4. Build the opfpy extension
/ucrt64/bin/cmake --build build/gcc --target opfpy
```

The resulting `opfpy.pyd` will be in `pythonlib/bin`.

---

## Step 4: Run Tests (Windows .venv)

After building, run tests using the **Windows `.venv`** (activate it first in a PowerShell terminal):

```powershell
# Activate the Windows .venv
.\pythonlib\.venv\Scripts\Activate.ps1

# Run tests from the pythonlib directory
cd pythonlib
python -m unittest -v test_opfpy_bindings.py
python -m unittest -v test_opfpy_cython.py
python -m unittest -v test_opfpy_distance.py
```

The test files automatically add `pythonlib/bin` to `sys.path` and register the UCRT64 DLL directories via `windows_runtime_helper.py`, so no manual `PYTHONPATH` setup is needed.

---

**Note:**
- Rebuild the extension after any changes to the C++ code or pybind11 bindings.
- Do NOT run `build_opfpy.sh` with Windows-native bash (WSL) — this will fail with `HCS_E_SERVICE_NOT_AVAILABLE`.
- For more details, see the main project README and `pythonlib_plan.md`.
