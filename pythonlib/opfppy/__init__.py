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

# ---------------------------------------------------------------------------
# Bootstrap: ensure the built extension (.pyd / .so) and helpers are findable
# ---------------------------------------------------------------------------

_PKG_DIR = os.path.dirname(__file__)
_PYTHONLIB_DIR = os.path.normpath(os.path.join(_PKG_DIR, ".."))
_PKG_BIN_DIR = os.path.normpath(os.path.join(_PKG_DIR, "bin"))
_BIN_DIR = os.path.normpath(os.path.join(_PKG_DIR, "..", "bin"))

if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)
# Wheel/self-contained layout: opfpy binary is bundled under opfppy/bin.
if os.path.isdir(_PKG_BIN_DIR) and _PKG_BIN_DIR not in sys.path:
    sys.path.insert(0, _PKG_BIN_DIR)
# Development/source layout fallback.
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

# Re-export low-level free functions at package top-level.
hello = _opfpy.hello
propagate_cluster_labels = _opfpy.propagate_cluster_labels
read_subgraph = _opfpy.read_subgraph
write_subgraph = _opfpy.write_subgraph
split_subgraph = _opfpy.split_subgraph
eucl_dist = _opfpy.eucl_dist
chi_squared_dist = _opfpy.chi_squared_dist
manhattan_dist = _opfpy.manhattan_dist
canberra_dist = _opfpy.canberra_dist
squared_chord_dist = _opfpy.squared_chord_dist
squared_chi_squared_dist = _opfpy.squared_chi_squared_dist
bray_curtis_dist = _opfpy.bray_curtis_dist
subgraph_info = _opfpy.subgraph_info
k_fold = _opfpy.k_fold
merge_subgraphs = _opfpy.merge_subgraphs
compute_distance_matrix = _opfpy.compute_distance_matrix
write_distance_matrix = _opfpy.write_distance_matrix

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
from opfppy.ply_adapter import encode_sh_params, decode_sh_params, subgraph_from_ply_file, SplatSubGraph
from opfppy.colormap import label_rgb, build_palette, labels_to_rgb_array, load_colormap

# ---------------------------------------------------------------------------
# Re-export every free function from opfpy so callers never need to touch
# the low-level extension directly.
# ---------------------------------------------------------------------------


# Explicitly export convenience submodules.
from opfppy import supervised, unsupervised, utils, distance, node, subgraph, opf_class, ply_adapter, colormap
from opfppy.unsupervised import split_subgraph_into_kernels, bestk_cluster_and_propagate


__all__ = [
    # Shim classes
    "Node", "Subgraph", "OPF",
    # Distance helpers
    "DistanceMetric", "resolve_distance", "register_distance",
    # PLY adapter helpers
    "encode_sh_params", "decode_sh_params", "subgraph_from_ply_file", "SplatSubGraph",
    # Colormap helpers
    "label_rgb", "build_palette", "labels_to_rgb_array", "load_colormap",
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
    "distance", "node", "subgraph", "opf_class", "ply_adapter", "colormap",
    # Unsupervised helpers
    "split_subgraph_into_kernels", "bestk_cluster_and_propagate",
    # Low-level extension (escape hatch)
    "_opfpy",
]
