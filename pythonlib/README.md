# opfppy — OPF Pretty Python

**opfppy** (**OPF Pretty PYthon**) is a high-level Python API for the
Optimum-Path Forest classifier/clusterer, backed by a C++20/pybind11 extension
(`opfpy`).

---

## Python Layer Naming Convention

This project has three Python-facing layers, each with a distinct purpose:

| Layer | Package / module | Full name | Purpose |
|-------|-----------------|-----------|---------|
| C++ extension | `opfpy` | OPF Python | Raw pybind11 bindings — thin bridge to C++; no Python utilities |
| Cython wrapper | `opfpy_cython` | OPF Python (Cython) | Typed Cython classes over `opfpy`; low-level, no high-level helpers |
| Python shim | `opfppy` | **OPF Pretty PYthon** | Full Python API: pretty repr, `DistanceMetric` enum, high-level workflows |

### What `opfppy` adds over the raw `opfpy` / `opfpy_cython` layers

The `opfpy` and `opfpy_cython` layers expose only what the C++ binding
provides directly.  `opfppy` adds:

| Feature | `opfpy` / `opfpy_cython` | `opfppy` |
|---------|--------------------------|---------|
| Pretty `__repr__` for `Node`, `Subgraph`, `OPF` | ❌ raw `<opfpy.Node object …>` | ✅ human-readable, head/tail truncation |
| `DistanceMetric` enum (`EUCLIDEAN`, `MANHATTAN`, …) | ❌ integer ids only | ✅ named constants + string resolver |
| `resolve_distance(name/enum/int)` | ❌ | ✅ `"manhattan"`, `"L1"`, `DistanceMetric.MANHATTAN`, `3` all accepted |
| `register_distance(name, id)` | ❌ | ✅ extension point for custom metrics |
| High-level workflow functions | ❌ | ✅ `train_and_classify`, `learn_and_classify`, `cluster_and_propagate`, `semi_supervised`, … |
| Utility helpers | ❌ | ✅ `load`, `split`, `merge`, `normalize`, `accuracy`, `info`, `k_fold`, `compute_distance_matrix`, `write/read_distance_matrix` |
| Windows DLL setup | ❌ (must be done manually) | ✅ automatic on `import opfppy` |
| Single bootstrap import | ❌ requires `sys.path` manipulation | ✅ `import opfppy` is enough |

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
import opfppy                                     # single import — DLL setup done automatically

data = opfppy.Subgraph.from_original_file("../data/boat.dat")
train_sg, test_sg = opfppy.split_subgraph(data, 0.5)

from opfppy.supervised import train_and_classify
acc = train_and_classify(train_sg, test_sg)
print(f"Accuracy: {acc:.2%}")
```

---

## Package Layout

```
opfppy/
  __init__.py          # package root; bootstrap (sys.path, DLL dirs), re-exports
  node.py              # Node shim class — pretty repr, wrap(), register()
  subgraph.py          # Subgraph shim class — pretty repr, factory class-methods
  opf_class.py         # OPF shim class — pretty repr, wrap(), register()
  distance.py          # DistanceMetric enum, resolve(), register()
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

### `opfppy` top-level (shim classes)

| Symbol | Description |
|--------|-------------|
| `Node` | Shim over `opfpy.Node` — inherits all C++ properties + pretty `__repr__` |
| `Subgraph` | Shim over `opfpy.Subgraph` — factory class-methods return `opfppy.Subgraph` |
| `OPF` | Shim over `opfpy.OPF` — inherits all C++ methods + pretty `__repr__` |
| `DistanceMetric` | `IntEnum`: `EUCLIDEAN=1 … BRAY_CURTIS=7` |
| `resolve_distance(x)` | Resolve string / enum / int → integer id |
| `register_distance(name, id)` | Register a custom metric name |

All `opfpy` free functions are also re-exported directly from `opfppy`:
`split_subgraph`, `read_subgraph`, `write_subgraph`, `propagate_cluster_labels`,
`eucl_dist`, `chi_squared_dist`, `manhattan_dist`, `canberra_dist`,
`squared_chord_dist`, `squared_chi_squared_dist`, `bray_curtis_dist`,
`subgraph_info`, `k_fold`, `merge_subgraphs`, `compute_distance_matrix`,
`write_distance_matrix`, `hello`.

