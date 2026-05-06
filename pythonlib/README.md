# opf — Python library for Optimum-Path Forest

High-level Python API for the OPF classifier/clusterer, backed by a
C++20/pybind11 extension (`opfpy`).

---

## Requirements

| Tool | Version |
|------|---------|
| Python | ≥ 3.11 |
| C++ compiler | GCC/Clang with C++20 (MSYS2 UCRT64 on Windows) |
| CMake | ≥ 3.16 |
| Conan | 2.x |

---

## Building the Extension

```sh
# From the project root, inside the MSYS2 UCRT64 shell:
bash ./build_opfpy.sh
```

The compiled module (`opfpy.pyd` / `opfpy.so`) is placed in `pythonlib/bin/`.  
See [build_opfpy.md](build_opfpy.md) for step-by-step details.

---

## Quick Start

```python
import sys
sys.path.insert(0, "bin")          # add built extension

from opf.utils import load, split, accuracy
from opf.supervised import train_and_classify

data = load("../data/boat.dat")
train_sg, test_sg = split(data, 0.5)
acc = train_and_classify(train_sg, test_sg)
print(f"Accuracy: {acc:.2%}")
```

---

## Package Layout

```
opf/
  __init__.py          # package root; lazy-loads sub-modules
  utils.py             # I/O, split/merge/normalize, accuracy, distance matrix
  supervised.py        # train, classify, learn, prune helpers
  unsupervised.py      # cluster, knn_classify, semi_supervised helpers
examples/
  example1_supervised.py            # supervised OPF, no learning
  example2_learning.py              # supervised OPF with learning
  example3_precomputed_distances.py # distance matrix I/O + supervised OPF
  example4_normalization.py         # feature normalization + supervised OPF
  example5_unsupervised.py          # unsupervised clustering + k-NN classify
  example6_semi_supervised.py       # semi-supervised OPF
```

---

## API Reference

### `opf.utils`

| Function | Description |
|----------|-------------|
| `load(path)` | Load subgraph from LibOPF `.dat` file (original format) |
| `read(path)` | Read subgraph from OPF training binary format |
| `write(path, sg)` | Write subgraph to OPF training binary format |
| `split(sg, pct)` | Label-stratified split into two subgraphs |
| `merge(sg1, sg2)` | Merge two subgraphs |
| `k_fold(sg, k)` | Stratified k-fold partition |
| `normalize(sg)` | Z-score feature normalization **in-place** |
| `accuracy(sg)` | Accuracy from `label` vs `truelabel` |
| `info(sg)` | Dict with `nnodes`, `nlabels`, `nfeats` |
| `compute_distance_matrix(sg, id)` | Pairwise distance matrix (id 1–7) |
| `write_distance_matrix(mat, path)` | Write binary distance matrix |
| `read_distance_matrix(path)` | Read binary distance matrix |

Distance IDs: 1 Euclidean · 2 Chi-squared · 3 Manhattan · 4 Canberra ·
5 Squared-chord · 6 Squared-chi-squared · 7 Bray-Curtis.

---

### `opf.supervised`

| Function | Description |
|----------|-------------|
| `train(sg_train)` | Train supervised OPF in-place |
| `classify(sg_train, sg_test)` | Classify test nodes in-place |
| `train_and_classify(sg_train, sg_test)` | Train + classify; returns accuracy |
| `learn_and_classify(train, eval, test, n_iter)` | Iterative learning + classify; returns accuracy |
| `prune(sg_train, sg_eval, tolerance)` | Iterative pruning; returns pruning rate |

---

### `opf.unsupervised`

| Function | Description |
|----------|-------------|
| `cluster_and_propagate(sg, k)` | Cluster in-place and propagate labels |
| `knn_classify(sg_train, sg_test)` | k-NN classify using stored radii |
| `semi_supervised(labeled, unlabeled, eval?)` | Semi-supervised learning; returns merged graph |

---

### Low-level extension (`opfpy`)

The `opfpy` C++ extension exposes:

| Symbol | Type | Description |
|--------|------|-------------|
| `Node` | class | Single graph node with all OPF fields |
| `Subgraph` | class | Container of nodes + graph metadata |
| `OPF` | class | Classifier / learner methods |
| `read_subgraph` / `write_subgraph` | functions | Binary I/O |
| `split_subgraph` | function | Stratified split |
| `propagate_cluster_labels` | function | Label propagation |
| `eucl_dist`, `chi_squared_dist`, … | functions | Distance metrics |
| `subgraph_info` | function | Metadata dict |
| `k_fold` | function | k-fold partition |
| `merge_subgraphs` | function | Merge |
| `compute_distance_matrix` | function | Pairwise distances |
| `write_distance_matrix` / `read_distance_matrix` | functions | Distance matrix I/O |

---

## Running the Tests

```sh
cd pythonlib
# activate .venv first
python -m unittest discover -v
```

Expected: **54+ tests, OK** across Phases 1–6.

---

## Running the Examples

```sh
cd pythonlib
python examples/example1_supervised.py
python examples/example2_learning.py
python examples/example3_precomputed_distances.py
python examples/example4_normalization.py
python examples/example5_unsupervised.py
python examples/example6_semi_supervised.py
```
