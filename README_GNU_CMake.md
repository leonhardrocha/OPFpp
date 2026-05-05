
## Step-by-Step Guide: Configuring CMake with GNU Compiler on Windows (MSYS2) or Linux

This guide explains how to set up CMake to use the GNU compiler (GCC/G++) on both Windows (with MSYS2) and Linux, including recommended settings for Visual Studio Code.

---

## 1. Install CMake and GNU Toolchain

### Windows (MSYS2/UCRT64)
1. Open the MSYS2 UCRT64 terminal.
2. Install CMake and the GNU toolchain:
   ```sh
   pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-toolchain
   ```
3. (Optional) For faster builds, install Ninja:
   ```sh
   pacman -S mingw-w64-ucrt-x86_64-ninja
   ```
4. Ensure the path `D:/msys64/ucrt64/bin` is in your Windows system `PATH`.

### Linux
1. Install CMake and the GNU toolchain using your package manager. For example:
   ```sh
   sudo apt update
   sudo apt install build-essential cmake ninja-build
   ```

---

## 2. Configure VS Code Settings

Open your `settings.json` in VS Code and add or update the following settings as needed:

### Set the CMake Executable Path
```json
"cmake.cmakePath": "D:/msys64/ucrt64/bin/cmake.exe"
```
*On Linux, this is usually just `cmake` if it is in your PATH.*

### Choose a Build Generator

#### Option 1: Use Ninja (Recommended)
```json
"cmake.generator": "Ninja",
"cmake.configureArgs": [
    "-DCMAKE_MAKE_PROGRAM=D:/msys64/ucrt64/bin/ninja.exe"
]
```
*On Linux, you can omit the path if `ninja` is in your PATH.*

#### Option 2: Use Unix Makefiles
1. Ensure `make` is installed:
   - Windows (MSYS2):
     ```sh
     pacman -S mingw-w64-ucrt-x86_64-make
     ```
   - Linux: Already included in `build-essential`.
2. Update settings:
```json
"cmake.generator": "Unix Makefiles",
"cmake.configureArgs": [
    "-DCMAKE_MAKE_PROGRAM=D:/msys64/ucrt64/bin/mingw32-make.exe"
]
```
*On Linux, use the default `make`.*

---

## 3. Set Up the Build Environment (MSYS2 Only)

To ensure the correct tools and DLLs are found, prepend the UCRT64 bin directory to the environment:
```json
"cmake.configureEnvironment": {
    "PATH": "D:/msys64/ucrt64/bin;D:/msys64/usr/bin;${env:PATH}"
},
"cmake.buildEnvironment": {
    "PATH": "D:/msys64/ucrt64/bin;D:/msys64/usr/bin;${env:PATH}"
}
```

---

## 4. Select the Correct Kit in VS Code (CMake Extension)

In the VS Code status bar, click on the CMake Kit selection (e.g., "No Kit Selected") and choose the kit that points to the UCRT64 GCC/G++ compilers.

---

## 5. Troubleshooting

- **CMake/Ninja/Make not found:**
  - Ensure the correct path is set in `settings.json` and the tools are installed.
- **Compiler sanity check fails:**
  - Make sure the environment variables are set as above so the compiler can find all required DLLs and tools.
- **Antivirus blocks build:**
  - Some antivirus software may block temporary executables created by CMake. Check your antivirus logs if builds fail unexpectedly.
- **Cache issues:**
  - Delete the build directory (e.g., `build/gcc`) and reconfigure if you encounter persistent errors.

---

## 6. VsCode CMake/Ninja Local Settings Examples

**In `.vscode/settings.json`:**

```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build/${kitName}/${buildType}",
    "cmake.configureOnOpen": true,
    "cmake.cmakePath": "D:/msys64/ucrt64/bin/cmake.exe",
    "cmake.generator": "Ninja",
    "cmake.additionalKits": [],
    "cmake.configureArgs": [
        "-DCMAKE_MAKE_PROGRAM=D:/msys64/ucrt64/bin/ninja.exe"
    ],
    "cmake.configureEnvironment": {
    "PATH": "D:/msys64/ucrt64/bin;D:/msys64/usr/bin;${env:PATH}"
    },
    "cmake.buildEnvironment": {
        "PATH": "D:/msys64/ucrt64/bin;D:/msys64/usr/bin;${env:PATH}"
    },
    "terminal.integrated.profiles.windows": {
        "UCRT64": {
            "path": "d:\\msys64\\usr\\bin\\bash.exe",
            "args": [
                "--login",
                "-i"
            ],
            "env": {
                "MSYSTEM": "UCRT64",
                "CHERE_INVOKING": "1"
            }
        }
    },
}
```

**In `.vscode/c_cpp_properties.json.json`**:

```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.22621.0",
            "compilerPath": "cl.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-msvc-x64",
            "configurationProvider": "ms-vscode.cmake-tools"
        },
        {
            "name": "GCC",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/include"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.22000.0",
            "compilerPath": "d:/msys64/ucrt64/bin/gcc.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-gcc-x64",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```

