"""
opfppy — high-level Python API for the OPF (Optimum-Path Forest) library.

All heavy lifting is delegated to the ``opfpy`` C++/pybind11 extension.
This package provides:

* Python shim classes for every opfpy C++ type (Node, Subgraph, OPF) with
  pretty repr and ``wrap()`` / ``register()`` factories — the same pattern
  used by :mod:`opfppy.distance`.
* Re-exports of all opfpy free functions so ``import opfppy`` is sufficient.
* Centralised Windows runtime DLL setup (no per-script boilerplate needed).
* Convenience wrappers: :mod:`opfppy.supervised`, :mod:`opfppy.unsupervised`,
  :mod:`opfppy.utils`.

Quick-start
-----------
>>> import opfppy
>>> sg = opfppy.Subgraph.from_original_file("data.dat")
>>> train, test = opfppy.split_subgraph(sg, 0.5)
>>> clf = opfppy.OPF()
>>> clf.train(train)
>>> clf.classify(train, test)
>>> print(clf.accuracy(test))
"""

import os
import sys
from importlib import import_module as _imp

# ---------------------------------------------------------------------------
# Bootstrap: ensure the built extension (.pyd / .so) and helpers are findable
# ---------------------------------------------------------------------------

_PKG_DIR = os.path.dirname(__file__)
_PYTHONLIB_DIR = os.path.normpath(os.path.join(_PKG_DIR, ".."))
_BIN_DIR = os.path.normpath(os.path.join(_PKG_DIR, "..", "bin"))

if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)
if _BIN_DIR not in sys.path:
    sys.path.insert(0, _BIN_DIR)

# Configure Windows runtime DLL directories once so opfpy loads anywhere.
try:
    from windows_runtime_helper import add_windows_runtime_dirs
    add_windows_runtime_dirs()
except Exception:
    pass  # non-Windows or runtime helper absent — defer failure to opfpy import

# ---------------------------------------------------------------------------
# Low-level C++ extension (kept accessible as opfppy._opfpy)
# ---------------------------------------------------------------------------

import opfpy as _opfpy

# ---------------------------------------------------------------------------
# Python shim classes — Node, Subgraph, OPF
# Each module follows the opfppy.distance pattern:
#   • Python class wrapping the C type  (analogous to DistanceMetric)
#   • wrap()     factory                (analogous to resolve())
#   • register() extension point        (analogous to register())
# ---------------------------------------------------------------------------

from opfppy.node      import Node
from opfppy.subgraph  import Subgraph
from opfppy.opf_class import OPF

# ---------------------------------------------------------------------------
# Distance helpers
# ---------------------------------------------------------------------------

from opfppy.distance import DistanceMetric, resolve as resolve_distance, register as register_distance

# ---------------------------------------------------------------------------
# Re-export every free function from opfpy so callers never need to touch
# the low-level extension directly.
# ---------------------------------------------------------------------------

from opfpy import (
    hello,
    propagate_cluster_labels,
    read_subgraph,
    write_subgraph,
    split_subgraph,
    eucl_dist,
    chi_squared_dist,
    manhattan_dist,
    canberra_dist,
    squared_chord_dist,
    squared_chi_squared_dist,
    bray_curtis_dist,
    subgraph_info,
    k_fold,
    merge_subgraphs,
    compute_distance_matrix,
    write_distance_matrix,
)

# ---------------------------------------------------------------------------
# Lazy sub-module access
# ---------------------------------------------------------------------------

def __getattr__(name: str):
    submodules = ("supervised", "unsupervised", "utils", "distance",
                  "node", "subgraph", "opf_class")
    if name in submodules:
        return _imp(f"opfppy.{name}")
    raise AttributeError(f"module 'opfppy' has no attribute {name!r}")


__all__ = [
    # Shim classes
    "Node", "Subgraph", "OPF",
    # Distance helpers
    "DistanceMetric", "resolve_distance", "register_distance",
    # Free functions re-exported from opfpy
    "hello",
    "propagate_cluster_labels",
    "read_subgraph", "write_subgraph", "split_subgraph",
    "eucl_dist", "chi_squared_dist", "manhattan_dist", "canberra_dist",
    "squared_chord_dist", "squared_chi_squared_dist", "bray_curtis_dist",
    "subgraph_info", "k_fold", "merge_subgraphs",
    "compute_distance_matrix", "write_distance_matrix",
    # Sub-modules
    "supervised", "unsupervised", "utils",
    "distance", "node", "subgraph", "opf_class",
    # Low-level extension (escape hatch)
    "_opfpy",
]
