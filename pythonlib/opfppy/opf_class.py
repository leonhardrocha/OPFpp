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

import functools
import opfpy as _opfpy


class _OPFParentProxy:
    """Proxy that exposes a raw ``opfpy.OPF`` through Python methods/properties."""

    def __init__(self, parent: _opfpy.OPF):
        object.__setattr__(self, "_parent", parent)

    @property
    def parent(self) -> _opfpy.OPF:
        """Return the wrapped raw ``opfpy.OPF`` instance."""
        return object.__getattribute__(self, "_parent")

    def __getattr__(self, name):
        parent = object.__getattribute__(self, "_parent")
        attr = getattr(parent, name)
        if callable(attr):
            @functools.wraps(attr)
            def _wrapped(*args, **kwargs):
                return attr(*args, **kwargs)
            return _wrapped
        return attr

    def __setattr__(self, name, value):
        if name == "_parent":
            object.__setattr__(self, name, value)
            return
        setattr(object.__getattribute__(self, "_parent"), name, value)

    def __repr__(self) -> str:
        return OPF.__repr__(object.__getattribute__(self, "_parent"))


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
    def wrap(cls, opf: _opfpy.OPF) -> "OPF | _OPFParentProxy":
        """Wrap a raw ``opfpy.OPF`` in a Python proxy shim.

        Parameters
        ----------
        opf : opfpy.OPF
            Raw C-extension OPF instance.

        Returns
        -------
        OPF | _OPFParentProxy
            Existing shim instance when already wrapped, otherwise a proxy that
            forwards methods/properties to the parent object.
        """
        if isinstance(opf, cls):
            return opf
        if not isinstance(opf, _opfpy.OPF):
            raise TypeError(f"Expected opfpy.OPF, got {type(opf)!r}")
        return _OPFParentProxy(opf)

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