**In `.vscode/launch.json`**:

```json
{
    "version": "2.0.0",
    "configurations": [
        {
            "name": "CMake: Debug",
            "type": "cmake",
            "request": "launch",
            "targetName": "helloworld",
            "preLaunchTask": "CMake: Build"
        }
    ]
}
```
---

## 7. Additional Tips

- Always use forward slashes `/` or double backslashes `\\` in JSON paths on Windows.
- If you want to build from the terminal, make sure to open the MSYS2 UCRT64 terminal and that the correct `bin` directory is in your `PATH`.
- On Linux, most of these steps are handled by your package manager and default environment.
- Ensure CMake is available on your system. A couple of options are:

   - Download and install CMake in UCRT64 to use the GNU version. If using MSVC utilize the CMake that is bundled with a Visual Studio installation.

   - Install the [CMake Tools extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) from the Extensions pane within VS Code or from the VS Code Marketplace.

   - Ensure that either you've added your CMake executable to your PATH, or you've adjusted the cmake.cmakePath setting to point to your CMake executable.
---

You are now ready to configure and build your CMake project with the GNU toolchain on Windows (MSYS2) or Linux!

---

## 8. Integrating Python and pybind11 (Conan/MSYS2/VS Code)

When building Python extensions with **pybind11** (installed via Conan) and CMake using the **GNU toolchain (MSYS2/UCRT64)**, you may encounter issues if your Python is the official Windows version (e.g., installed via `uv` or python.org) rather than MSYS2's Python. This is because:

- Official Windows Python uses `.lib` files (MSVC format), while GCC (MinGW/UCRT64) expects `.a` or `.dll.a` libraries.
- Conan/pybind11's `FindPythonLibsNew.cmake` may fail to detect the correct Python library or reject the `.lib` file when using GCC.

### Step 1: Discover the Correct Python Paths

Open the **UCRT64** terminal in VS Code and run:

```bash
# Enter your venv directory
cd D:/src/3gs-mask-studio/OPFpp/pythonlib/

# Activate the venv (important for 'which' to work)
source .venv/Scripts/activate

# Capture the variables
PYTHON_BASE=$(python -c "import sys; print(sys.base_prefix.replace('\\\\', '/'))")
PYTHON_VER=$(python -c "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')")
PYTHON_EXE=$(which python)
```

### Step 2: Configure CMake Manually (with Conan Toolchain)

Run the following command in the same UCRT64 terminal. Note the explicit `-DPYTHON_LIBRARY` and `-DPYTHON_INCLUDE_DIR`:

```bash
D:/msys64/ucrt64/bin/cmake.exe \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=D:/src/3gs-mask-studio/OPFpp/build/gcc/conan_toolchain.cmake \
    -DCMAKE_C_COMPILER=d:/msys64/ucrt64/bin/gcc.exe \
    -DCMAKE_CXX_COMPILER=d:/msys64/ucrt64/bin/g++.exe \
    -DPYTHON_EXECUTABLE="$PYTHON_EXE" \
    -DPYTHON_INCLUDE_DIR="$PYTHON_BASE/include" \
    -DPYTHON_LIBRARY="$PYTHON_BASE/libs/python$PYTHON_VER.lib" \
    -S D:/src/3gs-mask-studio/OPFpp \
    -B D:/src/3gs-mask-studio/OPFpp/build/gcc
```

### Step 3: Integrate with VS Code (CMake Presets / Settings)

To make the VS Code "Configure" button work, add these variables to your `settings.json` or `CMakePresets.json`:

**In `.vscode/settings.json`:**

```json
"cmake.configureSettings": {
        "PYTHON_EXECUTABLE": "D:/src/3gs-mask-studio/OPFpp/pythonlib/.venv/Scripts/python.exe",
        "PYTHON_INCLUDE_DIR": "C:/Users/leonardo_rocha/AppData/Local/Programs/Python/Python312/include",
        "PYTHON_LIBRARY": "C:/Users/leonardo_rocha/AppData/Local/Programs/Python/Python312/libs/python312.lib"
}
```
*Make sure the `C:/Users/.../Python312` path matches your `$PYTHON_BASE` from Step 1.*

---

#### Why does "Python libraries not found" persist?

The `FindPythonLibsNew.cmake` script from Conan/pybind11 expects the library in `PYTHON_PREFIX/libs`. If your `PYTHON_PREFIX` is set to a `.venv` (e.g., created by `uv`), that folder **does not** contain a `libs` directory, so detection fails. By passing `PYTHON_LIBRARY` pointing to the real Python install (outside the venv), you bypass this broken check.

**Extra Tip:** If you get linker errors (e.g., "redefined symbols") after finding the library, you may need to install Python from MSYS2 (`pacman -S mingw-w64-ucrt-x86_64-python`). Try the above solution first to keep using your `uv` venv.

---

You are now ready to configure and build your CMake project with the GNU toolchain on Windows (MSYS2) or Linux, including Python/pybind11 integration!

