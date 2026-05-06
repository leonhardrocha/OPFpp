"""
opfppy.opf_class — Python shim for opfpy.OPF.

Follows the same pattern as opfppy.distance: wraps the raw C++ ``opfpy.OPF``
in a friendlier Python class.  The ``wrap()`` factory converts any bare
``opfpy.OPF`` to an ``opfppy.OPF``.

Usage
-----
>>> from opfppy.opf_class import OPF
>>> clf = OPF()
>>> clf.train(sg_train)
>>> clf.classify(sg_train, sg_test)

Extending
---------
Subclass :class:`OPF` to add or override workflow methods; all pybind11
methods (train, classify, learn, accuracy, cluster, knn_classify,
semi_supervised, normalize, pruning) are inherited.
Call :meth:`register` to record the subclass for domain lookup.
"""

from __future__ import annotations

import opfpy as _opfpy


# ---------------------------------------------------------------------------
# Python shim class
# ---------------------------------------------------------------------------

class OPF(_opfpy.OPF):
    """Python-level wrapper around ``opfpy.OPF``.

    Inherits every pybind11 method from the C++ binding and adds a
    ``__repr__``.  Use :meth:`wrap` to convert a bare ``opfpy.OPF``
    returned by C-level code into an ``opfppy.OPF``.
    """

    def __repr__(self) -> str:  # noqa: D105
        return "OPF()"

    # ------------------------------------------------------------------
    # Factory — analogous to distance.resolve()
    # ------------------------------------------------------------------

    @classmethod
    def wrap(cls, opf: _opfpy.OPF) -> "OPF":
        """Not supported: pybind11 C-extension types block ``__class__`` reassignment.

        Create a new :class:`OPF` instance directly: ``clf = OPF()``.
        """
        raise NotImplementedError(
            "Cannot promote opfpy.OPF to opfppy.OPF via __class__ assignment. "
            "Create an opfppy.OPF() instance directly."
        )

    # ------------------------------------------------------------------
    # Registry — analogous to distance.register()
    # ------------------------------------------------------------------

    _registry: dict[str, type] = {}

    @classmethod
    def register(cls, name: str, subclass: type) -> None:
        """Register *subclass* under *name* for domain-specific wrapping.

        Parameters
        ----------
        name     : str   Logical name for the subclass.
        subclass : type  Must be a subclass of :class:`OPF`.
        """
        if not issubclass(subclass, cls):
            raise TypeError(f"{subclass!r} is not a subclass of OPF")
        cls._registry[name] = subclass
