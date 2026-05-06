"""
opf — high-level Python API for the OPF (Optimum-Path Forest) library.

All heavy lifting is delegated to the ``opfpy`` C++/pybind11 extension.
This package provides:

* Convenience wrappers / higher-level functions.
* A stable import path: ``from opf import supervised, unsupervised, utils``.
* Re-exports of the most common names from ``opfpy``.

Quick-start
-----------
>>> import sys; sys.path.insert(0, 'bin')   # add built extension dir
>>> from opf.supervised import train_and_classify
>>> from opf.unsupervised import cluster_and_propagate
>>> from opf.utils import load, split, accuracy
"""

# Lazy-load sub-modules on first access so that importing this package
# does not fail if the built extension has not been placed on sys.path yet.

import os
import sys
from importlib import import_module as _imp

_PKG_DIR = os.path.dirname(__file__)
_PYTHONLIB_DIR = os.path.normpath(os.path.join(_PKG_DIR, ".."))
_BIN_DIR = os.path.normpath(os.path.join(_PKG_DIR, "..", "bin"))

if _PYTHONLIB_DIR not in sys.path:
    sys.path.insert(0, _PYTHONLIB_DIR)
if _BIN_DIR not in sys.path:
    sys.path.insert(0, _BIN_DIR)

# Configure runtime DLL search paths once on package import so importing
# opfpy from any opf submodule works without per-module setup code.
try:
    from windows_runtime_helper import add_windows_runtime_dirs
    add_windows_runtime_dirs()
except Exception:
    # If runtime helper is unavailable, defer failure to opfpy import.
    pass

# Import and re-export the low-level extension so one `import opf` is enough.
import opfpy as opfpy

from opf.distance import DistanceMetric, resolve as resolve_distance, register as register_distance
from opf import _repr as _repr_patch  # noqa: F401  # side-effect: patches opfpy reprs

def __getattr__(name: str):
    submodules = ("supervised", "unsupervised", "utils", "distance")
    if name in submodules:
        return _imp(f"opf.{name}")
    raise AttributeError(f"module 'opf' has no attribute {name!r}")

__all__ = [
    "supervised", "unsupervised", "utils", "distance",
    "DistanceMetric", "resolve_distance", "register_distance",
    "opfpy",
]
