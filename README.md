# Hello World C++ Project

A simple "Hello, World!" project in C++ using CMake.

## How to Build

### Prerequisites

*   A C++ compiler (like GCC, Clang, or MSVC)
*   CMake (version 3.10 or later)

### Build Steps

1.  **Clone the repository:**
    ```bash
    git clone <repository-url>
    cd helloworld
    ```

2.  **Configure CMake:**
    ```bash
    cmake -S . -B build
    ```

3.  **Build the project:**
    ```bash
    cmake --build build
    ```

4.  **Run the executable:**
    The executable `helloworld.exe` will be located in the `build/bin` directory.

## Visual Studio Code Setup

This project is set up to be built and debugged with Visual Studio Code.

### C/C++ Extension

For the best experience, install the [Microsoft C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) for VS Code. This extension provides features such as:

*   IntelliSense code completion
*   Debugging
*   Code navigation

### Using GCC with MinGW-w64 on Windows

To build and debug this project on Windows using GCC, you need to install the MinGW-w64 toolchain.

#### Prerequisites

1.  Install [Visual Studio Code](https://code.visualstudio.com/).
2.  Install the [C/C++ extension for VS Code](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools).

#### Installation

1.  **Install MinGW-w64 via MSYS2:**
    Follow the instructions on the [MSYS2 website](https://www.msys2.org/) to install MSYS2. Then, open an MSYS2 terminal and run the following command to install the MinGW-w64 toolchain:
    ```bash
    pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
    ```

2.  **Add MinGW to your PATH:**
    Add the path to your MinGW-w64 `bin` folder to the Windows PATH environment variable. If you installed MinGW-w64 to the default directory, this will be `C:\msys64\ucrt64\bin`.

3.  **Verify the installation:**
    Open a new Command Prompt or PowerShell and run the following commands to check that `g++` and `gdb` are correctly installed and in your PATH:
    ```bash
    g++ --version
    gdb --version
    ```

After these steps, you should be able to build and debug the project in VS Code using the provided launch and task configurations.
