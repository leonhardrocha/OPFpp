
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

### settings.json
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

### c_cpp_properties.json
```json .vscode\c_cpp_properties.json

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

### launch.json
```json .vscode\launch.json
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

