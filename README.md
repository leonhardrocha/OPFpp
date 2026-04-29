# LibOPF - C++20 Edition

## Overview

LibOPF is a library of functions and programs for the design of **Optimum-Path Forest (OPF)** classifiers. This repository contains a modern **C++20 conversion** of the original C implementation, providing enhanced performance, safety, and maintainability while preserving the core algorithms and functionality of the original library.

### About the Original Library

The original LibOPF library was developed to support the supervised and unsupervised OPF classifier methods reported in the foundational research:

- **Supervised OPF:** [PapaIJIST09, PapaPR12]
- **Unsupervised OPF:** [RochaIJIST09]

For detailed information about the OPF methodology, visit: http://www.ic.unicamp.br/~afalcao/LibOPF

### The C++20 Conversion

This C++20 version modernizes the original C codebase with:

- **Modern C++20 features:** Leveraging concepts, ranges, and improved smart pointers for expressive, efficient code
- **Improved safety:** Eliminating common C-style errors (memory leaks, buffer overflows) through RAII and standard library containers
- **Better performance:** Optimized data structures and memory management for large-scale datasets
- **Object-oriented design:** Clean, modular API replacing procedural C functions
- **Cross-platform build system:** CMake replaces the original Makefile for easier portability

### C++ Version License

The C++20 conversion is authored by **Leonardo Marques Rocha**, one of the original authors of the OPF algorithm, and is made available under the same license as the original LibOPF library. Please refer to the [COPYING](COPYING) and [COPYRIGHT](COPYRIGHT) files for license details.

---

## OPF File Format for Datasets

The library works with datasets in a specific binary format. Each sample is represented as:

```
<# of samples> <# of labels> <# of features>
<0> <label> <feature 1> <feature 2> ... <feature n>
<1> <label> <feature 1> <feature 2> ... <feature n>
...
<i> <label> <feature 1> <feature 2> ... <feature n>
```

**Example:** A dataset with 5 samples, 3 classes, and 2 features per sample:

```
5	3	2
0	1	0.21	0.45
1	1	0.22	0.43
2	2	0.67	1.12
3	2	0.60	1.11
4	3	0.79	0.04
```

**Note:** The file must be in binary format (no blank spaces). For unsupervised classification, use label 0 for all samples.

---

## Data Structures

The C++20 version introduces modern alternatives to the original C data structures:

- **`Node<T>` Class:** A templated class representing a graph node with support for flexible feature vector types (`float`, `double`, etc.)
- **`Subgraph<T>` Class:** Manages a collection of nodes, replacing the original procedural approach
- **STL Containers:** Replaces custom C implementations with:
  - `std::priority_queue` instead of `GQueue` and `RealHeap`
  - `std::set`/`std::unordered_set` instead of custom `Set`
  - `std::vector` for dynamic arrays with automatic memory management

For detailed information about the data structures, see [data_structures.md](data_structures.md).

---

## Workflow

The library supports various OPF operations through object-oriented methods. Here's an overview of the main workflows:

### Supervised Classification

1. **Split:** Partition dataset into training, evaluation, and test sets (`opf_split`)
2. **Train:** Build the OPF classifier from training data (`opf_train`)
3. **Learn (Optional):** Improve classifier by learning from evaluation errors (`opf_learn`)
4. **Classify:** Assign labels to test samples (`opf_classify`)
5. **Accuracy:** Compute classification accuracy (`opf_accuracy`)

### Unsupervised Clustering

1. **Cluster:** Perform OPF-based clustering on unlabeled data (`opf_cluster`)
2. **Classify:** Assign labels using the clustering model (`opf_knn_classify`)

### Auxiliary Operations

- **Distance:** Precompute distance matrices (`opf_distance`)
- **Normalize:** Normalize feature vectors (`opf_normalize`)
- **Info:** Retrieve dataset information (`opf_info`)
- **Fold:** K-fold cross-validation splitting (`opf_fold`)
- **Merge:** Combine datasets (`opf_merge`)
- **Pruning:** Reduce classifier size (`opf_pruning`)
- **Semi-supervised:** Train with labeled and unlabeled data (`opf_semi`)

For detailed workflow information, see [workflow.md](workflow.md).

---

## Conversion Details

The conversion from C to C++20 follows a systematic approach outlined in [conversion_plan.md](conversion_plan.md), including:

- **Data Structure Modernization:** Replacing C structs with C++ classes and STL containers
- **Memory Management:** Leveraging RAII, smart pointers, and automatic cleanup
- **API Redesign:** From procedural C functions to object-oriented methods
- **Build System:** Migration from Makefile to CMake for cross-platform support

For implementation details, see [conversion_plan.md](conversion_plan.md).

