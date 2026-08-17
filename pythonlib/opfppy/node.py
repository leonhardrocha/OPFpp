"""
opfppy.node — Python shim for opfpy.Node.

Follows the same pattern as opfppy.distance: the raw C++ type (opfpy.Node)
is the "integer id" analogue; this module wraps it in a friendlier Python
class and provides a ``wrap()`` factory analogous to ``resolve()``.

Usage
-----
>>> from opfppy.node import Node
>>> n = Node()
>>> n.label = 1
>>> n.feat = [1.0, 2.0, 3.0]
>>> print(n)

Extending
---------
Subclass ``Node`` to add domain-specific attributes or methods; all
pybind11 properties (label, truelabel, pathval, dens, radius, root,
pred, position, status, relevant, nplatadj, feat, adj) are inherited.
"""

from __future__ import annotations

import functools
from typing import Sequence

import opfpy as _opfpy


class _NodeParentProxy:
    """Proxy that exposes a raw ``opfpy.Node`` through Python methods/properties."""

    def __init__(self, parent: _opfpy.Node):
        object.__setattr__(self, "_parent", parent)

    @property
    def parent(self) -> _opfpy.Node:
        """Return the wrapped raw ``opfpy.Node`` instance."""
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
        # Reuse shim repr against the wrapped parent object.
        return Node.__repr__(object.__getattribute__(self, "_parent"))


# ---------------------------------------------------------------------------
# Formatting helper (mirrors _repr.py but owned by this module)
# ---------------------------------------------------------------------------

def _fmt_seq(values: Sequence, *, head: int = 3, tail: int = 2, precision: int = 4) -> str:
    """Format a sequence with head/tail truncation and element count."""
    items = list(values)
    n = len(items)

    def _fmt(v):
        if isinstance(v, float):
            return f"{v:.{precision}g}"
        return str(v)

    if n <= head + tail + 1:
        rendered = ", ".join(_fmt(v) for v in items)
    else:
        left = ", ".join(_fmt(v) for v in items[:head])
        right = ", ".join(_fmt(v) for v in items[-tail:])
        rendered = f"{left}, ..., {right}"
    return f"[{rendered}] ({n})"


# ---------------------------------------------------------------------------
# Python shim class
# ---------------------------------------------------------------------------

class Node(_opfpy.Node):
    """Python-level wrapper around ``opfpy.Node``.

    Inherits every pybind11 property and method from the C++ binding and
    adds a pretty ``__repr__``.  Use :meth:`wrap` to convert a bare
    ``opfpy.Node`` instance returned by C-level code into an ``opfppy.Node``.
    """

    def __repr__(self) -> str:  # noqa: D105
        return (
            "Node("
            f"label={self.label}, truelabel={self.truelabel}, "
            f"pathval={self.pathval:.6g}, dens={self.dens:.6g}, radius={self.radius:.6g}, "
            f"root={self.root}, pred={self.pred}, position={self.position}, "
            f"status={self.status}, relevant={self.relevant}, nplatadj={self.nplatadj}, "
            f"feat={_fmt_seq(self.feat)}, adj={_fmt_seq(self.adj, precision=0)}"
            ")"
        )

    # ------------------------------------------------------------------
    # Factory — analogous to distance.resolve()
    # ------------------------------------------------------------------

    @classmethod
    def wrap(cls, node: _opfpy.Node) -> "Node | _NodeParentProxy":
        """Wrap a raw ``opfpy.Node`` in a Python proxy shim.

        Parameters
        ----------
        node : opfpy.Node
            Raw C-extension node instance.

        Returns
        -------
        Node | _NodeParentProxy
            Existing shim instance when already wrapped, otherwise a proxy that
            forwards methods/properties to the parent object.
        """
        if isinstance(node, cls):
            return node
        if not isinstance(node, _opfpy.Node):
            raise TypeError(f"Expected opfpy.Node, got {type(node)!r}")
        return _NodeParentProxy(node)

    # ------------------------------------------------------------------
    # Registry — analogous to distance.register()
    # Allows subclasses to register themselves as the default wrap target.
    # ------------------------------------------------------------------

    _registry: dict[str, type] = {}

    @classmethod
    def register(cls, name: str, subclass: type) -> None:
        """Register *subclass* under *name* for domain-specific wrapping.

        Parameters
        ----------
        name     : str   Logical name for the subclass.
        subclass : type  Must be a subclass of :class:`Node`.
        """
        if not issubclass(subclass, cls):
            raise TypeError(f"{subclass!r} is not a subclass of Node")
        cls._registry[name] = subclass
