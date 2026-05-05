# Building the opfpy Python Extension (C++/pybind11)

This guide describes how to configure and build the C++/pybind11-based Python extension module (opfpy) for the OPF library.

---

## Step 4: Configure and Build the Extension

### 1. Ensure prerequisites are installed
- Python 3.14+ (with .venv in pythonlib)
- uv (for Python environment and pip)
- Conan 2 (installed in the venv)
- CMake 3.16+
- C++20 compiler (GCC, Clang, or MSYS2/UCRT64 on Windows)

### 2. Install dependencies with Conan
From the project root:
```sh
conan install . --output-folder=build --build=missing
```
This will download pybind11 and generate the CMake toolchain file in `build/`.

### 3. Configure the CMake build
```sh
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
```
This sets up the build system using the Conan toolchain and finds all dependencies.

### 4. Build the opfpy extension
```sh
cmake --build build --target opfpy
```
The resulting Python extension (opfpy.pyd/.so) will be placed in `tools/3rdparty`.

### 5. Test the extension
From the `pythonlib` directory:
```sh
python -c "import sys; sys.path.insert(0, '../tools/3rdparty'); import opfpy; print(opfpy.hello())"
```
You should see: `Hello from OPF C++!`

---

**Note:**
- Rebuild the extension after any changes to the C++ code or pybind11 bindings.
- If you encounter build errors, check your compiler and CMake/Conan versions.
- For more details, see the main project README and pythonlib_plan.md.