---

### `opfppy.distance`

| Symbol | Description |
|--------|-------------|
| `DistanceMetric` | `IntEnum` with named constants for all 7 built-in metrics |
| `resolve(x)` | Accept `int`, `str` (case-insensitive, aliases: `l1`, `l2`, `chi2`, `bray`, …), or `DistanceMetric` → `int` |
| `register(name, id)` | Add a custom metric string alias |

---

### `opfppy.utils`

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
| `compute_distance_matrix(sg, distance)` | Pairwise distance matrix — accepts int / str / `DistanceMetric` |
| `write_distance_matrix(mat, path)` | Write binary distance matrix |
| `read_distance_matrix(path)` | Read binary distance matrix |

Distance IDs / names: `1`/`"euclidean"` · `2`/`"chi_squared"` · `3`/`"manhattan"` ·
`4`/`"canberra"` · `5`/`"squared_chord"` · `6`/`"squared_chi_squared"` · `7`/`"bray_curtis"`.  
Aliases: `l1` → manhattan, `l2` → euclidean, `chi2` → chi_squared, `bray` → bray_curtis, etc.

---

### `opfppy.supervised`

| Function | Description |
|----------|-------------|
| `train(sg_train)` | Train supervised OPF in-place |
| `classify(sg_train, sg_test)` | Classify test nodes in-place |
| `train_and_classify(sg_train, sg_test)` | Train + classify; returns accuracy |
| `learn_and_classify(train, eval, test, n_iter)` | Iterative learning + classify; returns accuracy |
| `prune(sg_train, sg_eval, tolerance)` | Iterative pruning; returns pruning rate |

---

### `opfppy.unsupervised`

| Function | Description |
|----------|-------------|
| `cluster_and_propagate(sg, k)` | Cluster in-place and propagate labels |
| `knn_classify(sg_train, sg_test)` | k-NN classify using stored radii |
| `semi_supervised(labeled, unlabeled, eval?)` | Semi-supervised learning; returns merged graph |

---

### Low-level C++ extension (`opfpy`) and Cython wrapper (`opfpy_cython`)

`opfpy` is the raw pybind11 binding — the C++ extension module. It exposes
`Node`, `Subgraph`, and `OPF` as plain C-extension types with no Python
utilities.  `opfpy_cython` wraps these in typed Cython classes.

Neither layer provides:
- pretty `__repr__` (objects print as `<opfpy.Node object at 0x…>`)
- `DistanceMetric` enum or string distance resolution
- `resolve_distance` / `register_distance`
- workflow helpers (`train_and_classify`, `normalize`, `accuracy`, etc.)
- Windows DLL directory setup
- single-import bootstrap (`sys.path` must be set manually)

Use `opfppy` for all application code. Access `opfppy._opfpy` only when you
need the raw C-extension objects for performance-critical inner loops.

| Symbol | Type | Description |
|--------|------|-------------|
| `Node` | class | Single graph node with all OPF fields |
| `Subgraph` | class | Container of nodes + graph metadata |
| `OPF` | class | Classifier / learner methods |
| `read_subgraph` / `write_subgraph` | functions | Binary I/O |
| `split_subgraph` | function | Stratified split |
| `propagate_cluster_labels` | function | Label propagation |
| `eucl_dist`, `chi_squared_dist`, … | functions | Distance metrics (integer id only) |
| `subgraph_info` | function | Metadata dict |
| `k_fold` | function | k-fold partition |
| `merge_subgraphs` | function | Merge |
| `compute_distance_matrix` | function | Pairwise distances (integer id only) |
| `write_distance_matrix` / `read_distance_matrix` | functions | Distance matrix I/O |

---

## Running the Tests

```sh
cd pythonlib
# activate .venv first
python -m unittest discover -v
```

Expected: **70 tests, OK** across Phases 1–6.

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