---

## How to Build

Use the instructions in [README_GNU_CMake](README_GNU_CMake) to install GNU tools using CMake. Other C++ kits (MSVC//CLANG) can be used on your discretion.

### Prerequisites

- **A C++ compiler** supporting C++20 (GCC 11+, Clang 13+, or MSVC 2019+)
- **CMake** (version 3.20 or later)
- **Git** (for cloning the repository)

### On Linux/macOS

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd LibOPF
   ```

2. **Configure CMake:**
   ```bash
   cmake -S . -B build
   ```

3. **Build the project:**
   ```bash
   cmake --build build
   ```

4. **Run the executables:**
   The compiled programs will be in the `build/bin` directory:
   ```bash
   ./build/bin/opf_split
   ./build/bin/opf_train
   ./build/bin/opf_classify
   # ... and other programs
   ```

### On Windows with MinGW-w64

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd LibOPF
   ```

2. **Configure CMake:**
   ```bash
   cmake -S . -B build
   ```

3. **Build the project:**
   ```bash
   cmake --build build
   ```

4. **Run the executables:**
   The compiled programs will be in the `build\bin` directory:
   ```cmd
   build\bin\opf_split.exe
   build\bin\opf_train.exe
   build\bin\opf_classify.exe
   # ... and other programs
   ```

---

## Visual Studio Code Setup

This project is optimized for development with Visual Studio Code.

### C/C++ Extension

Install the [Microsoft C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) for VS Code to get:

- IntelliSense code completion
- Interactive debugging
- Code navigation and refactoring
- Real-time error checking

### CMake Tools Extension

For enhanced CMake integration, install the [CMake Tools extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools).

### Using GCC with MinGW-w64 on Windows

To build and debug this project on Windows using GCC, you need the MinGW-w64 toolchain.

#### Installation Steps

1. **Install Visual Studio Code and Extensions:**
   - Install [Visual Studio Code](https://code.visualstudio.com/)
   - Install the [C/C++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
   - Install the [CMake Tools extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)

2. **Install MinGW-w64 via MSYS2:**
   - Download and install [MSYS2](https://www.msys2.org/)
   - Open an MSYS2 terminal and run:
     ```bash
     pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
     ```

3. **Add MinGW to your PATH:**
   - Add `C:\msys64\ucrt64\bin` to the Windows PATH environment variable
   - (Adjust the path if you installed MSYS2 to a different location)

4. **Verify the installation:**
   - Open Command Prompt or PowerShell and verify:
     ```bash
     g++ --version
     gdb --version
     cmake --version
     ```

After completing these steps, you can build and debug the project in VS Code using the integrated CMake support.

---

## Quick Start Example

### 1. Split a dataset:
```bash
./build/bin/opf_split data/cone-torus.dat 0.4 0 0.6 0
```
This creates `training.dat` (40%) and `testing.dat` (60%) from the dataset.

### 2. Train the classifier:
```bash
./build/bin/opf_train training.dat
```
This generates `classifier.opf` (the trained model).

### 3. Classify the test set:
```bash
./build/bin/opf_classify testing.dat
```
This produces `testing.dat.out` (predicted labels).

### 4. Compute accuracy:
```bash
./build/bin/opf_accuracy testing.dat
```
This creates `testing.dat.acc` with the accuracy metrics.

---

## Example Scripts

Several example scripts are provided in the `examples/` directory to demonstrate various workflows:

- **example1.sh:** Training and testing without the learning procedure
- **example2.sh:** Learning from evaluation errors
- **example3.sh:** Training and testing with precomputed distances
- **example4.sh:** Learning with precomputed distances
- **example5.sh:** Unsupervised OPF clustering

Run any example script from the root directory, e.g.:
```bash
cd examples
bash example1.sh
```

---

## Available Datasets

Sample datasets are included in the `data/` directory:

**Synthetic Datasets:**
- `cone-torus.dat`: 400 samples, 3 classes, 2 features
- `petals.dat`: 100 samples, 4 classes, 2 features
- `boat.dat`: 100 samples, 3 classes, 2 features
- `saturn.dat`: 200 samples, 2 classes, 2 features

**Artificial Datasets:**
- `mpeg7_BAS.dat`: 1400 samples (70 classes), 180 features
- `mpeg7_FOURIER.dat`: 1400 samples (70 classes), 126 features

---

## Tools

The `tools/` directory contains utility programs for working with OPF files:

- **txt2opf:** Convert ASCII OPF files to binary format
- **opf2txt:** Convert binary OPF files to ASCII format
- **opf_check:** Validate OPF file format
- **statistics:** Compute mean accuracy and standard deviation from results

Example usage:
```bash
# Check if a text file is in valid OPF format
./tools/opf_check mydataset.txt

# Convert to binary format
./tools/txt2opf mydataset.txt mydataset.dat

# Compute statistics
./tools/statistics results.txt 10 "OPF Classification Results"
```

---

## Accuracy Computation

The `opf_accuracy` program computes classification accuracy considering class balance. This prevents classifiers from artificially inflating accuracy by always predicting the most frequent class.

**Formula:**
```
Accuracy = (TP + TN) / (TP + TN + FP + FN)
```
with per-class weighting to account for imbalanced datasets.

For details on the accuracy formula and methodology, visit: http://www.ic.unicamp.br/~afalcao/LibOPF

---

## Testing

The project includes a test suite for validation:

```bash
# Run all tests
cd build
ctest
```

Individual test executables are located in `build/bin/test_*.exe`.

---

## Performance Considerations

When working with large datasets:

1. **Feature Normalization:** Use `opf_normalize` or include the normalization parameter in `opf_split`
2. **Precomputed Distances:** For expensive distance functions, precompute and store with `opf_distance`
3. **K-fold Cross-validation:** Use `opf_fold` and `opf_merge` for robust evaluation
4. **Semi-supervised Learning:** Consider `opf_semi` if limited labeled samples are available
5. **Pruning:** Use `opf_pruning` on evaluation set to reduce model size while maintaining accuracy

---

## References

### Original OPF Publications

```bibtex
@Manual{LibOPF09,
  author = {J.P. Papa and A.X. Falcão and C.T.N. Suzuki},
  title = {{LibOPF}: {A} library for the design of optimum-path forest classifiers},
  institution = {Institute of Computing, University of Campinas},
  year = {2009},
  note = {Software version 2.0 available at 
          http://www.ic.unicamp.br/~afalcao/LibOPF}
}

@article{PapaIJIST09,
  author = {J. P. Papa and A. X. Falcão and Celso T. N. Suzuki},
  title = {Supervised Pattern Classification based on Optimum-Path Forest},
  journal = {International Journal of Imaging Systems and Technology},
  volume = {19},
  issue = {2},
  pages = {120--131},
  publisher = {Wiley-Interscience},
  year = {2009}
}

@article{PapaPR12,
  author = {J. P. Papa and A. X. Falcão and V. H. C. Albuquerque and J. M. R. S. Tavares},
  title = {Efficient supervised optimum-path forest classification for large datasets},
  journal = {Pattern Recognition},
  volume = {45},
  number = {1},
  year = {2012},
  pages = {512--520},
  publisher = {Elsevier Science Inc.}
}

@article{RochaIJIST09,
  author = {L.M. Rocha and F.A.M. Cappabianco and A.X. Falcão},
  title = {Data clustering as an optimum-path forest problem with applications in image analysis},
  journal = {International Journal of Imaging Systems and Technology},
  publisher = {Wiley Periodicals},
  volume = {19},
  number = {2},
  pages = {50--68},
  year = {2009}
}
```

### Dataset References

If using the synthetic datasets, cite:
```bibtex
@article{Kuncheva,
  Author = {L. Kuncheva},
  title = {Artificial Data},
  Journal = {School of Informatics, University of Wales, Bangor},
  url = {http://www.informatics.bangor.ac.uk/~kuncheva},
  year = {1996}
}
```

If using the MPEG-7 dataset, cite:
```bibtex
@article{MPEG-7,
  author = {MPEG-7},
  title = {MPEG-7: The Generic Multimedia Content Description Standard, Part 1},
  journal = {IEEE MultiMedia},
  volume = {09},
  number = {2},
  pages = {78-87},
  year = {2002}
}
```

---

## Contributing

The LibOPF project is maintained with the goal of providing a robust, efficient implementation of Optimum-Path Forest classifiers. Contributions are welcome. Please ensure that:

- Code follows C++20 modern practices
- Changes maintain backward compatibility where possible
- New features include appropriate test coverage
- Documentation is updated accordingly

---

## Support and Contact

For questions, issues, or feedback about the original algorithm, visit:
- **Website:** http://www.ic.unicamp.br/~afalcao/LibOPF
- **Original Authors:**
  - J.P. Papa (papa.joaopaulo@gmail.com)
  - A.X. Falcão (afalcao@ic.unicamp.br)
  
For issues with the C++20 conversion:
- Please open an issue on this repository

---

## License

Please see the [COPYRIGHT](COPYRIGHT) and [COPYING](COPYING) files for license information.

The C++20 conversion is authored by **Leonardo Marques Rocha** and is provided under the same licensing terms as the original LibOPF library.

---

**Last Updated:** February 2026
